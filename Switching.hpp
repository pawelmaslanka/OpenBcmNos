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

#include <Asic.hpp>
#include <PortManager.hpp>

class MacLearning {
  public:
    using Handle = std::shared_ptr<MacLearning>;
    using MacLearningCallback = Result::Value(*)(std::string);
    MacLearningCallback getCallback();
  private:
};

class RxCallback {
  public:
    using Handle = std::shared_ptr<RxCallback>;
//    using CallbackFunction =
};

class Switching {
  public:
    using Handle = std::shared_ptr<Switching>;
    Switching(Asic::Handle& asic, PortManager::Handle& portManager);
    Result::Value init();

  private:
    Asic::Handle _asic;
    PortManager::Handle _portManager;
};

