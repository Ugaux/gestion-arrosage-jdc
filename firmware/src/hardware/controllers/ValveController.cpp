#include "ValveController.h"

#include <Wire.h>
#include "Pinout.h"
#include "core/FaultManager.h"
#include "core/SystemMonitor.h"

ValveController::ValveController(TwoWire &i2c, SystemMonitor &monitor, FaultManager &faults)
  : m_monitor(monitor),
    m_faults(faults),
    m_ioExpander(&i2c, 0x20),
    m_valves{
      Valve(m_ioExpander, Pinout::Expander::Relay1),
      Valve(m_ioExpander, Pinout::Expander::Relay2),
      Valve(m_ioExpander, Pinout::Expander::Relay3),
      Valve(m_ioExpander, Pinout::Expander::Relay4),
      Valve(m_ioExpander, Pinout::Expander::Relay5),
      Valve(m_ioExpander, Pinout::Expander::Relay6),
      Valve(m_ioExpander, Pinout::Expander::Relay7),
      Valve(m_ioExpander, Pinout::Expander::Relay8),
    } {}

void ValveController::begin() {
  log_d("ValveController is initializing...");

  m_failedValves.reset();
  m_desiredOpenValves.reset();
  m_schedulerControlledValves.reset();

  // Registers all PCF8574 pins and their safe initial states
  // before initializing the expander.
  for (uint8_t i = 0; i < kValveCount; i++)
    m_valves[i].begin();

  // This actually initializes I2C and writes the initial
  // PCF8574 output state.
  if (!m_ioExpander.begin()) {
    m_faults.set(
      Fault::Component::ValveController,
      Fault::Code::MissingIOExpander);
    return;
  }

  // Allow the first update() call to execute immediately.
  m_lastTimeoutCheck = millis() - kTimeoutCheckIntervalMs;

  m_initialized = true;
  log_i("ValveController has initialized");
}

void ValveController::update(unsigned long now) {
  if (!m_initialized)
    return;

  // Safety timeout for manual/debug valve opens.
  // Scheduler-controlled valves are not affected by this timeout.
  if (now - m_lastTimeoutCheck >= kTimeoutCheckIntervalMs) {

    for (uint8_t i = 0; i < kValveCount; ++i) {
      if (m_schedulerControlledValves[i])
        continue;

      if (!m_desiredOpenValves[i])
        continue;

      if (now - m_manualOpenSince[i] >= kManualOpenTimeoutMs)
        close(i);
    }

    m_lastTimeoutCheck = now;
  }

  // Limits switching frequency to avoid inrush current / water hammer.
  if (now - m_lastChange >= m_currentDelay) {

    for (uint8_t i = 0; i < kValveCount; ++i) {
      const bool desiredOpen = isOpenDesired(i);

      if (desiredOpen == m_valves[i].isOpen())
        continue;

      if (desiredOpen) {
        if (!m_valves[i].open()) {
          m_failedValves.set(i);
          log_w("Valve index %d failed to open", i);
          continue;
        }
        m_failedValves.reset(i);
        m_currentDelay = kSwitchOnDelayMs;
        log_d("Valve index %d opened", i);
      } else {
        if (!m_valves[i].close()) {
          m_failedValves.set(i);
          log_w("Valve index %d failed to close", i);
          continue;
        }
        m_failedValves.reset(i);
        m_currentDelay = kSwitchOffDelayMs;
        log_d("Valve index %d closed", i);
      }

      m_lastChange = now;
      break;  // only one valve per update
    }

    if (m_failedValves.any())
      m_faults.set(
        Fault::Component::ValveController,
        Fault::Code::OperationFailed);
    else
      m_faults.clear(
        Fault::Component::ValveController,
        Fault::Code::OperationFailed);
  }
}

bool ValveController::isBusy() const {
  for (uint8_t i = 0; i < kValveCount; ++i) {
    if (isOpenDesired(i) != m_valves[i].isOpen())
      return true;
  }

  return false;
}

void ValveController::open(uint8_t valveIndex) {
  if (!m_monitor.canWater()) return;
  if (valveIndex >= kValveCount) return;

  m_desiredOpenValves.set(valveIndex);
  m_manualOpenSince[valveIndex] = millis();
}

void ValveController::close(uint8_t valveIndex) {
  if (valveIndex >= kValveCount) return;

  m_desiredOpenValves.reset(valveIndex);
  m_manualOpenSince[valveIndex] = 0;
}

void ValveController::schedulerOpen(uint8_t valveIndex) {
  if (!m_monitor.canWater()) return;
  if (valveIndex >= kValveCount) return;

  m_schedulerControlledValves.set(valveIndex);
}

void ValveController::schedulerRelease(uint8_t valveIndex) {
  if (valveIndex >= kValveCount) return;

  m_schedulerControlledValves.reset(valveIndex);
}

bool ValveController::isOpen(uint8_t valveIndex) const {
  if (valveIndex >= kValveCount) return false;

  return m_valves[valveIndex].isOpen();
}

void ValveController::testAllValves() {
  unsigned long lastChange = 0;
  bool          opened     = true;

  while (true) {
    const auto now = millis();

    update(now);

    if (now - lastChange < 10000)
      return;

    if (opened)
      for (int i = 0; i < 8; i++) open(i);
    else
      for (int i = 0; i < 8; i++) close(i);

    opened     = !opened;
    lastChange = now;
  }
}

bool ValveController::isOpenDesired(uint8_t valveIndex) const {
  if (valveIndex >= kValveCount) return false;

  // Scheduler has priority
  if (m_schedulerControlledValves[valveIndex])
    return true;

  // Otherwise normal control applies
  return m_desiredOpenValves[valveIndex];
}
