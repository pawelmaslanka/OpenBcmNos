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

#include "LoggingFacility.hpp"
#include "Types.hpp"

#include <map>
#include <memory>
#include <set>
#include <string>

enum CommitOrderingResolve : size_t {
    DependencyTo = 1 << 0,
    WithoutDependencyTo = 1 << 1,
    PortSettingToEnabledBreakoutMode = 1 << 2,
    PortSettingToActiveLagMemeber = 1 << 3,
    PortSetWithDependencyToActiveLagMember,
    PortSetWithoutDependencyToActiveLagMember,
    PortSetWithoutDependencyToEnabledBreakoutMode,
    PortSetWithDependencyToEnabledBreakoutMode,
    VlanCreate,
    PortCreate,
    PortInit,
    PortSet,
    LagCreate,
    StpCreate,
    FdbFlush,
    VlanAddMemberPorts,
    VlanRemoveMemberPorts,
    StpDelete,
    LagDelete,
    PortDeinit,
    PortDelete,
    VlanDelete,
    Unordered
};

class ResultCallback {
  public:
    using Handle = std::shared_ptr<ResultCallback>;
    virtual ~ResultCallback() = default;
    virtual void onCommandResult(const Result::Value result, const std::string_view msg = "") = 0;
};

class NullResultCallback : public ResultCallback {
  public:
    inline NullResultCallback();
    virtual ~NullResultCallback() override = default;
    inline virtual void onCommandResult([[maybe_unused]] const Result::Value result, [[maybe_unused]] const std::string_view msg) override;
};

static ResultCallback::Handle gNullResultCallback = std::make_shared<NullResultCallback>();

#define CALL_CALLBACK_AND_RETURN_RESULT_IF_FAIL(RESULT, CB)   \
    CB->onCommandResult(RESULT);                              \
    if (Result::Failed(RESULT)) return RESULT

#define CALL_CALLBACK_AND_RETURN_RESULT_IF_FAIL_OR_SUCCESS(RESULT, CB)   \
    CALL_CALLBACK_AND_RETURN_RESULT_IF_FAIL(RESULT, CB);                 \
    return RESULT

#define CALL_CALLBACK_AND_RETURN_RESULT_SUCCESS(CB)     \
    CB->onCommandResult(Result::Value::Success);        \
    return Result::Value::Success

#define CALL_CALLBACK_AND_RETURN_RESULT_FAIL(CB)     \
    CB->onCommandResult(Result::Value::Fail);        \
    return Result::Value::Fail

class Command {
  public:
    using Handle = std::shared_ptr<Command>;
    virtual ~Command() = default;
    virtual size_t getCommitOrderingResolve() const = 0;
    virtual Result::Value execute(ResultCallback::Handle& callback = gNullResultCallback) = 0;
};

class Revertable {
  public:
    virtual ~Revertable() = default;
    virtual Result::Value undo(ResultCallback::Handle& callback = gNullResultCallback) = 0;
};

class UndoableCommand : public Command, public Revertable {
  public:
    using Handle = std::shared_ptr<UndoableCommand>;
};

/// @note Decorator will extend behaviour of execute() and undo() methods
template <typename TYPE, typename TYPE_ID, typename TYPE_HANDLE>
class CommandManager : public UndoableCommand {
  public:
    virtual ~CommandManager() = default;
    Result::Value add(const TYPE_ID id) {
        auto foundToRemovingIt = _toRemoving.find(id);
        if (foundToRemovingIt != std::end(_toRemoving)) {
            _toRemoving.erase(foundToRemovingIt);
        }

        if (_configured.find(id) != std::end(_configured)) {
            return Result::Value::AlreadyExists;
        }

        _toAdding.emplace(id);
        return Result::Value::Success;
    }

    Result::Value remove(const TYPE_ID id) {
        auto foundToAddingIt = _toAdding.find(id);
        if (foundToAddingIt != std::end(_toAdding)) {
            _toAdding.erase(foundToAddingIt);
        }

        if (std::end(_configured) == _configured.find(id)) {
            return Result::Value::NotExists;
        }

        _toRemoving.emplace(id);
        return Result::Value::Success;
    }

    virtual Result::Value execute(ResultCallback::Handle& callback) override {
        Result::Value result = Result::Value::Success;
        try {
          _mementoAdded.clear();
          _mementoRemoved.clear();
          for (const auto id : _toRemoving) {
              destroyHandle(_idToHandleMap[id]);
              _idToHandleMap.erase(id);
              _configured.erase(id);
              _mementoRemoved.emplace(id);
          }

          for (const auto id : _toAdding) {
              TYPE_HANDLE handle = createHandle(id);
              _idToHandleMap.emplace(id, handle);
              _configured.emplace(id);
              _mementoAdded.emplace(id);
          }
        }
        catch (...) {
          if (not Failed(undo(callback))) {
              callback->onCommandResult(Result::Value::Fail);
          }

          result =  Result::Value::Fail;
        }

        _toRemoving.clear();
        _toAdding.clear();
        callback->onCommandResult(result);
        return result;
    }

    virtual Result::Value undo(ResultCallback::Handle& callback) override {
        Result::Value result = Result::Value::Success;
        try {
          for (const auto id : _mementoAdded) {
              destroyHandle(_idToHandleMap[id]);
              _idToHandleMap.erase(id);
              _configured.erase(id);
          }

          for (const auto id : _mementoRemoved) {
              TYPE_HANDLE handle = createHandle(id);
              _idToHandleMap.emplace(id, handle);
              _configured.emplace(id);
          }
        }
        catch (...) {
          callback->onCommandResult(Result::Value::Fail);
          result = Result::Value::Fail;
        }

        _toRemoving.clear();
        _toAdding.clear();
        _mementoAdded.clear();
        _mementoRemoved.clear();
        return result;
    }

    bool exists(const TYPE_ID id) const {
        return _idToHandleMap.find(id) != std::end(_idToHandleMap);
    }

    TYPE_HANDLE& getHandle(const TYPE_ID id) {
        return _idToHandleMap.at(id);
    }

    virtual size_t getCommitOrderingResolve() const override { return CommitOrderingResolve::Unordered; }

  protected:
    virtual TYPE_HANDLE createHandle(const TYPE_ID id) { return std::make_shared<TYPE>(id); }
    virtual void destroyHandle(TYPE_HANDLE handle) { handle.reset(); }

    std::map<TYPE_ID, TYPE_HANDLE> _idToHandleMap;
    std::set<TYPE_ID> _toAdding;
    std::set<TYPE_ID> _toRemoving;
    std::set<TYPE_ID> _mementoAdded;
    std::set<TYPE_ID> _mementoRemoved;
    std::set<TYPE_ID> _configured;
};

NullResultCallback::NullResultCallback() { /* Nothing more to do */ }

void NullResultCallback::onCommandResult([[maybe_unused]] const Result::Value result, [[maybe_unused]] const std::string_view msg) { }
