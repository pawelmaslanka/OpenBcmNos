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

#include "Asic.hpp"
#include "Command.hpp"
#include "HwPortManager.hpp"
#include "Port.hpp"
#include "Types.hpp"

class PortManager final : public CommandManager<Port, PortId, PortHandle>, public Observer,
                          public std::enable_shared_from_this<PortManager> {
  public:
    using Handle = std::shared_ptr<PortManager>;
    PortManager();
    virtual ~PortManager() override = default;
    Result::Value init();
    virtual void update(const ObservedSubjectHandle& subject, const UpdateReason updateReason) override;
    virtual ObserverId hash() override;
    virtual Result::Value execute(ResultCallback::Handle& callback) override;

  private:
    HwPortCommandFactory::Handle _hwPortCommandFactory;
    HwPortLinkScanHandling::Handle _hwPortLinkScanHandling;
    HwPortModuleInitializing::Handle _hwPortModuleInitializing;
    /// Let's backup ports link status to have known of port link status in case when
    /// port will be created in future
    std::map<PortId, bool> _portsLinkStatus;
};

class PortSettingMemento {
  public:
    using Handle = std::shared_ptr<PortSettingMemento>;
    virtual ~PortSettingMemento() = default;
};

class NullPortSettingMemento : public PortSettingMemento {
  public:
    virtual ~NullPortSettingMemento() override = default;
};

class PortSettingProcessing {
  public:
    using Handle = std::shared_ptr<PortSettingProcessing>;
    virtual ~PortSettingProcessing() = default;
    virtual Result::Value processCommand(Port& port) = 0;
};

class NullPortSettingProcessing : public PortSettingProcessing {
  public:
    virtual ~NullPortSettingProcessing() override = default;
    virtual Result::Value processCommand(Port& /* port */) override { return Result::Value::Fail; }
};

//Result::Value NullPortSettingProcessing::processCommand(Port&) { return Result::Value::Fail; }

/// Defines strategy design pattern for methods createMemento() and setMemento()
class PortSettingExecutor : public UndoableCommand {
  public:
    PortSettingExecutor(PortManager::Handle& portManager, const PortId portNo);
    virtual ~PortSettingExecutor() override = default;
    virtual size_t getCommitOrderingResolve() const override = 0;
    virtual Result::Value execute(ResultCallback::Handle& callback = gNullResultCallback) override;
    virtual Result::Value undo(ResultCallback::Handle& callback = gNullResultCallback) override;

  protected:
    void setPortSettingMemento(PortSettingMemento::Handle& portSettingMemento);
    void setPortSettingProcessing(PortSettingProcessing::Handle& portSettingProcessing);
    virtual void createMemento(Port& port) = 0;
    virtual Result::Value setMemento(Port& port) = 0;

    PortManager::Handle _portManager;
    PortId _portNo;
    PortSettingMemento::Handle _portSettingMemento;
    PortSettingProcessing::Handle _portSettingProcessing;
    bool _executed;
};

class PortCommandImplement :
        public PortSettingMemento,
        public PortSettingProcessing,
        public PortSettingExecutor {
  public:
    virtual ~PortCommandImplement() = default;
    PortCommandImplement(PortManager::Handle& portManager, const PortId portNo);
};

class PortShutdownSetting final : public PortCommandImplement,
        public std::enable_shared_from_this<PortShutdownSetting> {
  public:
    PortShutdownSetting(PortManager::Handle& portManager, const PortId portNo, const bool disable);
    virtual size_t getCommitOrderingResolve() const override;

  private:
    virtual void createMemento(Port& port) override;
    virtual Result::Value setMemento(Port& port) override;
    virtual Result::Value processCommand(Port& port) override;
    bool _disable;
    bool _disableMemento;
};

class PortSpeedSetting final : public PortCommandImplement,
                               public std::enable_shared_from_this<PortShutdownSetting> {
  public:
    PortSpeedSetting(PortManager::Handle& portManager, const PortId portNo, const PortSpeed speed);
    virtual size_t getCommitOrderingResolve() const override;

  private:
    virtual void createMemento(Port& port) override { _speedMemento = port.getSpeed(); }
    virtual Result::Value setMemento(Port& port) override { return port.setSpeed(_speedMemento); }
    virtual Result::Value processCommand(Port& port) override { return port.setSpeed(_speed); }
    PortSpeed _speed;
    PortSpeed _speedMemento;
};
