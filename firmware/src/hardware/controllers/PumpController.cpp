#include "PumpController.h"

#include <Arduino.h>
#include "Pinout.h"
#include "core/SystemMonitor.h"
#include "core/FaultManager.h"

PumpController::PumpController(
  SystemMonitor &monitor, FaultManager &faults)
  : m_remote(Pinout::ESP::RadioTX, RMT_CHANNEL_0),
    m_monitor(monitor),
    m_faults(faults) {}

void PumpController::begin() {
  log_d("PumpController is initializing...");

  if (!onWateringBlocked) {
    log_e("onWateringBlocked callback is not configured!");
    while (true) delay(1000);
  }

  // Repeat sending 5 times. With the protocol 1 used,
  // it means a send time of 44.8ms*5=224ms
  m_remote.setRepeatTransmit(5);

  if (!m_remote.begin()) {
    m_faults.set(Fault::Component::PumpController,
                 Fault::Code::RfRemoteInitFailed);
    return;
  }

  m_remote.onTxFinished = [this](esp_err_t err) {
    this->radioTxComplete(err);
  };

  // Allow the first update() call to execute immediately.
  m_lastUpdateTime = millis() - kUpdateDelayMs;

  m_initialized = true;
  log_i("PumpController has initialized");
}

void PumpController::update(unsigned long now) {
  if (!m_initialized)
    return;

  // Update pump controller state
  if (now - m_lastUpdateTime >= kUpdateDelayMs) {

    m_lastUpdateTime = now;

    updateWateringPumpState();
  }

  // Schedule next radio command send
  if (now >= m_nextRadioSendTime) {
    // Pump receivers have a 5s watchdog: sending a message
    // keeps the pump running. If no message is received
    // within 5s, the receiver stops the pump.

    sendNextRadioCommand(now);
  }
}

void PumpController::requestOn(Type type) {
  switch (type) {

    case Type::Watering:
      if (!m_monitor.isWateringSystemHealthy()) return;
      if (m_wateringRequested) return;
      log_d("Watering pump requested on");
      m_wateringRequested = true;
      break;

    case Type::WaterTank:
      if (m_waterTankPumpState == PumpState::On) return;
      log_i("Water tank pump on");
      m_waterTankPumpState = PumpState::On;
      break;
  }
}

void PumpController::requestOff(Type type) {
  switch (type) {

    case Type::Watering:
      if (!m_wateringRequested) return;
      log_d("Watering pump requested off");
      m_wateringRequested = false;
      break;

    case Type::WaterTank:
      if (m_waterTankPumpState == PumpState::Off) return;
      log_i("Water tank pump off");
      m_waterTankPumpState = PumpState::Off;
      break;
  }
}

void PumpController::allowWatering() {
  m_wateringBlocked       = false;
  m_wateringBlockedReason = WateringBlockReason::None;
}

void PumpController::blockWatering(WateringBlockReason reason) {
  if (reason == WateringBlockReason::None)
    return;

  if (m_wateringBlockedReason != WateringBlockReason::None)
    return;

  m_wateringBlocked       = true;
  m_wateringBlockedReason = reason;
}

bool PumpController::isPumpOn(Type type) const {
  if (type == Type::Watering)
    return m_wateringPumpState == PumpState::On;
  if (type == Type::WaterTank)
    return m_waterTankPumpState == PumpState::On;

  return false;
}

void PumpController::testRemote() {
  if (!m_initialized) {
    log_w("Failed to init");
    while (true) { delay(1000); }
  }

  while (true) {
    log_d("Sending rf!");
    // "A" button of remote for home pump
    m_remote.sendRF("100001111100010111110010");
    delay(2000);
  }
}

void PumpController::sendNextRadioCommand(unsigned long now) {
  const bool wateringPumpOn =
    m_wateringPumpState == PumpState::On;

  const bool waterTankPumpOn =
    m_waterTankPumpState == PumpState::On;

  Type currentTarget;

  if (wateringPumpOn && waterTankPumpOn) {
    // Both pumps are ON: alternate targets
    currentTarget = m_nextRadioTarget;
    switch (currentTarget) {
      case Type::Watering:
        m_nextRadioTarget = Type::WaterTank;
        break;
      case Type::WaterTank:
        m_nextRadioTarget = Type::Watering;
        break;
    }
    m_nextRadioSendTime = now + kRadioSendDelayMs / 2;
  } else if (wateringPumpOn) {
    // Only watering pump is ON
    currentTarget       = Type::Watering;
    m_nextRadioSendTime = now + kRadioSendDelayMs;
  } else if (waterTankPumpOn) {
    // Only water tank pump is ON
    currentTarget       = Type::WaterTank;
    m_nextRadioSendTime = now + kRadioSendDelayMs;
  } else {  // Nothing is running
    m_nextRadioSendTime = now + kUpdateDelayMs;
    return;
  }

  // If both are ON, alternate every 500 ms.
  // If only one is ON, send it every 1 second.
  sendRadioCommand(currentTarget);
  return;
}

void PumpController::sendRadioCommand(Type type) {
  bool accepted = false;

  switch (type) {

    case Type::Watering:
      // "C" button of remote
      accepted = m_remote.sendRF("101000000110101010110100");
      break;

    case Type::WaterTank:
      // "A" button of remote
      accepted = m_remote.sendRF("101000000110101010110010");
      break;
  }

  if (!accepted) {
    if (m_remote.isBusy())
      log_d("Remote could not send command,"
            " it was already transmitting");
    else
      log_d("Remote rejected RF command");

    m_faults.set(Fault::Component::PumpController,
                 Fault::Code::RadioCommandRejected);
    return;
  }

  m_faults.clear(Fault::Component::PumpController,
                 Fault::Code::RadioCommandRejected);
}

void PumpController::updateWateringPumpState() {
  PumpState newState;

  if (!m_wateringRequested)
    newState = PumpState::Off;
  else if (m_wateringBlocked)
    newState = PumpState::Blocked;
  else
    newState = PumpState::On;

  if (newState == m_wateringPumpState)
    return;

  const bool pumpWasOn = m_wateringPumpState == PumpState::On;
  const bool pumpIsOn  = newState == PumpState::On;

  m_wateringPumpState = newState;

  if (newState == PumpState::Blocked) {
    logWateringBlocked();
    onWateringBlocked(m_wateringBlockedReason);
  }

  // Only log when the pump's effective state changes:
  // OFF/BLOCKED -> ON, or ON -> OFF/BLOCKED.
  if (pumpIsOn != pumpWasOn)
    log_i("Watering pump %s", pumpIsOn ? "on" : "off");
}

void PumpController::logWateringBlocked() const {
  const char logPrefix[] = "Watering pump run blocked:";

  switch (m_wateringBlockedReason) {

    case WateringBlockReason::TankEmpty:
      log_w("%s water tank is empty", logPrefix);
      break;

    default:
      log_w("%s unknown reason", logPrefix);
  }
}

void PumpController::radioTxComplete(esp_err_t err) {
  if (err == ESP_OK) {
    m_faults.clear(
      Fault::Component::PumpController,
      Fault::Code::RadioCommandSendFailed);
    return;
  }

  log_d("RF transmission failed: %s",
        esp_err_to_name(err));

  m_faults.set(
    Fault::Component::PumpController,
    Fault::Code::RadioCommandSendFailed);
}
