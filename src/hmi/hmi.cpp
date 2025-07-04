#include "hmi.h"

#include <ezButton.h>

#include "oled/oled.h"
#include "flow/flow.h"
#include "watering/watering.h"
#include "humidity/humidity.h"

Hmi::Hmi() : m_state(IDLE),
             m_manualWatering(0),
             m_time(0) {
}

ezButton functionButton(FUNCTION_BUTTON);
ezButton manualButton(MANUAL_BUTTON);

void Hmi::setup(void) {
  functionButton.setDebounceTime(50);
  manualButton.setDebounceTime(50);
  manualReleasedHandled = false;
  //  functionButton.setCountMode(COUNT_FALLING);
}

void Hmi::displayDefaults(void) {
  display.clearLine(0);
  display.clearLine(1);
  display.clearLine(2);
  int moisture;
  getSoilMoisture(&moisture);  // just display to terminal
  display.displayMoisture(moisture);
  float flow = getFlow();
  // display.displayFlow(flow);
}

void Hmi::displayIPAndNextWatering(void) {
  display.clearLine(1);
  display.clearLine(2);
  display.displayIP();
  m_time  = millis();
  m_state = DISPLAY_IP;
}

void Hmi::displayManual(void) {
  display.clearLine(1);
  display.clearLine(2);
  m_time = millis();
  if (m_manualWatering == 0) {
    m_manualWatering = Way::getFirst();
  } else {
    m_manualWatering = Way::getNext();
    if (m_manualWatering == 0) {
      m_manualWatering = Way::getFirst();
    }
  }
  display.displayManualWatering(m_manualWatering->getName(), m_manualWatering->manualStarted(NULL));
  m_state = DISPLAY_MANUAL;
}

void Hmi::run(void) {
  uint8_t btn;
  //  time_t t = getCurrentTime(); // t = heure actuelle
  time_t      t;
  const char *way;

  functionButton.loop();
  manualButton.loop();
  switch (m_state) {
    case IDLE:
      if (manualButton.isReleased()) {
        Serial.printf("# MANUAL BTN PRESSED\n");
        displayManual();
      }
      if (functionButton.isReleased()) {
        Serial.printf("# FUNCTION BTN PRESSED\n");
        displayIPAndNextWatering();
      }
      break;
    case DISPLAY_IP:
      if (millis() - m_time > 2000) {
        display.clearLine(0);
        display.clearLine(1);
        display.clearLine(2);
        way = Watering::getNextWateringTime(&t);
        if (way) {
          display.displayNextWatering(way, t);
        }
        m_time  = millis();
        m_state = DISPLAY_NEXTWATERING;
      }
      break;
    case DISPLAY_NEXTWATERING:
      if (millis() - m_time > 2000) {
        displayDefaults();
        m_state = IDLE;
      }
      break;
    case DISPLAY_MANUAL:
      if (millis() - m_time > 4000) {
        displayDefaults();
        m_state          = IDLE;
        m_manualWatering = 0;
      }
      // Call displayManual() every time the button is released, but only once per release
      if (manualButton.isReleased() && !manualReleasedHandled) {
        displayManual();               // Update the display in response to button release
        manualReleasedHandled = true;  // Prevent further calls for the same release
      }

      // Reset when the button is pressed again
      if (manualButton.isPressed()) {
        manualReleasedHandled = false;
      }
      if (functionButton.isReleased()) {
        if (m_manualWatering->manualStarted(NULL)) {
          Serial.println("STOP");
          m_manualWatering->manualStop();
        } else {
          Serial.println("START");
          m_manualWatering->manualStart(Watering::manualDuration());
        }
        displayDefaults();
        m_state          = IDLE;
        m_manualWatering = 0;
      }
      break;
  }
}

bool Hmi::isBusy(void) {
  return m_state != IDLE;
}
