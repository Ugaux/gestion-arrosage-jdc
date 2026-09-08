#include "Valve.h"

#include <PCF8574.h>

Valve::Valve(PCF8574& io, uint8_t pin)
  : m_io(io), m_pin(pin) {}

void Valve::begin() {
  // Registers safe startup state with the PCF8574.
  // Actual I2C transaction happens when the PCF8574 is
  // initialized by ValveController.
  m_io.pinMode(m_pin, OUTPUT, kRelayOff);
}

bool Valve::open() {
  if (!m_io.digitalWrite(m_pin, kRelayOn)) {
    log_w("Failed to open valve at pin %d", m_pin);
    return false;
  }
  m_opened = true;
  return true;
}

bool Valve::close() {
  if (!m_io.digitalWrite(m_pin, kRelayOff)) {
    log_w("Failed to close valve at pin %d", m_pin);
    return false;
  }
  m_opened = false;
  return true;
}
