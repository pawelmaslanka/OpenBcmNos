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
// OpenNOS
#include "Command.hpp"
#include "HwPortManager.hpp"
#include "Observer.hpp"
#include "Types.hpp"
// C++ Standard Library
#include <array>
#include <set>

/// Port can be notified from hardware that link has been turn into down status. That's why it is observer.
/// Port can also notify that link status has changed. That's why it is observed subject.
class Port final : public Observer, public ObservedSubject {
  public:
    struct Memento {
        using Handle = std::unique_ptr<Memento>;
        bool created;
        PortParameters portParameters;
    };

    Port(const PortId portNo);
    virtual ~Port() override;
    PortId id() const noexcept;
    virtual ObserverId hash() override;
    virtual void update(const ObservedSubjectHandle& subject, const UpdateReason updateReason) override;
    void setHwPortCommandFactory(HwPortCommandFactory::Handle& hwPortCommandFactory);

    Result::Value create();
    Result::Value destroy();
    bool isShutdowned() const;
    Result::Value shutdown(const bool disable);
    Result::Value setSpeed(const PortSpeed speed);
    PortSpeed getSpeed() const;
    void setLinkStatus(const bool linkedUp);
    /// @retval false if link is down
    /// @retval true if link is up
    bool isOperable() const;

    Memento::Handle createMemento();
    Result::Value setMemento(Memento::Handle);

  private:
    friend struct PortCompare;
    friend struct MemberPortOwnerCompare;

    bool _created;
    bool _linkedUp;
    PortParameters _parameters;
    HwPortCommandFactory::Handle _hwPortCommandFactory;
};

//struct PortCompare {
//   bool operator() (const PortHandle& lhs, const PortHandle& rhs) const {
//       return lhs->id() < rhs->id();
//   }
//};

//struct MemberPortOwnerCompare {
//    bool operator() (const MemberPortOwnerHandle& lhs, const MemberPortOwnerHandle& rhs) const {
//        return lhs->id() < rhs->id();
//    }
//};
