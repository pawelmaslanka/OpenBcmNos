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

#include "Switching.hpp"

extern "C" {
#   include <opennsl/l2.h>
}

Switching::Switching(Asic::Handle& asic, PortManager::Handle& portManager, MacLearning::Handle& macLearning)
    : _asic { asic }, _portManager { portManager } {
    // Nothing more to do
}

Result::Value Switching::init() {
    if (Failed(_asic->init())) {
        ERROR_LOG("Failed initialize ASIC");
        return Result::Value::Fail;
    }

    if (Failed(_portManager->init())) {
        ERROR_LOG("Failed initialize port module");
        return Result::Value::Fail;
    }

    return Result::Value::Success;
}
