#include "PumpController.h"

#include <Arduino.h>
#include "hardware/RFRemote.h"

PumpController::PumpController(
  RFRemote        &remote,
  std::string_view pumpName,
  std::string_view turnOnCommand)
  : m_remote(remote),
    m_pumpName(pumpName),
    m_turnOnCommand(turnOnCommand) {}

void PumpController::begin() {
  log_d("PumpController for %.*s pump is initializing...",
        static_cast<int>(m_pumpName.size()),
        m_pumpName.data());

  // Allow the first update() call to execute immediately.
  m_lastUpdateTime = -kUpdateDelayMs;

  log_i("PumpController for %.*s pump has initialized",
        static_cast<int>(m_pumpName.size()),
        m_pumpName.data());
}

void PumpController::update(unsigned long now) {
  if (!m_isOn)
    return;

  if (now - m_lastUpdateTime < kUpdateDelayMs)
    return;

  m_lastUpdateTime = now;

  // Pump receivers have a 5s watchdog: sending a message
  // keeps the pump running. If no message is received
  // within 5s, the receiver stops the pump.
  sendRadioCommand();
}

void PumpController::turnOn() {
  if (m_isOn)
    return;

  m_isOn = true;
  log_i("Turned on %.*s pump",
        static_cast<int>(m_pumpName.size()),
        m_pumpName.data());

  // Update immediately instead of waiting for next update().
  update(millis());
}

void PumpController::turnOff() {
  if (!m_isOn)
    return;

  m_isOn = false;
  log_i("Turned off %.*s pump",
        static_cast<int>(m_pumpName.size()),
        m_pumpName.data());
}

void PumpController::testRemote() {
  while (true) {
    // "A" remote button of home pump
    m_remote.sendRF("100001111100010111110010");
    log_d("Remote testing: radio command"
          " for home pump has been sent!");
    delay(2000);
  }
}

void PumpController::sendRadioCommand() {
  if (m_remote.sendRF(m_turnOnCommand) == RFRemote::SendResult::Queued)
    return;

  log_d("RF command to %.*s pump could not be queued",
        static_cast<int>(m_pumpName.size()),
        m_pumpName.data());
}
