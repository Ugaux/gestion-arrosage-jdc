#include "BoxLedController.h"

#include <Arduino.h>
#include "Pinout.h"

void BoxLedController::begin() {
  log_d("BoxLedController is initializing...");

  pinMode(Pinout::ESP::BoxLed::Status, OUTPUT);
  pinMode(Pinout::ESP::BoxLed::Fault, OUTPUT);
  pinMode(Pinout::ESP::BoxLed::Watering, OUTPUT);

  // Initialize physical outputs to a known safe state.
  digitalWrite(Pinout::ESP::BoxLed::Status, LOW);
  digitalWrite(Pinout::ESP::BoxLed::Fault, LOW);
  digitalWrite(Pinout::ESP::BoxLed::Watering, LOW);

  log_i("BoxLedController has initialized");
}

void BoxLedController::update(unsigned long now) {
  updateStatusLedState(now);
}

void BoxLedController::setStatus(StatusMode status) {
  if (status == m_targetStatusMode)
    return;

  m_targetStatusMode = status;
}

void BoxLedController::setFault(bool on) {
  if (on == m_faultLedState)
    return;

  m_faultLedState = on;
  log_d("Fault led set to %s", on ? "on" : "off");
  digitalWrite(Pinout::ESP::BoxLed::Fault, on ? HIGH : LOW);
}

void BoxLedController::setWatering(bool on) {
  if (on == m_wateringLedState)
    return;

  m_wateringLedState = on;
  log_d("Watering led set to %s", on ? "on" : "off");
  digitalWrite(Pinout::ESP::BoxLed::Watering, on ? HIGH : LOW);
}

void BoxLedController::updateStatusLedState(unsigned long now) {
  switch (m_currentStatusMode) {

    case StatusMode::NotRunning:
    case StatusMode::Running:
      if (isStatusModeChangePending())
        transitionToTargetStatusMode(now);
      // Nothing to animate.
      break;

    case StatusMode::StartingAP:
      updateStatusLedApState(now);
      break;

    case StatusMode::ConnectingSTA:
      updateStatusLedStaState(now);
      break;
  }
}

void BoxLedController::updateStatusLedApState(unsigned long now) {
  unsigned long elapsed = now - m_lastStatusLedUpdate;

  switch (m_statusLedState) {

    case StatusLedState::Pulse1On:
      if (elapsed >= kApPulseOnMs) {
        transitionToStatusLedState(StatusLedState::Pulse1Off, now);
      }
      break;

    case StatusLedState::Pulse1Off:
      if (elapsed >= kApPulseOffMs) {
        transitionToStatusLedState(StatusLedState::Pulse2On, now);
      }
      break;

    case StatusLedState::Pulse2On:
      if (elapsed >= kApPulseOnMs) {
        transitionToStatusLedState(StatusLedState::PulsePause, now);
      }
      break;

    case StatusLedState::PulsePause:
      if (elapsed >= kApPauseMs) {
        if (isStatusModeChangePending()) {
          transitionToTargetStatusMode(now);
          break;
        }
        transitionToStatusLedState(StatusLedState::Pulse1On, now);
      }
      break;

    default:
      break;
  }
}

void BoxLedController::updateStatusLedStaState(unsigned long now) {
  unsigned long elapsed = now - m_lastStatusLedUpdate;

  switch (m_statusLedState) {

    case StatusLedState::BlinkOn:
      if (elapsed >= kStaBlinkDelayMs) {
        transitionToStatusLedState(StatusLedState::BlinkOff, now);
      }
      break;

    case StatusLedState::BlinkOff:
      if (elapsed >= kStaBlinkDelayMs) {
        if (isStatusModeChangePending()) {
          transitionToTargetStatusMode(now);
          break;
        }
        transitionToStatusLedState(StatusLedState::BlinkOn, now);
      }
      break;

    default:
      break;
  }
}

void BoxLedController::transitionToTargetStatusMode(unsigned long now) {
  m_currentStatusMode = m_targetStatusMode;

  switch (m_currentStatusMode) {

    case StatusMode::NotRunning:
      log_d("Status LED turned off");
      transitionToStatusLedState(StatusLedState::SolidOff, now);
      break;

    case StatusMode::Running:
      log_d("Status LED turned on");
      transitionToStatusLedState(StatusLedState::SolidOn, now);
      break;

    case StatusMode::StartingAP:
      log_d("Status LED set to pulsing mode");
      transitionToStatusLedState(StatusLedState::Pulse1On, now);
      break;

    case StatusMode::ConnectingSTA:
      log_d("Status LED set to blinking mode");
      transitionToStatusLedState(StatusLedState::BlinkOn, now);
      break;
  }
}

void BoxLedController::transitionToStatusLedState(StatusLedState state, unsigned long now) {
  if (state == m_statusLedState)
    return;

  m_statusLedState      = state;
  m_lastStatusLedUpdate = now;

  switch (state) {

    case StatusLedState::SolidOn:
    case StatusLedState::Pulse1On:
    case StatusLedState::Pulse2On:
    case StatusLedState::BlinkOn:
      digitalWrite(Pinout::ESP::BoxLed::Status, HIGH);
      break;

    case StatusLedState::SolidOff:
    case StatusLedState::Pulse1Off:
    case StatusLedState::PulsePause:
    case StatusLedState::BlinkOff:
      digitalWrite(Pinout::ESP::BoxLed::Status, LOW);
      break;
  }
}

bool BoxLedController::isStatusModeChangePending() const {
  return m_targetStatusMode != m_currentStatusMode;
}
