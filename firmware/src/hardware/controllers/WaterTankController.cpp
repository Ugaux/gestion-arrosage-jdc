#include "WaterTankController.h"

#include <Arduino.h>
#include "hardware/controllers/PumpController.h"
#include "hardware/controllers/SensorController.h"

WaterTankController::WaterTankController(
  FaultManager&     faults,
  PumpController&   pumps,
  SensorController& sensors)
  : m_faults(faults),
    m_pumps(pumps),
    m_sensors(sensors) {}

void WaterTankController::begin() {
  log_d("WaterTankController is initializing...");

  const auto now = millis();

  // Allow the first update() call to execute immediately.
  m_lastUpdateTime = now - kUpdateDelayMs;

  transitionTo(State::Idle, now);

  log_i("WaterTankController has initialized");
}

void WaterTankController::update(unsigned long now) {
  if (now - m_lastUpdateTime < kUpdateDelayMs)
    return;

  m_lastUpdateTime = now;

  const bool highLevelDetected = m_sensors.waterTankHighLevel();
  const bool lowLevelDetected  = m_sensors.waterTankLowLevel();

  if (highLevelDetected && !lowLevelDetected) {
    m_level = Level::Unknown;
    // Sensor readings are inconsistent; enter fault and abort this update.
    // Do not make any control decisions based on invalid sensor data.
    enterFault(Fault::Code::InconsistentLevelSensors, now);
    return;
  }

  const Level previousLevel = m_level;

  m_level = highLevelDetected  ? Level::Full
            : lowLevelDetected ? Level::Partial
                               : Level::Empty;

  const bool fillingWaterDetected = m_sensors.waterTankFilling();

  switch (m_state) {

    case State::Uninitialized:
      // Only here at startup
      break;

    case State::Idle:
      if (fillingWaterDetected) {
        enterFault(Fault::Code::FillingWaterWhilePumpOff, now);
        break;
      }
      if (m_level == Level::Empty) {
        log_i("Tank is empty");
        transitionTo(State::Filling, now);
      }
      break;

    case State::Filling:
      {
        const auto fillingTimeMs = now - m_fillingStartTime;
        if (!fillingWaterDetected
            && fillingTimeMs >= kNoWaterTimeoutMs) {
          // Water flow was not established in time.
          enterFault(Fault::Code::NoWaterWhileFilling, now);
          break;
        }
        if (m_level == Level::Full) {
          // Water tank is now full.
          log_i("Tank is full");
          transitionTo(State::PumpCooldown, now);
          break;
        }
        if (fillingTimeMs >= kMaxFillingTimeMs) {
          // Tank not full after 30 minutes:
          // stop filling and let the pump cool down
          log_w("Tank is not full after %f minutes",
                kMaxFillingTimeMs / 60000.0);
          transitionTo(State::PumpCooldown, now);
        }
        break;
      }

    case State::PumpCooldown:
      if (fillingWaterDetected) {
        enterFault(Fault::Code::FillingWaterWhilePumpOff, now);
        break;
      }
      if (now - m_pumpCooldownStartTime >= kPumpCooldownDelayMs) {
        log_i("Pump cooldown complete");
        transitionTo(State::Idle, now);
      }
      break;

    case State::Fault:
      // Stay here until resume() is called.
      break;
  }

  if (m_level == Level::Empty) {

    if (!m_pumps.isWateringBlocked())
      m_pumps.blockWatering(
        PumpController::WateringBlockReason::TankEmpty);
  }

  else {

    if (previousLevel == Level::Empty)
      // Water has become available again: reset hysteresis delay.
      m_waterAvailableSince = now;

    if (m_pumps.isWateringBlocked()
        && now - m_waterAvailableSince >= kAllowWateringDelayMs)
      m_pumps.allowWatering();
  }
}

bool WaterTankController::startFilling() {
  if (m_state != State::Idle)
    return false;

  if (m_level == Level::Full)
    return false;

  log_i("Manual filling started");
  transitionTo(State::Filling, millis());

  return true;
}

bool WaterTankController::resume() {
  if (m_state != State::Fault)
    return false;

  log_i("Operation resumed");
  transitionTo(State::Idle, millis());

  return true;
}

void WaterTankController::enterFault(Fault::Code code, unsigned long now) {
  m_faults.set(Fault::Component::WaterTankController, code);

  transitionTo(State::Fault, now);
}

void WaterTankController::transitionTo(State state, unsigned long now) {
  if (state == m_state)
    return;

  m_state = state;
  onEnterState(state, now);
}

void WaterTankController::onEnterState(State state, unsigned long now) {
  switch (state) {

    case State::Uninitialized:
      break;

    case State::Filling:
      log_i("Water tank is getting filled...");
      m_fillingStartTime = now;
      m_pumps.requestOn(PumpController::Type::WaterTank);
      break;

    case State::Idle:
      log_i("Water tank is idling...");
      m_pumps.requestOff(PumpController::Type::WaterTank);
      break;

    case State::PumpCooldown:
      log_i("Water tank pump is cooling...");
      m_pumpCooldownStartTime = now;
      m_pumps.requestOff(PumpController::Type::WaterTank);
      break;

    case State::Fault:
      log_i("Water tank entered fault state!");
      m_pumps.requestOff(PumpController::Type::WaterTank);
      break;
  }
}
