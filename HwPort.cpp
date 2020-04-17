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

#include "HwPort.hpp"

#include "HwErrors.hpp"
#include "LoggingFacility.hpp"

extern "C" {
#   include <opennsl/error.h>
#   include <opennsl/l2.h>
#   include <opennsl/port.h>
#   include <opennsl/stg.h>
}

opennsl_port_t HwPort::Mapping::panelPortToHwPort(const PortId /* portNo */) {
    return 0;
}

PortId HwPort::Mapping::hwPortToPanelPort(const opennsl_port_t /* hwPort */) {
    return 0;
}

HwPort& HwPort::setPortParameters(const PortParameters parameters) {
    _parameters = parameters;
    return *this;
}

HwPortFdbFlushing& HwPortFdbFlushing::addPortToFlushing(const PortId portNo) {
    if (std::end(_portsToFlushing) == _portsToFlushing.find(portNo)) {
        _portsToFlushing.emplace(portNo);
    }

    return *this;
}

size_t HwPortFdbFlushing::getCommitOrderingResolve() const {
    return CommitOrderingResolve::FdbFlush;
}

Result::Value HwPortFdbFlushing::execute(ResultCallback::Handle& callback) {
     opennsl_module_t mod = -1;
     uint32 flags = 0;

    for (auto& portNo : _portsToFlushing) {
        opennsl_port_t hwPort = Mapping::panelPortToHwPort(portNo);
        auto rv = opennsl_l2_addr_delete_by_port(Asic::getDefaultHwUnit(), mod, hwPort, flags);
        CALL_CALLBACK_AND_RETURN_IF_OPENNSL_FAIL(rv, callback);
    }

    _portsToFlushing.clear();
    CALL_CALLBACK_AND_RETURN_RESULT_SUCCESS(callback);
}

size_t HwPortParametersSetting::getCommitOrderingResolve() const {
    return CommitOrderingResolve::PortSet;
}

