#pragma once

#include <cstdint>
#include <driver/pcnt.h>

class FlowSensor {
public:
  static constexpr pcnt_unit_t kPcntUnit = PCNT_UNIT_0;

  // Sensor = pulses per liter
  enum class Type : u_int16_t {
    YFDN50 = 20,
    YFS201 = 450,
    YFB5   = 450,
    YFB1   = 660,
    YFB10  = 352,
    YFS401 = 5880,
    YFB1S  = 1077,

    OF10ZAT = 400,
    OF10ZZT = 400,
    OF05ZAT = 2174,
    OF05ZZT = 2174,
  };

  static constexpr float toPulsesPerLiter(Type type) {
    return static_cast<float>(type);
  }

  // @param ok starts at true and is
  // set to false if an error occured
  struct Reading {
    bool    valid  = true;
    float   flow   = 0.0f;  // in L/min
    int16_t pulses = 0;
  };

  FlowSensor(uint8_t pin, Type type);

  bool begin();

  // Returns filtered flow rate with a zero deadband
  // (L/min). Updates approximately every second.
  Reading getFlow();

private:
  bool getPulseCount(int16_t& count);

  uint8_t m_pin;
  Type    m_type;

  unsigned long m_lastGetPulseCountTime = 0;

  bool    m_hasFlowMeasurement = false;
  float   m_filteredFlow       = 0.0f;
  int16_t m_lastPulseCount     = 0;
};
