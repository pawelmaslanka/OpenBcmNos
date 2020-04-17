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

#include "Observer.hpp"

#include <algorithm>

class IsObserverEqualTo final {
  public:
    explicit IsObserverEqualTo(const Observer::Handle& observer) : _observer { observer } {}
    bool operator()(const Observer::Handle& observerToCompare) {
        return observerToCompare->hash() == _observer->hash();
    }

  private:
    Observer::Handle _observer;
};

Observer::Observer(std::set<UpdateReason> supportedUpdateReasons)
    : _supportedUpdateReasons { supportedUpdateReasons } {
    // Nothing more to do
}

const std::set<UpdateReason>& Observer::getSupportedUpdateReasons() const {
    return _supportedUpdateReasons;
}

Result::Value ObservedSubject::addObserver(Observer::Handle& observerToAdd) {
    auto observerIt = std::find_if(std::begin(_observers), std::end(_observers), IsObserverEqualTo(observerToAdd));
    if (std::end(_observers) == observerIt) {
        _observers.emplace(observerToAdd);
    }

    return Result::Value::Success;
}

void ObservedSubject::removeObserver(Observer::Handle& observerToRemove) {
    auto observerIt = std::find_if(std::begin(_observers), std::end(_observers), IsObserverEqualTo(observerToRemove));
    if (observerIt != std::end(_observers)) {
        _observers.erase(observerIt);
    }
}

void ObservedSubject::notifyAllObservers(const UpdateReason updateFromReason) const {
    for (auto& observer : _observers) {
        const auto& supportedUpdateReasons = observer->getSupportedUpdateReasons();
        if (supportedUpdateReasons.find(updateFromReason) != std::end(supportedUpdateReasons)) {
            observer->update(shared_from_this(), updateFromReason);
        }
    }
}
