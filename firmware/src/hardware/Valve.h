#pragma once

#include <cstdint>
#include <Arduino.h>

class PCF8574;

class Valve {
public:
  Valve(PCF8574& io, uint8_t pin);

  void begin();

  bool open();
  bool close();
  bool isOpen() const { return m_opened; };

private:
  static constexpr uint8_t kRelayOn  = LOW;
  static constexpr uint8_t kRelayOff = HIGH;

  PCF8574&      m_io;
  const uint8_t m_pin;

  bool m_opened = false;
};