Result::Value HwPortParametersSetting::execute(ResultCallback::Handle& callback) {
    int rc = OPENNSL_E_NONE;
    const opennsl_port_t hwPort = HwPort::Mapping::panelPortToHwPort(_parameters.portNo);
    opennsl_port_info_t bcm_pinfo;
    opennsl_port_info_t_init(&bcm_pinfo);

    // Whether we are enabling or disabling, we need to set the operational state (ENABLE) flag.
    bcm_pinfo.action_mask |= OPENNSL_PORT_ATTR_ENABLE_MASK;

    if (not _parameters.shutdowned) {
        DEBUG_LOG("Enabling hw_port=%d, autoneg=%d, duplex=%d",
                   hwPort, _parameters.autoneg, _parameters.fullDuplex);

        bcm_pinfo.enable = TRUE;

        /// @todo Set the max frame size if requested by the user.

        // We are enabling the port, so need to configure speed, duplex, pause, and autoneg as requested.
        /// @note half-duplex is not supported.
        if (_parameters.autoneg) {
            opennsl_port_ability_t port_ability;
            opennsl_port_ability_t advert_ability;

            opennsl_port_ability_t_init(&port_ability);
            opennsl_port_ability_t_init(&advert_ability);

            port_ability.fec = OPENNSL_PORT_ABILITY_FEC_NONE;
            /// @todo We only support advertising one speed or all possible speeds at the moment.
            if (PortSpeed::Max == _parameters.speed) {
                // Full autonegotiation desired.  Get all possible speed/duplex values for this port from h/w, then filter out HD support.
                rc = opennsl_port_ability_local_get(Asic::getDefaultHwUnit(), hwPort, &port_ability);
                if (OPENNSL_SUCCESS(rc)) {
                    // Mask out half-duplex support.
                    port_ability.speed_half_duplex = 0;
                } else {
                    VLOG_ERR("Failed to get port %d ability", hwPort);
                    // Assume typical 100G port setting. Can't be worse than just exiting...
                    port_ability.speed_full_duplex = OPENNSL_PORT_ABILITY_100GB;
                }
            }
            else if (PortSpeed::_1Gb == _parameters.speed) {
                port_ability.speed_full_duplex = OPENNSL_PORT_ABILITY_1000MB;
            }
            else if (PortSpeed::_10Gb == _parameters.speed) {
                port_ability.speed_full_duplex = OPENNSL_PORT_ABILITY_10GB;
            }
            else if (PortSpeed::_20Gb == _parameters.speed) {
                port_ability.speed_full_duplex = OPENNSL_PORT_ABILITY_20GB;
            }
            else if (PortSpeed::_25Gb == _parameters.speed) {
                port_ability.speed_full_duplex = OPENNSL_PORT_ABILITY_25GB;
            }
            else if (PortSpeed::_40Gb == _parameters.speed) {
                port_ability.speed_full_duplex = OPENNSL_PORT_ABILITY_40GB;
            }
            else if (PortSpeed::_50Gb == _parameters.speed) {
                port_ability.speed_full_duplex = OPENNSL_PORT_ABILITY_50GB;
            }
            else if (PortSpeed::_100Gb == _parameters.speed) {
                port_ability.speed_full_duplex = OPENNSL_PORT_ABILITY_100GB;
                if (_parameters.fec) {
                    port_ability.fec = OPENNSL_PORT_ABILITY_FEC_CL91;
                }
            } else {
                // Unsupported speed.
                VLOG_ERR("Failed to configure unavailable speed %d",
                         _parameters.speed);
                CALL_CALLBACK_AND_RETURN_RESULT_FAIL(callback);
            }

            rc = opennsl_port_ability_advert_get(Asic::getDefaultHwUnit(), hwPort, &advert_ability);
            if (OPENNSL_FAILURE(rc)) {
                VLOG_ERR("Failed to get port %d local advert", hw_port);
                // Assume typical advertised ability.
                advert_ability.pause = OPENNSL_PORT_ABILITY_PAUSE;
            }

            // Only change speed/duplex part of advertisement.
            advert_ability.speed_full_duplex = port_ability.speed_full_duplex;
            advert_ability.speed_half_duplex = port_ability.speed_half_duplex;

            // Update flow control (pause) advertisement.
            if (_parameters.rxPause) {
                advert_ability.pause |= OPENNSL_PORT_ABILITY_PAUSE_RX;
            } else {
                advert_ability.pause &= ~static_cast<unsigned int>(OPENNSL_PORT_ABILITY_PAUSE_RX);
            }

            if (_parameters.txPause) {
                advert_ability.pause |= OPENNSL_PORT_ABILITY_PAUSE_TX;
            } else {
                advert_ability.pause &= ~static_cast<unsigned int>(OPENNSL_PORT_ABILITY_PAUSE_TX);
            }

            bcm_pinfo.local_ability = advert_ability;
            bcm_pinfo.autoneg       = TRUE;
            bcm_pinfo.action_mask  |= (OPENNSL_PORT_ATTR_LOCAL_ADVERT_MASK |
                                       OPENNSL_PORT_ATTR_AUTONEG_MASK );
            bcm_pinfo.action_mask2 |= OPENNSL_PORT_ATTR2_PORT_ABILITY;

        } else {
            // Autoneg is not requested.
            bcm_pinfo.speed        = static_cast<int>(_parameters.speed);
            bcm_pinfo.duplex       = _parameters.fullDuplex ? OPENNSL_PORT_DUPLEX_FULL : OPENNSL_PORT_DUPLEX_HALF;
            bcm_pinfo.pause_rx     = _parameters.rxPause;
            bcm_pinfo.pause_tx     = _parameters.txPause;
            bcm_pinfo.autoneg      = FALSE;

            bcm_pinfo.action_mask |=   OPENNSL_PORT_ATTR_AUTONEG_MASK;
            bcm_pinfo.action_mask |= ( OPENNSL_PORT_ATTR_SPEED_MASK    |
                                       OPENNSL_PORT_ATTR_DUPLEX_MASK   |
                                       OPENNSL_PORT_ATTR_PAUSE_RX_MASK |
                                       OPENNSL_PORT_ATTR_PAUSE_TX_MASK );
        }

        /// @todo Configure interface type if it is a supported interface.
    // if (_parameters.shutdowned)
    } else {
        //For 100G support, need to remove autoneg, during no shut
        //Otherwise shut/no-shut wont work
        DEBUG_LOG("Disabling hw_port=%d", hwPort);
        bcm_pinfo.enable = 0;
        bcm_pinfo.autoneg = FALSE;
        bcm_pinfo.action_mask  |= OPENNSL_PORT_ATTR_AUTONEG_MASK;
    }

    // Program h/w with the given values.
    rc = opennsl_port_selective_set(Asic::getDefaultHwUnit(), hwPort, &bcm_pinfo);
    CALL_CALLBACK_AND_RETURN_IF_OPENNSL_FAIL(rc, callback);
    CALL_CALLBACK_AND_RETURN_RESULT_SUCCESS(callback);
}

