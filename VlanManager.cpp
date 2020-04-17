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

#include "VlanManager.hpp"

#define OPS_VLAN_MIN       0
#define OPS_VLAN_MAX       4095
#define OPS_VLAN_COUNT     (OPS_VLAN_MAX - OPS_VLAN_MIN + 1)
#define OPS_VLAN_VALID(v)  ((v)>OPS_VLAN_MIN && (v)<OPS_VLAN_MAX)

VlanManager::VlanManager(PortManager::Handle& portManager, LagManager::Handle& lagManager)
    : _portManager { portManager }, _lagManager { lagManager } {
    /* Nothing more to do */
}

void
vlan_reconfig_on_link_change(int unit, opennsl_port_t hw_port, int link_is_up)
{
    int vid, count;
    opennsl_pbmp_t pbm;
    ops_vlan_data_t *vlanp;

    OPENNSL_PBMP_CLEAR(pbm);
    OPENNSL_PBMP_PORT_ADD(pbm, hw_port);

    for (vid=0, count=0; vid<OPS_VLAN_COUNT && count<ops_vlan_count; vid++) {
        if (ops_vlans[vid] != NULL) {
            count++;
            vlanp = ops_vlans[vid];
            if (vlanp->hw_created) {
                if (link_is_up) {
                    // Link has come up.
                    if (OPENNSL_PBMP_MEMBER(vlanp->cfg_access_ports[unit], hw_port)) {
                        hw_add_ports_to_vlan(unit, pbm, pbm, vid, 1);
                        OPENNSL_PBMP_OR(vlanp->hw_access_ports[unit], pbm);

                    } else if (OPENNSL_PBMP_MEMBER(vlanp->cfg_trunk_ports[unit], hw_port)) {
                        hw_add_ports_to_vlan(unit, pbm, g_empty_pbm, vid, 0);
                        OPENNSL_PBMP_OR(vlanp->hw_trunk_ports[unit], pbm);

                    } else if (OPENNSL_PBMP_MEMBER(vlanp->cfg_native_tag_ports[unit], hw_port)) {
                        hw_add_ports_to_vlan(unit, pbm, g_empty_pbm, vid, 0);
                        OPENNSL_PBMP_OR(vlanp->hw_native_tag_ports[unit], pbm);
                        native_vlan_set(unit, vid, pbm, 0);

                    } else if (OPENNSL_PBMP_MEMBER(vlanp->cfg_native_untag_ports[unit], hw_port)) {
                        hw_add_ports_to_vlan(unit, pbm, pbm, vid, 0);
                        OPENNSL_PBMP_OR(vlanp->hw_native_untag_ports[unit], pbm);

                    }

                    if (OPENNSL_PBMP_MEMBER(vlanp->cfg_subinterface_ports[unit], hw_port)) {
                        hw_add_ports_to_vlan(unit, pbm, g_empty_pbm, vid, 0);
                        OPENNSL_PBMP_OR(vlanp->hw_subinterface_ports[unit], pbm);
                    }
                } else {
                    // Link has gone down.
                    if (OPENNSL_PBMP_MEMBER(vlanp->hw_access_ports[unit], hw_port)) {
                        hw_del_ports_from_vlan(unit, pbm, pbm, vid, 1);
                        OPENNSL_PBMP_PORT_REMOVE(vlanp->hw_access_ports[unit], hw_port);

                    } else if (OPENNSL_PBMP_MEMBER(vlanp->hw_trunk_ports[unit], hw_port)) {
                        hw_del_ports_from_vlan(unit, pbm, g_empty_pbm, vid, 0);
                        OPENNSL_PBMP_PORT_REMOVE(vlanp->hw_trunk_ports[unit], hw_port);

                    } else if (OPENNSL_PBMP_MEMBER(vlanp->hw_native_tag_ports[unit], hw_port)) {
                        hw_del_ports_from_vlan(unit, pbm, g_empty_pbm, vid, 0);
                        OPENNSL_PBMP_PORT_REMOVE(vlanp->hw_native_tag_ports[unit], hw_port);
                        native_vlan_clear(unit, pbm, 0);

                    } else if (OPENNSL_PBMP_MEMBER(vlanp->hw_native_untag_ports[unit], hw_port)) {
                        hw_del_ports_from_vlan(unit, pbm, pbm, vid, 0);
                        OPENNSL_PBMP_PORT_REMOVE(vlanp->hw_native_untag_ports[unit], hw_port);

                    }

                    if (OPENNSL_PBMP_MEMBER(vlanp->hw_subinterface_ports[unit], hw_port)) {
                        hw_del_ports_from_vlan(unit, pbm, g_empty_pbm, vid, 0);
                        OPENNSL_PBMP_PORT_REMOVE(vlanp->hw_subinterface_ports[unit], hw_port);
                    }
                }
            }
        }
    }

} // vlan_reconfig_on_link_change
