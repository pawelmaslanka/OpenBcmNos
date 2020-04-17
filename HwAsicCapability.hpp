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

#include "Compile.hpp"
// C++ Standard Library
#include <memory>

namespace OpenNos {

class HwAsicCapability {
  public:
    using Handle = std::shared_ptr<HwAsicCapability>;
    virtual ~HwAsicCapability() = default;
    virtual void init() = 0;
    virtual int getDefaultHwUnit() = 0;
    virtual int getCpuPort(const int hwUnit) = 0;
};

class BcmAsicCapability FINAL : public HwAsicCapability {
  public:
    virtual ~BcmAsicCapability() override = default;
    virtual void init() override;
    virtual int getDefaultHwUnit() override { return 0; }
    virtual int getCpuPort(const int /* hwUnit */) override { return 0; }
};

} // namespace OpenNos
