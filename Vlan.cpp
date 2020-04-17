// Copyright 2020 - Present | Pawel Maslanka (pawmas.pawelmaslanka@gmail.com)
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//      http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include "Vlan.hpp"
#include "Utils/LoggingFacility.hpp"
#include "Utils/OpenNslException.hpp"

#include "OpenNosException.hpp"

Vlan::Vlan(const VlanId vid)
    : _vid { vid }, _created { false } {
    // Nothing more to do
}

Vlan::~Vlan() {
    if (_created) {
        destroy();
    }
}

Result Vlan::create() {
    /// @todo Send request to HW in order to create VLAN
    _created = true;
    notifyAboutCreatedVlan();
}

Result Vlan::destroy() {
    /// @todo Send request to HW in order to delete VLAN
    _created = false;
    notifyAboutDestroyedVlan();
}

VlanManager::VlanManager(Command::ResultCallback::Handle callback)
    : UndoableCommand { callback } {

}

Result VlanManager::create(const VlanId vid) {
    if (_committedVlans.find(vid) != std::end(_committedVlans)) {
        return Result::AlreadyExists;
    }

    auto vlan = std::make_shared<Vlan>(vid);
    Result result = vlan->create();
    if (not Failed(result)) {
        _committedVlans.emplace(vid, vlan);
    }

    return result;
}

Result VlanManager::destroy(const VlanId vid) {
    const auto foundVlanIt = _committedVlans.find(vid);
    if (foundVlanIt == std::end(_committedVlans)) {
        return Result::VlanNotExists;
    }

    _committedVlans.erase(foundVlanIt);
    return Result::Success;
}

void VlanManager::execute() {
    for (auto vid : _destroyVlan) {
        /// @todo Pass request to HW about destroy VLAN
    }

    for (auto vid : _createVlan) {
        /// @todo Pass request to HW about create VLAN
    }
}

Vlan::Handle VlanManager::getVlan(const VlanId vid) const {
    if (not vlanExists(vid)) {
        throw MAKE_OPENNOS_EXCEPTION(Result::VlanNotExists);
    }

    return _committedVlans.find(vid)->second;
}

bool VlanManager::vlanExists(const VlanId vid) const {
    return _committedVlans.find(vid) != std::end(_committedVlans);
}

ManagingVlanMemberPorts::ManagingVlanMemberPorts(VlanId vlan, ResultCallback::Handle& operationCallback)
    : _vid { vlan }, _operationCallback { operationCallback } {
    // Nothing more to do
}

AddingVlanMemberPorts::AddingVlanMemberPorts(VlanId vlan, ResultCallback::Handle callback)
    : ManagingVlanMemberPorts { vlan, callback } {
    // Nothing more to do
}

Result AddingVlanMemberPorts::addVlanMemberPorts(const PortId portNo, const TaggingMode taggingMode) {
    auto foundPortIt = _ports->find(portNo);
    if (foundPortIt != std::end(*_ports)) {
        _ports->emplace(portNo, taggingMode);
        return Result::Success;
    }

    if ((taggingMode.tagged == foundPortIt->second.tagged)
            && (taggingMode.untagged == foundPortIt->second.untagged)) {
        return Result::AlreadyExists;
    }

    foundPortIt->second = taggingMode;

    return Result::Success;
}

size_t AddingVlanMemberPorts::getCommitOrderingResolve() const noexcept {
    return CommitOrderingResolve::VlanAddMemberPorts;
}

RemovingVlanMemberPorts::RemovingVlanMemberPorts(VlanId vid, Command::ResultCallback::Handle callback)
    : ManagingVlanMemberPorts { vid, callback } {
    // Nothing more to do
}

size_t RemovingVlanMemberPorts::getCommitOrderingResolve() const noexcept {
    return CommitOrderingResolve::VlanRemoveMemberPorts;
}

VlanMemberPortManager::VlanMemberPortManager(VlanManager::Handle vlanManager, HwCommandProcessor::Handle hwCommandProcessor, Vlan::Handle vlan)
    : _vlanManager { vlanManager }, _hwCommandProcessor { hwCommandProcessor },
      _nullResultCallback { std::dynamic_pointer_cast<Command::ResultCallback>(std::make_shared<NullResultCallback>()) } {
    // Nothing more to do
}

