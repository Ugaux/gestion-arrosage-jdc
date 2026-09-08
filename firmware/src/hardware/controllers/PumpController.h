#pragma once

#include <cstdint>
#include <functional>
#include "hardware/RFRemote.h"

class SystemMonitor;
class FaultManager;

class PumpController {
public:
  static constexpr unsigned long kUpdateDelayMs    = 500;
  static constexpr unsigned long kRadioSendDelayMs = 1000;

  enum class Type : uint8_t {
    Watering = 0,
    WaterTank
  };

  enum class WateringBlockReason : uint8_t {
    None = 0,
    TankEmpty
  };

  PumpController(SystemMonitor &monitor, FaultManager &faults);

  std::function<void(WateringBlockReason)> onWateringBlocked;

  void begin();
  // Must be called periodically while a pump is running to refresh
  // the receiver's 5s watchdog.
  void update(unsigned long now);

  // Requests the pump to turn on. Idempotent: does nothing if already requested on.
  void requestOn(Type type);
  // Requests the pump to turn off. Idempotent: does nothing if already requested off.
  void requestOff(Type type);

  void allowWatering();
  void blockWatering(WateringBlockReason reason);
  bool isWateringBlocked() const { return m_wateringBlocked; }

  bool isPumpOn(Type type) const;

  void testRemote();

private:
  void sendNextRadioCommand(unsigned long now);
  void sendRadioCommand(Type type);
  void updateWateringPumpState();
  void logWateringBlocked() const;

  void radioTxComplete(esp_err_t err);

  enum class PumpState : uint8_t {
    Off = 0,
    Blocked,
    On,
  };

  RFRemote m_remote;

  SystemMonitor &m_monitor;
  FaultManager  &m_faults;

  unsigned long       m_lastUpdateTime        = 0;
  bool                m_wateringRequested     = false;
  bool                m_wateringBlocked       = false;
  WateringBlockReason m_wateringBlockedReason = WateringBlockReason::None;
  PumpState           m_wateringPumpState     = PumpState::Off;
  PumpState           m_waterTankPumpState    = PumpState::Off;

  bool          m_initialized       = false;
  Type          m_nextRadioTarget   = Type::Watering;
  unsigned long m_nextRadioSendTime = 0;
  unsigned long m_lastRadioSendTime = 0;
};
