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

#include "Asic.hpp"
#include "Command.hpp"
#include "Observer.hpp"

#include <condition_variable>
#include <memory>
#include <mutex>
#include <thread>

extern "C" {
#   include <opennsl/link.h>
#   include <opennsl/port.h>
}

class HwPort : public Command {
  public:
    using Handle = std::shared_ptr<HwPort>;
    virtual ~HwPort() = default;
    HwPort& setPortParameters(const PortParameters parameters);
    struct Mapping {
        static opennsl_port_t panelPortToHwPort(const PortId portNo);
        static PortId hwPortToPanelPort(const opennsl_port_t hwPort);
    };

  protected:
    PortParameters _parameters;
};

/// Lag and Vlan port will have separated class of FDB flushing command
class HwPortFdbFlushing : public HwPort {
  public:
    using Handle = std::shared_ptr<HwPortFdbFlushing>;
    virtual ~HwPortFdbFlushing() override = default;
    HwPortFdbFlushing& addPortToFlushing(const PortId portNo);
    Result::Value removePortFromFlushing(const PortId portNo);
    virtual size_t getCommitOrderingResolve() const override;
    virtual Result::Value execute(ResultCallback::Handle& callback = gNullResultCallback) override;

  private:
    std::set<PortId> _portsToFlushing; // ports from which learned MACs should be cleared
};

class HwPortShutdownSetting : public HwPort {
  public:
    using Handle = std::shared_ptr<HwPortShutdownSetting>;
    virtual size_t getCommitOrderingResolve() const override;
    virtual Result::Value execute(ResultCallback::Handle& callback = gNullResultCallback) override;
};

class HwPortSpeedSetting : public HwPort {
  public:
    using Handle = std::shared_ptr<HwPortSpeedSetting>;
    virtual size_t getCommitOrderingResolve() const override;
    virtual Result::Value execute(ResultCallback::Handle& callback = gNullResultCallback) override;
};

class HwPortDefaultParametersSetting : public HwPort {
  public:
    using Handle = std::shared_ptr<HwPortDefaultParametersSetting>;
    virtual size_t getCommitOrderingResolve() const override;
    virtual Result::Value execute(ResultCallback::Handle& callback = gNullResultCallback) override;
};

class HwPortParametersSetting : public HwPort {
  public:
    using Handle = std::shared_ptr<HwPortParametersSetting>;
    HwPortParametersSetting& setPortParameters(const PortParameters& parameters);
    virtual size_t getCommitOrderingResolve() const override;
    virtual Result::Value execute(ResultCallback::Handle& callback = gNullResultCallback) override;
};
