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

#include <Types.hpp>
#include <memory>

class Asic {
  public:
    using Handle = std::shared_ptr<Asic>;
    Asic();
    Result::Value init();
    static inline constexpr int getDefaultHwUnit();
    static inline constexpr int getHwUnitsCount();
    static inline constexpr int getCpuPort(const int hwUnit);
    static inline constexpr int getDefaultStgId();
    static int getMaxPorts(const int hwUnit);
};

constexpr int Asic::getDefaultHwUnit() { return 0; }

constexpr int Asic::getHwUnitsCount() { return 1; }

constexpr int Asic::getCpuPort(const int hwUnit) {
    switch (hwUnit) {
      default:
            return 0;
    }
}

constexpr int Asic::getDefaultStgId() { return 1; }



