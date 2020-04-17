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

#include <array>
#include <cstdint>
#include <limits>
#include <memory>

#ifndef UNIT_TEST
#   define FINAL final
#endif

namespace Result {
  enum class Value : uint16_t {
      Success = 0,
      NotExists,
      PortNotExists,
      VlanNotExists,
      PortInVlanNotExists,
      AlreadyExists,
      NoMemory,
      UpdateReasonNotSupported,
      CommandNotUndoable,
      Fail = std::numeric_limits<uint8_t>::max()
  };

  static inline bool Failed(const Value result) noexcept {
      return result != Value::Success;
  }

  constexpr auto gResultSuccessString { "Success" };
  constexpr auto gResultFailString { "Fail" };
}

template <typename TYPE_HANDLE>
struct WeakHandleCompare {
   bool operator() (const TYPE_HANDLE& lhs, const TYPE_HANDLE& rhs) const {
       if (auto lhsShared = lhs.lock()) { // Has to be copied into a shared_ptr before usage
           if (auto rhsShared = rhs.lock()) {
               return lhsShared->id() < rhsShared->id();
           }

           return true;
       }

       return false;
   }
};

/// @note Below types are dynamically create and delete so they are define in one place
class Port;
using MemberPortOwnerHandle = std::shared_ptr<Port>;
using PortId = uint16_t;
using PortHandle = std::weak_ptr<Port>;

class Lag;
using LagOwnedHandle = std::weak_ptr<Lag>;
using LagId = uint16_t;
using LagHandle = std::weak_ptr<Lag>;

class Stp;
using StpOwnedHandle = std::weak_ptr<Stp>;
using StpId = uint16_t;
using StpHandle = std::weak_ptr<Stp>;

class Vlan;
using VlanOwnedHandle = std::weak_ptr<Vlan>;
using VlanId = uint16_t;
using VlanHandle = std::weak_ptr<Vlan>;

enum class PortSpeed : size_t {
    _1Mb   = 1,
    _10Mb  = _1Mb * 10,
    _100Mb = _1Mb * 100,
    _1Gb   = _1Mb * 1000,
    _10Gb  = _1Gb * 10,
    _20Gb  = _1Gb * 20,
    _25Gb  = _1Gb * 25,
    _40Gb  = _1Gb * 40,
    _50Gb  = _1Gb * 50,
    _100Gb = _1Gb * 100,
    _200Gb = _1Gb * 200,
    _400Gb = _1Gb * 400,
    Max = 0
};

enum class PortSplitMode : uint8_t {
    None,
    _4x10G,
    _4x25G,
    _2x50G,
    _4x100,
    _2x200
};

enum class PortLaneNo : uint8_t {
    // 10G/25G/100G mode
    Lane_1 = 1,
    Lane_2,
    Lane_3,
    Lane_4,
    // 50G/200G mode
    Lane_1_2,
    Lane_3_4
};

static constexpr uint8_t MaxSlavePorts = 4;
static constexpr uint8_t MacAddressSize = 6;

struct PortParameters {
    static constexpr PortId InvalidPort = std::numeric_limits<PortId>::max();
    PortId portNo;
    bool shutdowned;
    bool autoneg;
    bool fec;
    bool fullDuplex;
    bool rxPause;
    bool txPause;
    PortSplitMode splitMode;
    PortSpeed speed;
    uint32_t preemphasis;
    uint32_t current;
    PortId parentPort;
    PortLaneNo laneNo;
    std::array<PortId, MaxSlavePorts> slavePorts;
    std::array<uint8_t, MacAddressSize> macAddress;
};
