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

#include "HwPort.hpp"

class HwPortManager : public Command, public ObservedSubject {
  public:
    virtual ~HwPortManager() = default;
};

class HwPortLinkScanHandling : public HwPort, public ObservedSubject {
  public:
    using Handle = std::shared_ptr<HwPortLinkScanHandling>;
    HwPortLinkScanHandling();
    virtual ~HwPortLinkScanHandling() override = default;
    inline opennsl_linkscan_handler_t getLinkScanCallback() const;
    inline std::map<PortId, bool> getRecentlyLinkStatusChangedOnPorts() const;
    virtual size_t getCommitOrderingResolve() const override;
    virtual Result::Value execute(ResultCallback::Handle& callback = gNullResultCallback) override;

  private:
    void portLinkStatusUpdate(int unit, opennsl_port_t port, opennsl_port_info_t* info);
    /// We need keep notifies observers about link status update in separated thread,
    /// because Broadcom linkscan callback is called in SDK thread so we can't stuck
    /// in handling observer's callbacks.
    void changeLinkStatusOnHwPortsNotifier();
    opennsl_linkscan_handler_t _linkScanHandler;
    bool _linkStatusHwPortsUpdated;
    std::map<opennsl_port_t, bool> _recentlyLinkStatusChangedOnHwPorts;
    std::map<PortId, bool> _recentlyLinkStatusChangedOnPorts;
};

opennsl_linkscan_handler_t HwPortLinkScanHandling::getLinkScanCallback() const { return _linkScanHandler; }

std::map<PortId, bool> HwPortLinkScanHandling::getRecentlyLinkStatusChangedOnPorts() const {
    return _recentlyLinkStatusChangedOnPorts;
}

class HwPortModuleInitializing final : public HwPort {
  public:
    using Handle = std::shared_ptr<HwPortModuleInitializing>;
    HwPortModuleInitializing(HwPortLinkScanHandling::Handle& linkScanHandle);
    virtual ~HwPortModuleInitializing() override = default;
    virtual size_t getCommitOrderingResolve() const override;
    virtual Result::Value execute(ResultCallback::Handle& callback = gNullResultCallback) override;

  private:
    HwPortLinkScanHandling::Handle _linkScanHandle;
};

class HwPortCommandFactory {
  public:
    using Handle = std::shared_ptr<HwPortCommandFactory>;
    HwPortCommandFactory();
    inline HwPortDefaultParametersSetting::Handle& getDefaultParametersSettingCmd();
    inline HwPortFdbFlushing::Handle& getFdbFlushingCmd();
    inline HwPortParametersSetting::Handle& getParametersSettingCmd();
    inline HwPortShutdownSetting::Handle& getHwPortShutdownSettingCmd();
    inline HwPortSpeedSetting::Handle& getSpeedSettingCmd();

  private:
    HwPortDefaultParametersSetting::Handle _defaultParametersSetting;
    HwPortFdbFlushing::Handle _fdbFlushingCmd;
    HwPortParametersSetting::Handle _parametersSetting;
    HwPortShutdownSetting::Handle _shutdownSetting;
    HwPortSpeedSetting::Handle _speedSettingCmd;
};

HwPortDefaultParametersSetting::Handle& HwPortCommandFactory::getDefaultParametersSettingCmd() { return _defaultParametersSetting; }
HwPortFdbFlushing::Handle& HwPortCommandFactory::getFdbFlushingCmd() { return _fdbFlushingCmd; }
HwPortParametersSetting::Handle& HwPortCommandFactory::getParametersSettingCmd() { return _parametersSetting; }
HwPortShutdownSetting::Handle& HwPortCommandFactory::getHwPortShutdownSettingCmd() { return _shutdownSetting; }
HwPortSpeedSetting::Handle& HwPortCommandFactory::getSpeedSettingCmd() { return _speedSettingCmd; }
