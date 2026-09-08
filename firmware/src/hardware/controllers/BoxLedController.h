#pragma once

#include <cstdint>

// 🟢 Status
//   OFF          = not running
//   BLINK        = connecting to Wi-Fi
//   DOUBLE PULSE = AP/provisioning
//   SOLID        = running
//
// 🔴 Fault
//   OFF          = no fault
//   SOLID        = fault
//
// 🟡 Watering
//   OFF          = not watering
//   SOLID        = watering
class BoxLedController {
public:
  static constexpr unsigned long kStaBlinkDelayMs = 300;
  static constexpr unsigned long kApPulseOnMs     = 100;
  static constexpr unsigned long kApPulseOffMs    = 100;
  static constexpr unsigned long kApPauseMs       = 800;

  enum class StatusMode : uint8_t {
    NotRunning = 0,
    StartingAP,
    ConnectingSTA,
    Running,
  };

  void begin();
  void update(unsigned long now);

  // Set mode for status led.
  // Idempotent: does nothing if the LED is already in the requested state.
  void setStatus(StatusMode status);
  // Turns on/off fault led.
  // Idempotent: does nothing if the LED is already in the requested state.
  void setFault(bool on);
  // Turns on/off watering led.
  // Idempotent: does nothing if the LED is already in the requested state.
  void setWatering(bool on);

private:
  enum class StatusLedState : uint8_t {
    SolidOff = 0,  // NotRunning
    SolidOn,       // Running
    Pulse1On,      // StartingAP
    Pulse1Off,     // StartingAP
    Pulse2On,      // StartingAP
    PulsePause,    // StartingAP
    BlinkOn,       // ConnectingSTA
    BlinkOff,      // ConnectingSTA
  };

  void transitionToTargetStatusMode(unsigned long now);
  bool isStatusModeChangePending() const;
  void transitionToStatusLedState(StatusLedState state, unsigned long now);

  void updateStatusLedState(unsigned long now);
  void updateStatusLedApState(unsigned long now);
  void updateStatusLedStaState(unsigned long now);

  StatusMode     m_currentStatusMode   = StatusMode::NotRunning;
  StatusMode     m_targetStatusMode    = StatusMode::NotRunning;
  StatusLedState m_statusLedState      = StatusLedState::SolidOff;
  unsigned long  m_lastStatusLedUpdate = 0;

  bool m_faultLedState    = false;
  bool m_wateringLedState = false;
};