Result VlanMemberPortManager::addMemberPort(Vlan::Handle& vlan, const PortId portNo, const bool tagged, const bool untagged) {
    auto foundForVlanIt = _committedMemberPorts.find(vlan);
    if (foundForVlanIt == std::end(_committedMemberPorts)) {
        _addMemberPorts.emplace(vlan, std::make_pair(portNo, TaggingMode{ tagged, untagged }));
        return Result::Success;
    }

    auto& portTaggingModes = foundForVlanIt->second;
    auto foundForPortIt = portTaggingModes.find(portNo);
    if (foundForPortIt == std::end(portTaggingModes)) {
        portTaggingModes.emplace(portNo, TaggingMode{ tagged, untagged });
        return Result::Success;
    }

    auto& taggingMode = foundForPortIt->second;
    if ((tagged == taggingMode.tagged)
            && (untagged == taggingMode.untagged)) {
        return Result::AlreadyExists;
    }

    /// @todo Should we schedule remove VLAN port member before update its tagging mode?
    Result removeMemberPortResult = removeMemberPort(vlan, portNo);
    if (Failed(removeMemberPortResult)) {
        return removeMemberPortResult;
    }

    taggingMode.tagged = tagged;
    taggingMode.untagged = untagged;
    return Result::Success;
}

Result VlanMemberPortManager::removeMemberPort(Vlan::Handle& vlan, const PortId portNo) {
    auto foundForVlanIt = _committedMemberPorts.find(vlan->id());
    if (foundForVlanIt == std::end(_committedMemberPorts)) {
        return Result::VlanNotExists;
    }

    auto foundForPortIt = foundForVlanIt->second.find(portNo);
    if (foundForPortIt == std::end(foundForVlanIt->second)) {
        return Result::PortInVlanNotExists;
    }

    _removeMemberPorts.emplace(vlan->id(), portNo);
    return Result::Success;
}

void VlanMemberPortManager::execute() {
    _mementoAddedMemberPorts.clear();
    _mementoRemovedMemberPorts.clear();
    Result result {};

    for (auto& vlansPort : _removeMemberPorts) {
        auto vlanId = vlansPort.first;
        if (_removingVlanMemberPortsHandles.find(vlanId) == std::end(_removingVlanMemberPortsHandles)) {
            _removingVlanMemberPortsHandles.emplace(vlanId, RemovingVlanMemberPorts(vlanId, shared_from_this()));
        }

        for (auto& memberPortToRemove : vlansPort.second) {
            result = _removingVlanMemberPortsHandles[vlanId].removeVlanMemberPorts(memberPortToRemove);
            if (Failed(result)) {
                undo(_nullResultCallback);
                std::string errorMsg {};
                errorMsg.append("Failed to remove port no. ")
                        .append(std::to_string(memberPortToRemove))
                        .append(" from VLAN ")
                        .append(std::to_string(vlanId));
                callback->onCommandResultEvent(result, errorMsg);
                return;
            }

            _mementoRemovedMemberPorts.emplace(vlanId, _committedMemberPorts.find(vlanId));
        }
    }

    for (auto& vlansPort : _addMemberPorts) {
        auto vlanId = vlansPort.first;
        if (_addingVlanMemberPortsHandles.find(vlanId) == std::end(_addingVlanMemberPortsHandles)) {
            _addingVlanMemberPortsHandles.emplace(vlanId, AddingVlanMemberPorts(vlanId, shared_from_this()));
        }

        for (auto& memberPortToAdd : vlansPort.second) {
            result = _addingVlanMemberPortsHandles[vlanId].addVlanMemberPorts(memberPortToAdd.first, memberPortToAdd.second);
            if (Failed(result)) {
                undo(_nullResultCallback);
                std::string errorMsg {};
                errorMsg.append("Failed to remove port no. ")
                        .append(std::to_string(memberPortToAdd.first))
                        .append(" from VLAN ")
                        .append(std::to_string(vlanId));
                callback->onCommandResultEvent(result, errorMsg);
                return;
            }

            _mementoAddedMemberPorts.emplace(vlanId, memberPortToAdd.first);
        }
    }

    for (auto& commitRemovedMemberPorts : _removingVlanMemberPortsHandles) {
        commitRemovedMemberPorts.second.execute(std::dynamic_pointer_cast<Command::ResultCallback>(shared_from_this()));
    }

    for (auto& commitAddedMemberPorts : _addingVlanMemberPortsHandles) {
        commitAddedMemberPorts.second.execute(std::dynamic_pointer_cast<Command::ResultCallback>(shared_from_this()));
    }
}

