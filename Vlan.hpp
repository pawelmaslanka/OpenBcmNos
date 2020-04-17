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

#pragma once

#include "Types.hpp"
#include "Command.hpp"
#include "HwCommandProcessor.hpp"
#include "Observer.hpp"

extern "C" {
#   include <opennsl/vlan.h>
}

#include <map>
#include <set>
#include <stack>
#include <vector>

class VlanObserver {
  public:
    using Handle = std::shared_ptr<VlanObserver>;
    virtual ~VlanObserver() = default;
    virtual ObserverId Id() const = 0;
    virtual void update(ObservedSubjectHandle& subject) = 0;
};

class VlanSubject {
  public:
    using Handle = std::shared_ptr<VlanSubject>;
    virtual ~VlanSubject() = default;
    void addObserver(VlanObserver::Handle& observerToAdd);
    void removeObserver(VlanObserver::Handle& observerToRemove);

  protected:
    void notifyAllObservers() const;

  private:
    std::vector<Observer::Handle> _observers;
};

class Vlan final : public ObservedSubject, public DependencySource {
  public:
    using Handle = std::weak_ptr<Vlan>;
    using Id = uint16_t;
    Vlan(const Id vid);
    ~Vlan();
    Id id();
    Result::Value create();
    Result::Value destroy();
    void addVlanCreationObserver(ObservedSubjectHandle& observer);
    void addVlanDeletionObserver(ObservedSubjectHandle& observer);

  private:
    void notifyAboutCreatedVlan();
    void notifyAboutDestroyedVlan();
    std::set<MemberPortOwnerHandle> _memberPorts;
    Id _vid;
    bool _created;
};

struct VlanCompare {
   bool operator() (const Vlan::Handle& lhs, const Vlan::Handle& rhs) const {
       if (auto lhsShared = lhs.lock()) { // Has to be copied into a shared_ptr before usage
           if (auto rhsShared = rhs.lock()) {
               return lhsShared->id() < rhsShared->id();
           }

           return true;
       }

       return false;
   }
};

struct TaggingMode {
    TaggingMode(const bool isTagged = false, const bool isUntagged = true);
    bool tagged;
    bool untagged;
};
/// Maps front panel port into its tagging mode in VLAN.
/// If tagging mode is equal to 'true' it means that port is tagged.
/// Otherwise if tagging mode is equal to 'false' it means that port is untagged.
using PortTaggingModeMap = std::map<PortId, TaggingMode>;
using PortTaggingModeMapHandle = std::shared_ptr<PortTaggingModeMap>;

class ManagingVlanMemberPorts : public UndoableCommand {
  public:
    using Handle = std::shared_ptr<ManagingVlanMemberPorts>;
    ManagingVlanMemberPorts(VlanId vlan, Command::ResultCallback::Handle& operationCallback);
    virtual ~ManagingVlanMemberPorts() = default;
    inline VlanId getVlanId() const;

    virtual void execute(Command::ResultCallback::Handle callback) = 0;
    virtual void undo(Command::ResultCallback::Handle callback) = 0;
    virtual size_t getCommitOrderingResolve() const noexcept = 0;

  protected:
    VlanId _vid;
    Command::ResultCallback::Handle _operationCallback;
};

VlanId ManagingVlanMemberPorts::getVlanId() const { return _vid; }

class AddingVlanMemberPorts : public ManagingVlanMemberPorts, public ObservedSubject, public DependentUser {
  public:
    using Handle = std::shared_ptr<AddingVlanMemberPorts>;
    AddingVlanMemberPorts(VlanId vlan, Command::ResultCallback::Handle callback);
    virtual ~AddingVlanMemberPorts() override = default;
    virtual Result addVlanMemberPorts(const PortId portNo, const TaggingMode taggingMode);

    virtual void execute(Command::ResultCallback::Handle callback) override;
    virtual void undo(Command::ResultCallback::Handle callback) override;
    virtual size_t getCommitOrderingResolve() const noexcept override;

  private:
    PortTaggingModeMapHandle _ports;
};

