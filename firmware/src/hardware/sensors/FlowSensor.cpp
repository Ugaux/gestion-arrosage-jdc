#include "FlowSensor.h"

#include <Arduino.h>
#include "Pinout.h"

FlowSensor::FlowSensor(uint8_t pin, Type type)
  : m_pin(pin), m_type(type) {}

bool FlowSensor::begin() {
  pcnt_config_t pcnt_config = {
    .pulse_gpio_num = static_cast<int>(m_pin),
    .ctrl_gpio_num  = PCNT_PIN_NOT_USED,

    .lctrl_mode = PCNT_MODE_KEEP,
    .hctrl_mode = PCNT_MODE_KEEP,

    .pos_mode = PCNT_COUNT_INC,
    .neg_mode = PCNT_COUNT_DIS,

    .counter_h_lim = 32767,
    .counter_l_lim = 0,

    .unit    = kPcntUnit,
    .channel = PCNT_CHANNEL_0,
  };

  if (pcnt_unit_config(&pcnt_config) == ESP_OK
      // PCNT owns the pin from here

      // Optional glitch filter.
      // 1000 APB clock cycles ≈ 12.5 us at 80 MHz.
      && pcnt_set_filter_value(kPcntUnit, 1000) == ESP_OK
      && pcnt_filter_enable(kPcntUnit) == ESP_OK

      // Reset counter
      && pcnt_counter_pause(kPcntUnit) == ESP_OK
      && pcnt_counter_clear(kPcntUnit) == ESP_OK

      // Start counting
      && pcnt_counter_resume(kPcntUnit) == ESP_OK)
    return true;

  return false;
}

FlowSensor::Reading FlowSensor::getFlow() {
  unsigned long now       = micros();
  unsigned long elapsedUs = now - m_lastGetPulseCountTime;

  // Only calculate flow if the measurement
  // interval is long enough (1000ms here)
  if (elapsedUs >= 1000000) {

    if (!getPulseCount(m_lastPulseCount))
      return { .valid = false };

    m_lastGetPulseCountTime = now;

    // Pulses / sensor pulses per liter = liters measured
    // 60,000,000 us = one minute
    const float newFlow =
      (m_lastPulseCount / toPulsesPerLiter(m_type))
      * (60000000.0f / elapsedUs);

    if (m_hasFlowMeasurement) {
      // New filtered flow
      if (m_lastPulseCount == 0)
        m_filteredFlow = 0.0f;
      else m_filteredFlow = 0.2f * m_filteredFlow
                            + 0.8f * newFlow;
    } else {
      m_filteredFlow       = newFlow;
      m_hasFlowMeasurement = true;
    }
  }

  return {
    .valid  = true,
    .flow   = m_filteredFlow,
    .pulses = m_lastPulseCount,
  };
}

bool FlowSensor::getPulseCount(int16_t& count) {

  if (pcnt_get_counter_value(kPcntUnit, &count) == ESP_OK
      && pcnt_counter_clear(kPcntUnit) == ESP_OK)
    return true;

  return false;
}
