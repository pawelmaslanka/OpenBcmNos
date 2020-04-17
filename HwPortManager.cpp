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

#include "HwPortManager.hpp"

extern "C" {
#include <opennsl/error.h>
}

#include <functional>

HwPortCommandFactory::HwPortCommandFactory()
    : _defaultParametersSetting { std::make_shared<HwPortDefaultParametersSetting>() },
      _fdbFlushingCmd { std::make_shared<HwPortFdbFlushing>() },
      _parametersSetting { std::make_shared<HwPortParametersSetting>() },
      _speedSettingCmd { std::make_shared<HwPortSpeedSetting>() } {
    // Nothing more to do
}

static std::mutex gRecentlyLinkStatusChangedOnHwPortsMtx;
static std::mutex gRecentlyLinkStatusChangedOnPortsMtx;
static std::condition_variable gRecentlyLinkStatusChangedOnHwPortsCondVar;

template <typename T>
struct Callback;

template <typename Ret, typename... Params>
struct Callback<Ret(Params...)> {
    template <typename... Args>
    static Ret callback(Args... args) { return func(args...); }
    static std::function<Ret(Params...)> func;
};

// Initialize the static member.
template <typename Ret, typename... Params>
std::function<Ret(Params...)> Callback<Ret(Params...)>::func;

HwPortLinkScanHandling::HwPortLinkScanHandling()
    : _linkStatusHwPortsUpdated { false } {
    // Store member function and the instance using std::bind
     Callback<void(int, opennsl_port_t, opennsl_port_info_t*)>::func = std::bind(&HwPortLinkScanHandling::portLinkStatusUpdate, *this,
                                                                                 std::placeholders::_1, std::placeholders::_2,
                                                                                 std::placeholders::_3);
     // Convert callback-function to c-pointer
     _linkScanHandler = static_cast<decltype(_linkScanHandler)>(Callback<void(int, opennsl_port_t, opennsl_port_info_t*)>::callback);
}

void HwPortLinkScanHandling::portLinkStatusUpdate(int unit, opennsl_port_t port, opennsl_port_info_t* info) {
    if (Asic::getDefaultHwUnit() != unit) {
        return;
    }
    // Save physical port link status for use later.
    // Also update VLAN membership configuration.
    std::unique_lock<std::mutex> mtx(gRecentlyLinkStatusChangedOnHwPortsMtx);
    _recentlyLinkStatusChangedOnHwPorts.insert_or_assign(port, OPENNSL_PORT_LINK_STATUS_UP == info->linkstatus);
    _linkStatusHwPortsUpdated = true;
    mtx.unlock();

    gRecentlyLinkStatusChangedOnHwPortsCondVar.notify_one();

}

void HwPortLinkScanHandling::changeLinkStatusOnHwPortsNotifier() {
    while (true) {
        std::map<opennsl_port_t, bool> currLinkStatusOnHwPorts {};
        std::unique_lock<std::mutex> mlock(gRecentlyLinkStatusChangedOnHwPortsMtx);
        gRecentlyLinkStatusChangedOnHwPortsCondVar.wait(mlock, std::bind(&HwPortLinkScanHandling::_linkStatusHwPortsUpdated, this));
        currLinkStatusOnHwPorts.swap(_recentlyLinkStatusChangedOnHwPorts);
        _linkStatusHwPortsUpdated = false;
        mlock.unlock();

        for (auto& portLinkStatus : currLinkStatusOnHwPorts) {
            PortId portNo;
            /// @todo portNo = Port::Mapping::HwPortToPanelPort(portLinkStatus.first);
            _recentlyLinkStatusChangedOnPorts.insert_or_assign(portNo, portLinkStatus.second);
        }

        notifyAllObservers(UpdateReason::LinkStatusUpdate);
        _recentlyLinkStatusChangedOnPorts.clear();
    }
}

size_t HwPortLinkScanHandling::getCommitOrderingResolve() const {
    return CommitOrderingResolve::Unordered;
}

Result::Value HwPortLinkScanHandling::execute(ResultCallback::Handle& /* callback */) {
    return Result::Value::Fail;
}

HwPortModuleInitializing::HwPortModuleInitializing(HwPortLinkScanHandling::Handle& linkScanHandle)
    : _linkScanHandle { linkScanHandle } {
    // Nothing more to do
}

size_t HwPortModuleInitializing::getCommitOrderingResolve() const {
    return CommitOrderingResolve::Unordered;
}

Result::Value HwPortModuleInitializing::execute(ResultCallback::Handle& callback) {
    opennsl_port_t hw_port {};
    opennsl_port_config_t pcfg {};
    int rc = OPENNSL_E_NONE;
    // Update CPU port's L2 learning behavior to forward frames with
    // unknown src MACs.  This is the way it's always been, but the
    // default changed somehow when we upgraded from SDK-5.6.2 to
    // 5.9.0.  See Broadcom support case #382115.
    rc = opennsl_port_control_set(Asic::getDefaultHwUnit(),
                                  Asic::getCpuPort(Asic::getDefaultHwUnit()),
                                  opennslPortControlL2Move,
                                  OPENNSL_PORT_LEARN_FWD);
    if (OPENNSL_FAILURE(rc)) {
        VLOG_ERR("CPU L2 setting failed! err=%d (%s)",
                 rc, opennsl_errmsg(rc));
        callback->onCommandResult(Result::Value::Fail);
        return Result::Value::Fail;
    }

    // Register for link state change notifications.
    // Note that all ports come up by default in a disabled
    // state.  So until intfd is ready to enable the ports,
    // we should not get any callbacks.
    rc = opennsl_linkscan_register(Asic::getDefaultHwUnit(), _linkScanHandle->getLinkScanCallback());
    if (OPENNSL_FAILURE(rc)) {
        VLOG_ERR("Linkscan registration error, err=%d (%s)",
                 rc, opennsl_errmsg(rc));
        callback->onCommandResult(Result::Value::Fail);
        return Result::Value::Fail;
    }

    // Enable both ingress and egress VLAN filtering mode
    // for all Ethernet interfaces defined in the system.
    // Clear the stats for all Ethernet interfaces during initialization
    // This improvement is necessary for AS7712
    if (OPENNSL_SUCCESS(opennsl_port_config_get(Asic::getDefaultHwUnit(), &pcfg))) {
        OPENNSL_PBMP_ITER(pcfg.e, hw_port) {
            rc = opennsl_port_vlan_member_set(Asic::getDefaultHwUnit(), hw_port,
                                              (OPENNSL_PORT_VLAN_MEMBER_INGRESS |
                                               OPENNSL_PORT_VLAN_MEMBER_EGRESS));
            if (OPENNSL_FAILURE(rc)) {
                VLOG_ERR("Failed to set unit %d hw_port %d VLAN filter "
                         "mode, err=%d (%s)",
                         hw_unit, hw_port, rc, opennsl_errmsg(rc));
            }
            rc = opennsl_stat_clear(Asic::getDefaultHwUnit(), hw_port);
            if (OPENNSL_FAILURE(rc)) {
                VLOG_ERR("Failed to clear stat unit %d hw_port %d "
                         "err=%d (%s)",
                         hw_unit, hw_port, rc, opennsl_errmsg(rc));
            }
        }
    } else {
        VLOG_ERR("Failed to get switch port configuration");
        callback->onCommandResult(Result::Value::Fail);
        return Result::Value::Fail;
    }

    callback->onCommandResult(Result::Value::Success);
    return Result::Value::Success;
}