class RemovingVlanMemberPorts : public ManagingVlanMemberPorts {
  public:
    using Handle = std::shared_ptr<RemovingVlanMemberPorts>;
    RemovingVlanMemberPorts(VlanId vid, Command::ResultCallback::Handle callback);
    virtual ~RemovingVlanMemberPorts() override = default;
    virtual Result removeVlanMemberPorts(const PortId portNo);

    virtual void execute(Command::ResultCallback::Handle callback) override;
    virtual void undo(Command::ResultCallback::Handle callback) override;
};

class VlanMemberPortManager : public Observer, public UndoableCommand, public Command::ResultCallback, public std::enable_shared_from_this<VlanMemberPortManager> {
  public:
    using Handle = std::shared_ptr<VlanMemberPortManager>;
    VlanMemberPortManager(VlanManager::Handle vlanManager, HwCommandProcessor::Handle hwCommandProcessor, Vlan::Handle vlan);
    Result addMemberPort(Vlan::Handle& vlan, const PortId portNo, const bool tagged, const bool untagged);
    Result removeMemberPort(Vlan::Handle& vlan, const PortId portNo);
    virtual void execute() override;
    virtual void undo() override;

    void onRemoveEvent();
    Result commit() {
       auto ports = std::make_shared<PortTaggingModeMap>(_addMemberPorts);
        for (auto& addingVlanMemberPortsHandle : _addingVlanMemberPortsHandles) {
            if (Failed(addingVlanMemberPortsHandle->addVlanMemberPorts(port))) {
                // Check which one port left in ports and pass them to RemovingVlanMemberPortsHandle
                return Result::Fail;
            }
        }
    }

    void registerOperationOnVlanMemberPortsHandle(ManagingVlanMemberPorts::Handle handle) {
        _addingVlanMemberPortsHandles.push_back(handle);
    }

    Result commitAddVlanMemberPorts() {
        _hwCommandProcessor->addCommandToExecute(_addingVlanMemberPorts);
    }

    Result commitRemoveVlanMemberPorts() {
        _hwCommandProcessor->addCommandToExecute(_removingVlanMemberPorts);
    }

    virtual void onManageVlanMemberPortsEvent(const Result result, const std::string msg) {
      if (not Failed(result)) {
          _commitCompleted = true;
          return;
      }

//        std::cout << "Failed to add member ports to VLAN " << _vlan->getId() << ": " << error << std::endl;
    }

  private:
    VlanManager::Handle _vlanManager;
    std::map<Vlan::Handle, PortTaggingModeMap, VlanCompare> _committedMemberPorts;
    std::map<Vlan::Handle, PortTaggingModeMap, VlanCompare> _addMemberPorts;
    std::map<Vlan::Handle, std::set<PortId>, VlanCompare> _removeMemberPorts;
    std::map<Vlan::Handle, std::set<PortId>, VlanCompare> _mementoAddedMemberPorts;
    std::map<Vlan::Handle, PortTaggingModeMap, VlanCompare> _mementoRemovedMemberPorts;
    std::map<Vlan::Handle, AddingVlanMemberPorts, VlanCompare> _addingVlanMemberPortsHandles;
    std::map<Vlan::Handle, RemovingVlanMemberPorts, VlanCompare> _removingVlanMemberPortsHandles;
    HwCommandProcessor::Handle _hwCommandProcessor;
    Command::ResultCallback::Handle _nullResultCallback;
    bool _commitCompleted;
};

VlanMemberPortManager::VlanMemberPortManager(Vlan::Handle vlan) noexcept : _vlan { vlan }, _commitCompleted { false } { }

class HwVlanPortManager : public ManagingVlanMemberPorts {
  public:
    void registerVlanPortManager(VlanMemberPortManager::Handle& vlanPortManager) {
        vlanPortManager->registerAddingVlanMemberPortsHandle(_instance);
    }

    virtual Result addVlanMemberPort(const Vlan::Handle vlan, PortTaggingModeMapHandle ports) override;

  private:
    Handle _instance = std::make_shared<AddingVlanMemberPorts>();
};