size_t HwPortDefaultParametersSetting::getCommitOrderingResolve() const {
    return CommitOrderingResolve::PortInit;
}

Result::Value HwPortDefaultParametersSetting::execute(ResultCallback::Handle& callback) {
    opennsl_port_info_t portInfo {};
    opennsl_port_info_t_init(&portInfo);
    portInfo.speed = static_cast<int>(PortSpeed::Max);
    portInfo.duplex = OPENNSL_PORT_DUPLEX_FULL;
    portInfo.pause_rx = OPENNSL_PORT_ABILITY_PAUSE_RX;
    portInfo.pause_tx = OPENNSL_PORT_ABILITY_PAUSE_TX;
    portInfo.linkscan = OPENNSL_LINKSCAN_MODE_SW;
    portInfo.autoneg = FALSE;
    portInfo.enable = TRUE;
    portInfo.action_mask = OPENNSL_PORT_ATTR_AUTONEG_MASK
            | OPENNSL_PORT_ATTR_DUPLEX_MASK
            | OPENNSL_PORT_ATTR_PAUSE_TX_MASK
            | OPENNSL_PORT_ATTR_PAUSE_RX_MASK
            | OPENNSL_PORT_ATTR_LINKSCAN_MASK
            | OPENNSL_PORT_ATTR_ENABLE_MASK
            | OPENNSL_PORT_ATTR_SPEED_MASK;

    opennsl_port_config_t portConfig;
    opennsl_port_config_t_init(&portConfig);
    opennsl_port_config_get(Asic::getDefaultHwUnit(), &portConfig);
    opennsl_port_t port {};
    int rv = {};

    OPENNSL_PBMP_ITER(portConfig.e, port) { // Member .e contains all eth ports
        rv = opennsl_port_stp_set(Asic::getDefaultHwUnit(), port, OPENNSL_STG_STP_FORWARD);
        if (OPENNSL_FAILURE(rv)) {
            CALL_CALLBACK_AND_RETURN_RESULT_FAIL(callback);
        }

        rv = opennsl_port_selective_set(Asic::getDefaultHwUnit(), port, &portInfo);
        if (OPENNSL_FAILURE(rv)) {
            CALL_CALLBACK_AND_RETURN_RESULT_FAIL(callback);
        }
    }

    CALL_CALLBACK_AND_RETURN_RESULT_SUCCESS(callback);
}

size_t HwPortSpeedSetting::getCommitOrderingResolve() const {
    return CommitOrderingResolve::PortSet;
}

