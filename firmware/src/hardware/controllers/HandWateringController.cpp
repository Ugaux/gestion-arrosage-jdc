#include "HandWateringController.h"

#include "Pinout.h"
#include "hardware/controllers/PumpController.h"

HandWateringController::HandWateringController(PumpController &pumps)
  : m_pumps(pumps),
    m_button(
      Pinout::ESP::HandWatering::ButtonInput,
      INTERNAL_PULLUP),
    m_buttonLedBlinker(Pinout::ESP::HandWatering::ButtonLed) {}

void HandWateringController::begin(bool testing) {
  log_d("HandWateringController is initializing...");

  // Button init
  m_button.setDebounceTime(40);
  // Led init
  m_buttonLedBlinker.begin();
  // Valve init
  pinMode(Pinout::ESP::HandWatering::Valve, OUTPUT);
  digitalWrite(Pinout::ESP::HandWatering::Valve, LOW);

  if (testing)
    m_triggerTimes = {
      3 * kSeconds,
      7 * kSeconds,
      10 * kSeconds
    };

  log_i("HandWateringController has initialized");
}

void HandWateringController::update(unsigned long now) {
  unsigned long elapsed = now - m_cycleStartTime;

  m_button.loop();

  switch (m_state) {

    case SystemState::Idle:

      if (m_button.isReleased()) {
        log_i("%s button pressed, system turned ON", kTag);
        m_cycleStartTime = now;

        m_pumps.requestOn(PumpController::Type::Watering);
        m_buttonLedBlinker.setState(true);
        digitalWrite(Pinout::ESP::HandWatering::Valve, HIGH);

        m_state = SystemState::FirstPhase;
      }

      break;

    case SystemState::FirstPhase:

      if (m_button.isReleased()) {
        log_i("%s button pressed (during 1st phase), system turned OFF", kTag);
        turnAllOFF();
        break;
      }

      if (elapsed >= m_triggerTimes[0]) {
        log_i("%s entering second phase...", kTag);
        m_state = SystemState::SecondPhase;
      }

      break;

    case SystemState::SecondPhase:
      m_buttonLedBlinker.blinkCustom(200, 800);

      if (m_button.isReleased()) {
        log_i("%s button pressed (during 2nd phase), system turned OFF", kTag);
        turnAllOFF();
        break;
      }

      if (elapsed >= m_triggerTimes[1]) {
        log_i("%s entering last phase...", kTag);
        m_state = SystemState::LastPhase;
      }

      break;

    case SystemState::LastPhase:
      m_buttonLedBlinker.blinkCustom(250, 250);

      if (m_button.isReleased()) {
        log_i("%s button pressed (during last phase), system turned OFF", kTag);
        turnAllOFF();
        break;
      }

      if (elapsed >= m_triggerTimes[2]) {
        log_i("%s timer finished, system turned OFF", kTag);
        turnAllOFF();
      }

      break;
  }
}

void HandWateringController::turnAllOFF() {

  m_pumps.requestOff(PumpController::Type::Watering);
  m_buttonLedBlinker.setState(false);
  digitalWrite(Pinout::ESP::HandWatering::Valve, LOW);

  m_state = SystemState::Idle;
}
