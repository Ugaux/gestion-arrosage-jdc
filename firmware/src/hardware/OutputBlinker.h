#pragma once

#include <Arduino.h>

class OutputBlinker {
private:
  unsigned long m_lastLedToggle = 0;
  bool          m_on            = false;
  uint8_t       m_pin;

public:
  OutputBlinker(uint8_t pin) : m_pin(pin) {}

  void begin() {
    pinMode(m_pin, OUTPUT);
  }

  void blink(unsigned long interval) {
    unsigned long now = millis();
    if (now - m_lastLedToggle >= interval) {
      m_on            = !m_on;
      m_lastLedToggle = now;
      digitalWrite(m_pin, m_on);
    }
  }

  void blinkCustom(unsigned long onTime, unsigned long offTime) {
    unsigned long now = millis();

    if (m_on && now - m_lastLedToggle >= onTime) {
      m_on            = false;
      m_lastLedToggle = now;
      digitalWrite(m_pin, LOW);
    } else if (!m_on && now - m_lastLedToggle >= offTime) {
      m_on            = true;
      m_lastLedToggle = now;
      digitalWrite(m_pin, HIGH);
    }
  }

  void setState(bool on) {
    if (m_on == on)
      return;

    m_on = on;

    m_lastLedToggle = millis();
    digitalWrite(m_pin, m_on ? HIGH : LOW);
  }
};