Result::Value HwPortSpeedSetting::execute(ResultCallback::Handle& callback) {
    const auto hwPort = Mapping::panelPortToHwPort(_parameters.portNo);
    opennsl_port_info_t portInfo {};
    opennsl_port_info_t_init(&portInfo);
    portInfo.action_mask = OPENNSL_PORT_ATTR_SPEED_MASK;
    portInfo.speed = static_cast<int>(_parameters.speed);
    int rc {};
    if (_parameters.autoneg) {
        opennsl_port_ability_t port_ability;
        opennsl_port_ability_t advert_ability;

        opennsl_port_ability_t_init(&port_ability);
        opennsl_port_ability_t_init(&advert_ability);

        port_ability.fec = OPENNSL_PORT_ABILITY_FEC_NONE;
        /// @todo We only support advertising one speed or all possible speeds at the moment.
        if (PortSpeed::Max == _parameters.speed) {
            // Full autonegotiation desired.  Get all possible speed/duplex values for this port from h/w, then filter out HD support.
            rc = opennsl_port_ability_local_get(Asic::getDefaultHwUnit(), hwPort, &port_ability);
            if (OPENNSL_SUCCESS(rc)) {
                // Mask out half-duplex support.
                port_ability.speed_half_duplex = 0;
            } else {
                VLOG_ERR("Failed to get port %d ability", hwPort);
                // Assume typical 100G port setting. Can't be worse than just exiting...
                port_ability.speed_full_duplex = OPENNSL_PORT_ABILITY_100GB;
            }
        }
        else if (PortSpeed::_1Gb == _parameters.speed) {
            port_ability.speed_full_duplex = OPENNSL_PORT_ABILITY_1000MB;
        }
        else if (PortSpeed::_10Gb == _parameters.speed) {
            port_ability.speed_full_duplex = OPENNSL_PORT_ABILITY_10GB;
        }
        else if (PortSpeed::_20Gb == _parameters.speed) {
            port_ability.speed_full_duplex = OPENNSL_PORT_ABILITY_20GB;
        }
        else if (PortSpeed::_25Gb == _parameters.speed) {
            port_ability.speed_full_duplex = OPENNSL_PORT_ABILITY_25GB;
        }
        else if (PortSpeed::_40Gb == _parameters.speed) {
            port_ability.speed_full_duplex = OPENNSL_PORT_ABILITY_40GB;
        }
        else if (PortSpeed::_50Gb == _parameters.speed) {
            port_ability.speed_full_duplex = OPENNSL_PORT_ABILITY_50GB;
        }
        else if (PortSpeed::_100Gb == _parameters.speed) {
            port_ability.speed_full_duplex = OPENNSL_PORT_ABILITY_100GB;
            if (_parameters.fec) {
                port_ability.fec = OPENNSL_PORT_ABILITY_FEC_CL91;
            }
        } else {
            // Unsupported speed.
            VLOG_ERR("Failed to configure unavailable speed %d",
                     _parameters.speed);
            CALL_CALLBACK_AND_RETURN_RESULT_FAIL(callback);
        }

        rc = opennsl_port_ability_advert_get(Asic::getDefaultHwUnit(), hwPort, &advert_ability);
        if (OPENNSL_FAILURE(rc)) {
            VLOG_ERR("Failed to get port %d local advert", hw_port);
            // Assume typical advertised ability.
            advert_ability.pause = OPENNSL_PORT_ABILITY_PAUSE;
        }

        portInfo.local_ability = advert_ability;
        portInfo.autoneg       = TRUE;
        portInfo.action_mask  |= (OPENNSL_PORT_ATTR_LOCAL_ADVERT_MASK |
                                  OPENNSL_PORT_ATTR_AUTONEG_MASK );
        portInfo.action_mask2 |= OPENNSL_PORT_ATTR2_PORT_ABILITY;
    }

    // Program h/w with the given values.
    rc = opennsl_port_selective_set(Asic::getDefaultHwUnit(), hwPort, &portInfo);
    CALL_CALLBACK_AND_RETURN_IF_OPENNSL_FAIL(rc, callback);
    CALL_CALLBACK_AND_RETURN_RESULT_SUCCESS(callback);
}

size_t HwPortShutdownSetting::getCommitOrderingResolve() const {
    return CommitOrderingResolve::PortInit;
}

