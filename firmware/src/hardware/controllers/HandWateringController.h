#pragma once

#include <array>
#include <ezButton.h>
#include "hardware/OutputBlinker.h"

class PumpController;

class HandWateringController {
public:
  static constexpr unsigned long kSeconds = 1000;
  static constexpr unsigned long kMinutes = 60UL * kSeconds;

  HandWateringController(PumpController &pumps);

  void begin(bool testing = false);
  void update(unsigned long now);

  void turnAllOFF();

private:
  static constexpr char kTag[] = "[Hand Watering]";

  enum class SystemState : uint8_t {
    Idle = 0,
    FirstPhase,
    SecondPhase,
    LastPhase,
  };

  PumpController &m_pumps;

  ezButton      m_button;
  OutputBlinker m_buttonLedBlinker;

  SystemState   m_state          = SystemState::Idle;
  unsigned long m_cycleStartTime = 0;

  std::array<unsigned long, 3> m_triggerTimes = {
    17 * kMinutes + 30 * kSeconds,
    19 * kMinutes + 30 * kSeconds,
    20 * kMinutes,
  };
};
