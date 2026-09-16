#pragma once

#include <string_view>
#include "types/FixedString.h"

class RFRemote;

class PumpController {
public:
  static constexpr unsigned long kUpdateDelayMs = 1000;

  PumpController(RFRemote        &remote,
                 std::string_view pumpName,
                 std::string_view turnOnCommand);

  void begin();
  // Must be called periodically while a pump is running to refresh
  // the receiver's 5s watchdog.
  void update(unsigned long now);

  // Requests the pump to turn on. Idempotent: does nothing if already requested on.
  void turnOn();
  // Requests the pump to turn off. Idempotent: does nothing if already requested off.
  void turnOff();

  bool isOn() const { return m_isOn; };

  void testRemote();

private:
  void sendRadioCommand();

  RFRemote        &m_remote;
  std::string_view m_pumpName;
  std::string_view m_turnOnCommand;

  unsigned long m_lastUpdateTime = 0;

  bool m_isOn = false;
};
