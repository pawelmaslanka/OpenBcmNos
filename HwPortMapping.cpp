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

#include "HwPortMapping.hpp"

#include "LoggingFacility.hpp"

using namespace OpenNos;

opennsl_port_t HwPortMapping::panelPortToHwPort(const PortId /* portNo */) {
    return 0;
}

PortId HwPortMapping::hwPortToPanelPort(const opennsl_port_t /* hwPort */) {
    return 0;
}

std::string HwPortMapping::getPanelPortName(const PortParameters portParameters) const {
    std::string portName {};
    if (portParameters.portNo == 0) {
        portName = "cpu0";
    }
    else if (portParameters.splitMode != PortSplitMode::None) {
        portName = stringFormat("port-%hu:%hu", portParameters.parentPort, portParameters.laneNo);
    }
    else {
        portName = stringFormat("port-%hu", portParameters.parentPort, portParameters.laneNo);
    }

    return portName;
}
