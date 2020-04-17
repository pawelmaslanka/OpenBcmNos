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

#include "Asic.hpp"
#include "HwPort.hpp"
#include "PortManager.hpp"
#include "LoggingFacility.hpp"

PortManager::PortManager()
    : Observer({ UpdateReason::LinkStatusUpdate }),
      _hwPortCommandFactory { std::make_shared<HwPortCommandFactory>() },
      _hwPortLinkScanHandling { std::make_shared<HwPortLinkScanHandling>() },
      _hwPortModuleInitializing { std::make_shared<HwPortModuleInitializing>(_hwPortLinkScanHandling) } {
    // Nothing more to do
}

Result::Value PortManager::init() {
    std::shared_ptr<Observer> meAsObserver { shared_from_this() };
    _hwPortLinkScanHandling->addObserver(meAsObserver);
    return HwPortModuleInitializing(_hwPortLinkScanHandling).execute();
}

void PortManager::update(const ObservedSubjectHandle& subject, const UpdateReason updateReason) {
    switch (updateReason) {
      case UpdateReason::LinkStatusUpdate: {
        const auto hwPortLinkScanHandler = std::dynamic_pointer_cast<HwPortLinkScanHandling const>(subject);
        if (not hwPortLinkScanHandler) {
            ERROR_LOG("Got link scan update from unknown source");
            return;
        }

        const auto& recentlyLinkStatusChangedOnPorts { hwPortLinkScanHandler->getRecentlyLinkStatusChangedOnPorts() };
        for (const auto& portLinkStatus : recentlyLinkStatusChangedOnPorts) {
            const PortId portNo = portLinkStatus.first;
            const bool linkedUp = portLinkStatus.second;
            _portsLinkStatus.insert_or_assign(portNo, linkedUp);
            if (_configured.find(portNo) != std::end(_configured)) {
                if (auto port = getHandle(portNo).lock()) { // Has to be copied into a shared_ptr before usage
                    port->setLinkStatus(linkedUp);
                }
            }
        }

        break;
      }

      default: {
        return;
      }
    }
}

ObserverId PortManager::hash() {
    return static_cast<ObserverId>(ObserverIdentifier::PortManager);
}

Result::Value PortManager::execute(ResultCallback::Handle& callback) {
    CALL_CALLBACK_AND_RETURN_RESULT_IF_FAIL(CommandManager::execute(callback), callback);

    for (const auto& portNo : _mementoAdded) {
        if (auto portPtr = getHandle(portNo).lock()) {
            portPtr->setHwPortCommandFactory(_hwPortCommandFactory);
        }
    }

    CALL_CALLBACK_AND_RETURN_RESULT_SUCCESS(callback);
}

PortSettingExecutor::PortSettingExecutor(PortManager::Handle& portManager, const PortId portNo)
    : _portManager { portManager }, _portNo { portNo }, _executed { false } {
    PortSettingMemento::Handle nullPortSettingMemento = std::make_shared<NullPortSettingMemento>();
    setPortSettingMemento(nullPortSettingMemento);
    PortSettingProcessing::Handle nullPortSettingProcessing = std::make_shared<NullPortSettingProcessing>();
    setPortSettingProcessing(nullPortSettingProcessing);
}

Result::Value PortSettingExecutor::execute(ResultCallback::Handle& callback) {
    if (not _portManager->exists(_portNo)) {
        CALL_CALLBACK_AND_RETURN_RESULT_IF_FAIL(Result::Value::PortNotExists, callback);
    }

    if (auto portPtr = _portManager->getHandle(_portNo).lock()) {
        createMemento(*portPtr);
        CALL_CALLBACK_AND_RETURN_RESULT_IF_FAIL(_portSettingProcessing->processCommand(*portPtr), callback);
        _executed = true;
        CALL_CALLBACK_AND_RETURN_RESULT_SUCCESS(callback);
    }

    CALL_CALLBACK_AND_RETURN_RESULT_IF_FAIL_OR_SUCCESS(Result::Value::PortNotExists, callback);
}

Result::Value PortSettingExecutor::undo(ResultCallback::Handle& callback) {
    if (not _executed) {
        CALL_CALLBACK_AND_RETURN_RESULT_IF_FAIL(Result::Value::CommandNotUndoable, callback);
    }

    if (not _portManager->exists(_portNo)) {
        CALL_CALLBACK_AND_RETURN_RESULT_IF_FAIL(Result::Value::PortNotExists, callback);
    }

    if (auto portPtr = _portManager->getHandle(_portNo).lock()) {
        CALL_CALLBACK_AND_RETURN_RESULT_IF_FAIL(setMemento(*portPtr), callback);
        _executed = false;
        CALL_CALLBACK_AND_RETURN_RESULT_SUCCESS(callback);
    }

    CALL_CALLBACK_AND_RETURN_RESULT_IF_FAIL_OR_SUCCESS(Result::Value::PortNotExists, callback);
}

void PortSettingExecutor::setPortSettingMemento(PortSettingMemento::Handle& portSettingMemento) {
    _portSettingMemento = portSettingMemento;
}

void PortSettingExecutor::setPortSettingProcessing(PortSettingProcessing::Handle& portSettingProcessing) {
    _portSettingProcessing = portSettingProcessing;
}

#define PORT_SETTING_MEMENTO_AND_PROCESSING_INIT()                                  \
    PortSettingMemento::Handle portSettingMementoHandle = shared_from_this();       \
    setPortSettingMemento(portSettingMementoHandle);                                \
    PortSettingProcessing::Handle portSettingProcessingHandle = shared_from_this(); \
    setPortSettingProcessing(portSettingProcessingHandle)

PortCommandImplement::PortCommandImplement(PortManager::Handle& portManager, const PortId portNo)
    : PortSettingExecutor(portManager, portNo) {
    // Nothing more to do
}

PortShutdownSetting::PortShutdownSetting(PortManager::Handle& portManager, const PortId portNo, const bool disable)
    : PortCommandImplement{ portManager, portNo }, _disable { disable }, _disableMemento {} {
    PORT_SETTING_MEMENTO_AND_PROCESSING_INIT();
}

size_t PortShutdownSetting::getCommitOrderingResolve() const {
    return CommitOrderingResolve::PortSet;
}

void PortShutdownSetting::createMemento(Port& port) {
    _disableMemento = port.isShutdowned();
}

Result::Value PortShutdownSetting::setMemento(Port& port) {
    return port.shutdown(_disableMemento);
}

Result::Value PortShutdownSetting::processCommand(Port& port) {
    return port.shutdown(_disable);
}

PortSpeedSetting::PortSpeedSetting(PortManager::Handle& portManager, const PortId portNo, const PortSpeed speed)
    : PortCommandImplement{ portManager, portNo }, _speed { speed }, _speedMemento { PortSpeed::Max } {
    PORT_SETTING_MEMENTO_AND_PROCESSING_INIT();
}

size_t PortSpeedSetting::getCommitOrderingResolve() const {
    return CommitOrderingResolve::PortSet;
}
