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

#include "HwCommandProcessor.hpp"

Result HwCommandProcessor::addAddingLagMemberPortsToExecute(UndoableCommand::Handle cmdHandle) {
    _addingLagMemberPorts.push(cmdHandle);
    return Result::Success;
}

Result HwCommandProcessor::addAddingVlanMemberPortsToExecute(UndoableCommand::Handle cmdHandle) {
    _addingVlanMemberPorts.push(cmdHandle);
    return Result::Success;
}

Result HwCommandProcessor::execute() {
    // Implements Round-Robin scheduler for access to ASIC
    /// @todo Lock _addingLagMemberPorts mutex
    if (not _addingLagMemberPorts.empty()) {
        _addingLagMemberPorts.front()->execute();
        _addingLagMemberPorts.pop();
    }

    /// @todo Lock _addingVlanMemberPorts mutex
    if (not _addingVlanMemberPorts.empty()) {
        _addingVlanMemberPorts.front()->execute();
        _addingVlanMemberPorts.pop();
    }

    return Result::Success;
}
