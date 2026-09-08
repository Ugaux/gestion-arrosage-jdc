#pragma once

#include <cstdint>
#include "core/FaultManager.h"

class PumpController;
class SensorController;

class WaterTankController {
public:
  static constexpr unsigned long kUpdateDelayMs = 500;

  static constexpr unsigned long kAllowWateringDelayMs = 30UL * 1000;
  static constexpr unsigned long kPumpCooldownDelayMs  = 5UL * 60 * 1000;

  static constexpr unsigned long kNoWaterTimeoutMs = 10UL * 1000;
  static constexpr unsigned long kMaxFillingTimeMs = 30UL * 60 * 1000;

  enum class State : uint8_t {
    Uninitialized = 0,
    Idle,
    Filling,
    PumpCooldown,
    Fault,
  };

  enum class Level : uint8_t {
    Unknown = 0,
    Empty,
    Partial,
    Full
  };

  WaterTankController(FaultManager&     faults,
                      PumpController&   pumps,
                      SensorController& sensors);

  void begin();
  void update(unsigned long now);

  // Explicit/manual request
  bool startFilling();

  State state() const { return m_state; }
  Level level() const { return m_level; }

  // Resume normal state-machine operation.
  bool resume();

private:
  void enterFault(Fault::Code code, unsigned long now);
  // Transitions to the new state. Idempotent: does nothing if already in that state.
  void transitionTo(State state, unsigned long now);
  void onEnterState(State state, unsigned long now);

  FaultManager&     m_faults;
  PumpController&   m_pumps;
  SensorController& m_sensors;

  State m_state = State::Uninitialized;
  Level m_level = Level::Unknown;

  unsigned long m_lastUpdateTime        = 0;
  unsigned long m_fillingStartTime      = 0;
  unsigned long m_pumpCooldownStartTime = 0;
  unsigned long m_waterAvailableSince   = 0;
};