void VlanMemberPortManager::undo() {
    Result result {};
    for (auto& memberPortToRemove : _mementoAddedMemberPorts) {
        for (auto& port : memberPortToRemove.second) {
            result = _removingVlanMemberPortsHandles[memberPortToRemove.first].removeVlanMemberPorts(port);
            if (Failed(result)) {
                std::string errorMsg {};
                errorMsg.append("Failed to remove port no. ")
                    .append(std::to_string(port))
                    .append(" from VLAN ")
                    .append(std::to_string(portMemberToRemove.first));
                callback->onCommandResultEvent(result, errorMsg);
            }
        }
    }

    /// @todo the same for removed

    _mementoAddedMemberPorts.clear();
    _mementoRemovedMemberPorts.clear();
}

//extern "C" {
//#   include <opennsl/error.h>
//}

//#include <exception>

//Vlan::Vlan(const int asicUnit) noexcept
//    : _asicUnit { asicUnit } {
//    // Nothing more to do
//}

//Vlan::TaggingMode::TaggingMode(const bool isTagged, const bool isUntagged)
//    : tagged { isTagged }, untagged { isUntagged } {
//    // Nothing more to do
//}

//bool Vlan::addMemberPort(const opennsl_port_t port, const bool isTagged, const bool isUntagged) {
//    auto entry = _addMemberPorts.find(port);
//    if (_addMemberPorts.end() == entry) {
//        _addMemberPorts.emplace(port, TaggingMode{ isTagged, isUntagged });
//        return true;
//    }

//    auto& taggingMode = entry->second;
//    if ((isTagged == taggingMode.tagged) && (isUntagged == taggingMode.untagged)) {
//        return true;
//    }

//    taggingMode.tagged = isTagged;
//    taggingMode.untagged = isUntagged;

//    entry = _committedMemberPorts.find(port);
//    if (_addMemberPorts.end() != entry) {
//        _addMemberPorts.erase(entry);
//    }

//    return true;
//}

//bool Vlan::commitMemberPorts() {
//    opennsl_pbmp_t tagPortBitMap;
//    opennsl_pbmp_t untagPortBitMap;
//    OPENNSL_PBMP_CLEAR(tagPortBitMap);
//    OPENNSL_PBMP_CLEAR(untagPortBitMap);
//    for (const auto& port : _addMemberPorts) {
//        if (port.second.tagged) {
//            OPENNSL_PBMP_PORT_ADD(tagPortBitMap, port.first);
//        }

//        if (port.second.untagged) {
//            OPENNSL_PBMP_PORT_ADD(untagPortBitMap, port.first);
//        }
//    }

//    int rv = opennsl_vlan_port_add(_asicUnit, _id, tagPortBitMap, untagPortBitMap);
//    if (OPENNSL_FAILURE(rv)) {
//        ERROR_LOG(opennsl_errmsg(rv));
//        return false;
////        throw MAKE_OPENNSL_EXCEPTION(rv);
//    }

//    // Remove added port in _committedMemberPorts
//    // Program in ASIC
//    _addMemberPorts.clear();
//}

//void Vlan::execute() {
////    _savedPortTaggingMode.push()
//}

//void Vlan::undo() {
////    _addMemberPorts.erase(_savedPortTaggingMode.top().
////    _savedPortTaggingMode.pop()
//}

//VlanAddingMemberPort::VlanAddingMemberPort(const bool PortNo portNo, const bool tagged, const bool untagged) {

//}

//Result Vlan::remove() {
//    notifyAllObservers();
//}

////namespace Vlan {
//    Result VlanPortManager::addMemberPort(const PortNo portNo, const bool tagged, const bool untagged) {
//        // check if exists in _committedMemberPorts
//        // if yes, return Result::AlreadyExists

//        const auto containerSizeBeforeAdd = _addMemberPorts.size();
//        _addMemberPorts.emplace(portNo, TaggingMode{ tagged, untagged });
//        if (containerSizeBeforeAdd >= _addMemberPorts.size()) {
//            return Result::NoMemory;
////            std::string errorMsg {};
////            errorMsg.append("Failed to add VLAN ")
////                    .append(std::to_string(_vid))
////                    .append("member port no. ")
////                    .append(std::to_string(portNo));
////            throw std::runtime_error{ errorMsg };
//        }

//        return Result::Success;
//    }
//    //}