Result::Value HwPortShutdownSetting::execute(ResultCallback::Handle& callback) {
    int rc = OPENNSL_E_NONE;
    const opennsl_port_t hwPort = HwPort::Mapping::panelPortToHwPort(_parameters.portNo);
    opennsl_port_info_t portInfo;
    opennsl_port_info_t_init(&portInfo);

    // Whether we are enabling or disabling, we need to set the operational state (ENABLE) flag.
    portInfo.action_mask |= OPENNSL_PORT_ATTR_ENABLE_MASK;
    if ((not _parameters.shutdowned) && _parameters.autoneg) {
        opennsl_port_ability_t port_ability;
        opennsl_port_ability_t advert_ability;

        opennsl_port_ability_t_init(&port_ability);
        opennsl_port_ability_t_init(&advert_ability);

        port_ability.fec = OPENNSL_PORT_ABILITY_FEC_NONE;
        /// @todo We only support advertising one speed or all possible speeds at the moment.
        if (PortSpeed::Max == _parameters.speed) {
            // Full autonegotiation desired.  Get all possible speed/duplex values for this port from h/w, then filter out HD support.
            rc = opennsl_port_ability_local_get(Asic::getDefaultHwUnit(), hwPort, &port_ability);
            if (OPENNSL_SUCCESS(rc)) {
                // Mask out half-duplex support.
                port_ability.speed_half_duplex = 0;
            } else {
                VLOG_ERR("Failed to get port %d ability", hwPort);
                // Assume typical 100G port setting. Can't be worse than just exiting...
                port_ability.speed_full_duplex = OPENNSL_PORT_ABILITY_100GB;
            }
        }
        else if (PortSpeed::_1Gb == _parameters.speed) {
            port_ability.speed_full_duplex = OPENNSL_PORT_ABILITY_1000MB;
        }
        else if (PortSpeed::_10Gb == _parameters.speed) {
            port_ability.speed_full_duplex = OPENNSL_PORT_ABILITY_10GB;
        }
        else if (PortSpeed::_20Gb == _parameters.speed) {
            port_ability.speed_full_duplex = OPENNSL_PORT_ABILITY_20GB;
        }
        else if (PortSpeed::_25Gb == _parameters.speed) {
            port_ability.speed_full_duplex = OPENNSL_PORT_ABILITY_25GB;
        }
        else if (PortSpeed::_40Gb == _parameters.speed) {
            port_ability.speed_full_duplex = OPENNSL_PORT_ABILITY_40GB;
        }
        else if (PortSpeed::_50Gb == _parameters.speed) {
            port_ability.speed_full_duplex = OPENNSL_PORT_ABILITY_50GB;
        }
        else if (PortSpeed::_100Gb == _parameters.speed) {
            port_ability.speed_full_duplex = OPENNSL_PORT_ABILITY_100GB;
            if (_parameters.fec) {
                port_ability.fec = OPENNSL_PORT_ABILITY_FEC_CL91;
            }
        } else {
            // Unsupported speed.
            VLOG_ERR("Failed to configure unavailable speed %d",
                     _parameters.speed);
            CALL_CALLBACK_AND_RETURN_RESULT_FAIL(callback);
        }

        rc = opennsl_port_ability_advert_get(Asic::getDefaultHwUnit(), hwPort, &advert_ability);
        if (OPENNSL_FAILURE(rc)) {
            VLOG_ERR("Failed to get port %d local advert", hw_port);
            // Assume typical advertised ability.
            advert_ability.pause = OPENNSL_PORT_ABILITY_PAUSE;
        }

        portInfo.local_ability = advert_ability;
        portInfo.autoneg       = TRUE;
        portInfo.action_mask  |= (OPENNSL_PORT_ATTR_LOCAL_ADVERT_MASK |
                                  OPENNSL_PORT_ATTR_AUTONEG_MASK );
        portInfo.action_mask2 |= OPENNSL_PORT_ATTR2_PORT_ABILITY;
    }

    // Program h/w with the given values.
    rc = opennsl_port_selective_set(Asic::getDefaultHwUnit(), hwPort, &portInfo);
    CALL_CALLBACK_AND_RETURN_IF_OPENNSL_FAIL(rc, callback);
    CALL_CALLBACK_AND_RETURN_RESULT_SUCCESS(callback);
}
