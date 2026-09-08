#pragma once

#include <string_view>
#include <functional>
#include <mutex>
#include <cstdint>

namespace Fault {

enum class Component : uint8_t {
  FaultManager = 0,
  ConfigManager,
  PumpController,
  Scheduler,
  ValveController,
  WaterTankController,
  RTCModule,
  WifiManager,
  SensorController,
};

enum class Code : uint8_t {
  FaultOverflow = 0,          // FaultManager
  PersistenceReadFail,        // FaultManager
  PersistenceWriteFail,       // FaultManager
  LittleFsMountingFailed,     // ConfigManager
  LoadFailed,                 // ConfigManager
  SaveFailed,                 // ConfigManager
  WateringPumpOverCurrent,    // PumpController
  RfRemoteInitFailed,         // PumpController
  RadioCommandSendFailed,     // PumpController
  RadioCommandRejected,       // PumpController
  RuntimeBuildFailed,         // Scheduler
  MissingIOExpander,          // ValveController
  OperationFailed,            // ValveController
  InconsistentLevelSensors,   // WaterTankController
  NoWaterWhileFilling,        // WaterTankController
  FillingWaterWhilePumpOff,   // WaterTankController
  DeviceNotFound,             // RTCModule
  LostPower,                  // RTCModule
  SyncFromRTCFailed,          // RTCModule
  SyncRTCFailed,              // RTCModule
  ApCreationFailed,           // WifiManager
  StationConnectionFailed,    // WifiManager
  StationSleepModeFailed,     // WifiManager
  MdnsFailed,                 // WifiManager
  PowerAdjustFailed,          // WifiManager
  AdcSamplerInitFailed,       // SensorController
  FlowSensorInitFailed,       // SensorController
  PumpCurrentReadingFailed,   // SensorController
  FlowReadingFailed,          // SensorController
  SoilMoistureReadingFailed,  // SensorController
  AdcDmaReadFailed,           // AdcSampler
};

struct Entry {
  Component component;
  Code      code;
  uint32_t  id = 0;
};

constexpr std::string_view toString(Component component) {
  switch (component) {
    case Component::FaultManager: return "FaultManager";
    case Component::ConfigManager: return "ConfigManager";
    case Component::PumpController: return "PumpController";
    case Component::Scheduler: return "Scheduler";
    case Component::ValveController: return "ValveController";
    case Component::WaterTankController: return "WaterTankController";
    case Component::RTCModule: return "RTCModule";
    case Component::WifiManager: return "WifiManager";
    case Component::SensorController: return "SensorController";
  }

  return "UnknownComponent";
}

constexpr std::string_view toString(Code code) {
  switch (code) {
    case Code::FaultOverflow: return "FaultOverflow";
    case Code::PersistenceReadFail: return "PersistenceReadFail";
    case Code::PersistenceWriteFail: return "PersistenceWriteFail";
    case Code::LittleFsMountingFailed: return "LittleFsMountingFailed";
    case Code::LoadFailed: return "LoadFailed";
    case Code::SaveFailed: return "SaveFailed";
    case Code::WateringPumpOverCurrent: return "WateringPumpOverCurrent";
    case Code::RfRemoteInitFailed: return "RfRemoteInitFailed";
    case Code::RadioCommandRejected: return "RadioCommandRejected";
    case Code::RadioCommandSendFailed: return "RadioCommandSendFailed";
    case Code::RuntimeBuildFailed: return "RuntimeBuildFailed";
    case Code::MissingIOExpander: return "MissingIOExpander";
    case Code::OperationFailed: return "OperationFailed";
    case Code::InconsistentLevelSensors: return "InconsistentLevelSensors";
    case Code::NoWaterWhileFilling: return "NoWaterWhileFilling";
    case Code::FillingWaterWhilePumpOff: return "FillingWaterWhilePumpOff";
    case Code::DeviceNotFound: return "DeviceNotFound";
    case Code::LostPower: return "LostPower";
    case Code::SyncRTCFailed: return "SyncRTCFailed";
    case Code::SyncFromRTCFailed: return "SyncFromRTCFailed";
    case Code::ApCreationFailed: return "ApCreationFailed";
    case Code::StationConnectionFailed: return "StationConnectionFailed";
    case Code::StationSleepModeFailed: return "StationSleepModeFailed";
    case Code::MdnsFailed: return "MdnsFailed";
    case Code::PowerAdjustFailed: return "PowerAdjustFailed";
    case Code::AdcSamplerInitFailed: return "AdcSamplerInitFailed";
    case Code::FlowSensorInitFailed: return "FlowSensorInitFailed";
    case Code::PumpCurrentReadingFailed: return "FlowReadingFailed";
    case Code::FlowReadingFailed: return "FlowReadingFailed";
    case Code::SoilMoistureReadingFailed: return "SoilMoistureReadingFailed";
    case Code::AdcDmaReadFailed: return "AdcDmaReadFailed";
  }

  return "UnknownCode";
}

}  // namespace Fault

class DeviceStateStorage;

class FaultManager {
public:
  // Maximum number of fault entries retained, including overflow events.
  // FaultOverflow may appear multiple times; each entry represents one overflow event.
  static constexpr uint8_t kMaxFaults    = 5;
  static constexpr uint8_t kMaxCallbacks = 3;

  using FaultCallback = std::function<void(uint8_t)>;

  struct Policy {
    bool persistent;
    bool requiresRestart;
  };

  FaultManager(DeviceStateStorage& storage);

  void subscribeOnFaultCountChanged(FaultCallback callback);

  void begin();

  // Registers an active fault.
  // Duplicate component/code pairs are ignored.
  // A dismissed fault may be registered again.
  bool set(Fault::Component component, Fault::Code code);
  // For manual user interaction (via UI): exact entry using ID
  bool clear(Fault::Entry entry);
  // For component auto-recovery
  bool clear(Fault::Component component, Fault::Code code);
  // Dismisses the oldest fault in the list.
  // Note that this does not clear the underlying fault condition.
  // If the condition remains active, the component may report it again.
  bool dismissOldest();
  bool dismissAll();

  // Returns true if at least one fault is present.
  bool any() const;
  // Returns the number of active faults.
  uint8_t count() const;
  // Returns the oldest fault in the list.
  bool oldest(Fault::Entry& fault) const;
  bool has(Fault::Component component, Fault::Code code) const;

  // Component identifies which subsystem reported the fault.
  // Policy is currently determined by the fault code itself.
  Policy policy(Fault::Code code) const;

private:
  void notifyFaultCountChanged() const;
  void logFault(Fault::Entry entry, uint8_t faultCount) const;

  void restoreFaults();
  void saveFaults();
  // Persistent fault encoding:
  //   bits 15..8 = Component
  //   bits  7..0 = Code
  //   +1 reserves 0 as "empty slot".
  uint16_t     encode(Fault::Component component, Fault::Code code) const;
  Fault::Entry decode(uint16_t value) const;

  std::array<FaultCallback, kMaxCallbacks> m_callbacks;
  uint8_t                                  m_callbackCount = 0;

  std::array<Fault::Entry, kMaxFaults > m_entries;
  uint8_t                               m_entryCount = 0;

  mutable std::mutex m_mutex;

  DeviceStateStorage& m_storage;

  // Starts at 1, 0 reserved as "invalid/no ID".
  uint32_t m_nextFaultId = 1;
};
