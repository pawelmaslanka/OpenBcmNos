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

#include "Types.hpp"

#include <map>
#include <memory>
#include <set>

using ObserverId = size_t;

enum class ObserverIdentifier : ObserverId {
    Port,
    PortManager
};

enum class UpdateReason {
    LagDown,
    LagUp,
    PortDown,
    PortUp,
    VlanCreate,
    VlanDestroy,
    LinkStatusUpdate
};

class ObservedSubject;
using ObservedSubjectHandle = std::shared_ptr<const ObservedSubject>;

class Observer {
  public:
    using Handle = std::shared_ptr<Observer>;
    Observer(std::set<UpdateReason> supportedUpdateReasons);
    virtual ~Observer() = default;
    virtual ObserverId hash() = 0;
    virtual void update(const ObservedSubjectHandle& subject, const UpdateReason updateReason) = 0;
    const std::set<UpdateReason>& getSupportedUpdateReasons() const;

  protected:
    std::set<UpdateReason> _supportedUpdateReasons;
};

class ObservedSubject : public std::enable_shared_from_this<ObservedSubject> {
  public:
    virtual ~ObservedSubject() = default;
    Result::Value addObserver(Observer::Handle& observerToAdd);
    void removeObserver(Observer::Handle& observerToRemove);

  protected:
    void notifyAllObservers(const UpdateReason updateFromReason) const;

  private:
    std::set<Observer::Handle> _observers;
};

//class DependentUser {
//  public:
//    using Handle = std::shared_ptr<DependentUser>;
//    virtual ~DependentUser() = default;
//    virtual ObserverId id() = 0;
//    /// @param [in|out] info stores info about dependent user. Dependent user puts info about self in this parameter
//    /// @return true if still this object is dependent, otherwise returns false
//    virtual bool dependencyInfo(std::string& info) = 0;
//};

//struct DependentUserCompare {
//    bool operator() (const DependentUser::Handle& lhs, const DependentUser::Handle& rhs) const {
//        return lhs->id() < rhs->id();
//    }
//};

//class DependencySource {
//  public:
//    void addDependentUser(DependentUser::Handle& observerToAdd);
//    void removeObserver(DependentUser::Handle& observerToRemove);

//  protected:
//    void checkDependency() const;

//  private:
//    std::map<ObserverId, DependentUser::Handle, DependentUserCompare> _dependentUsers;
//};
