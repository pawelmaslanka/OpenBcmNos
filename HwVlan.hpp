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

#include "Vlan.hpp"

// This class maps front panel ports into ASIC port.
class HwVlan
{
  public:
    HwVlan(Vlan::Handle vlan) : _vlan { vlan } {}

  private:
    Vlan::Handle _vlan;
};

/// todo Should be notified about commit command?
class HwAddingVlanMemberPorts : public Observer, public Command::ResultNotifier {
  public:
    HwAddingVlanMemberPorts(ManagingVlanMemberPorts::Handle addingVlanMemberPortsHandle)
        : _addingVlanMemberPortsHandle { addingVlanMemberPortsHandle } {}

    virtual void onCommandResultEvent(const Result result, const std::string msg) override;

  private:
    ManagingVlanMemberPorts::Handle _addingVlanMemberPortsHandle;
};
