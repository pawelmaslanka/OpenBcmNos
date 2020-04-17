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

#include "Command.hpp"

#include <queue>

class HwCommandProcessor : public Command {
  public:
    using Handle = std::shared_ptr<HwCommandProcessor>;
    ~HwCommandProcessor() = default;
    Result addAddingVlanMemberPortsToExecute(UndoableCommand::Handle cmdHandle);
    virtual void execute() override;

    Result addCommandToExecute(Command::Handle command) {
        _commands.push_back();
    }

    /// runCommandsProcessor() is the function which will run in separated thread to execute directly
    /// request to ASIC.
    void runCommandsProcessor();

  private:
    std::priority_queue<Command::Handle> _commands;
    std::queue<UndoableCommand::Handle> _addingLagMemberPorts;
    std::queue<UndoableCommand::Handle> _addingVlanMemberPorts;
};

