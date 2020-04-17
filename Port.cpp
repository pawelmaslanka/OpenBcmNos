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
#include "Port.hpp"

Port::Port(const PortId portNo)
    : Observer { {UpdateReason::LinkStatusUpdate} }, _parameters { } {
    _parameters.portNo = portNo;
}

Port::~Port() {
    destroy();
}

ObserverId Port::hash() {
    return static_cast<ObserverId>(ObserverIdentifier::Port);
}

void Port::update(const ObservedSubjectHandle&, const UpdateReason) {

}

Result::Value Port::create() {
    /// @todo Create KNET interface
    auto result = _hwPortCommandFactory->getDefaultParametersSettingCmd()->execute();
    return result;
}

Result::Value Port::destroy() {
    if (not _created) {
        return Result::Value::Success;
    }

    _hwPortCommandFactory->getFdbFlushingCmd()->addPortToFlushing(_parameters.portNo).execute();
    _created = false;
    return Result::Value::Success;
}

bool Port::isShutdowned() const {
    return _parameters.shutdowned;
}

Result::Value Port::shutdown(const bool disable) {
    if ((disable && _parameters.shutdowned)
            || ((not disable) && (not _parameters.shutdowned))) {
        return Result::Value::Success;
    }

    auto parameters = _parameters;
    parameters.shutdowned = disable;
    const auto result = _hwPortCommandFactory->getHwPortShutdownSettingCmd()->setPortParameters(parameters).execute();
    if (not Result::Failed(result)) {
        _parameters.shutdowned = disable;
    }

    return result;
}

bool Port::isOperable() const {
    return _created && not _parameters.shutdowned && _linkedUp;
}

void Port::setHwPortCommandFactory(HwPortCommandFactory::Handle& hwPortCommandFactory) {
    _hwPortCommandFactory = hwPortCommandFactory;
}

PortSpeed Port::getSpeed() const {
    return _parameters.speed;
}

Result::Value Port::setSpeed(const PortSpeed speed) {
    auto parameters = _parameters;
    parameters.speed = speed;
    const auto result = _hwPortCommandFactory->getSpeedSettingCmd()->setPortParameters(parameters).execute();
    if (not Result::Failed(result)) {
        _parameters.speed = speed;
    }

    return result;
}

void Port::setLinkStatus(const bool linkedUp) {
    _linkedUp = linkedUp;
    if (not linkedUp) {
        _hwPortCommandFactory->getFdbFlushingCmd()->addPortToFlushing(_parameters.portNo).execute();
    }
}
