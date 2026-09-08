#pragma once

#include <cstdint>
#include <cstddef>
#include <Arduino.h>

class SoilMoistureSensor {
public:
  static constexpr uint16_t kSoilMoistureSensorAirCal   = 2080;
  static constexpr uint16_t kSoilMoistureSensorWaterCal = 740;

  static constexpr size_t kNumSamples = 30;  // per second
  static constexpr size_t kTrimCount  = 3;

  // @param ok starts at true and is
  // set to false if an error occured
  struct Reading {
    bool     valid      = true;
    float    percentage = 0.0f;  // Soil moisture percentage (0 -> 100%)
    uint16_t rawAdc     = 0;     // Trimmed mean of ADC samples took during sensor reading (before calibration clamping)
  };

  Reading getSoilMoisture();

  // Soil arrives at approximately 20000/2 = 10000 samples/sec.
  // Only about 60 samples/sec are needed: 10000/60 = ~166.67.
  // So approximately every 167th sample is kept.
  void processSample(uint16_t raw);

private:
  // Takes multiple samples and computes a trimmed-mean soil moisture value
  void processSamples();

  // Approximately 1 sample out of every 67.
  uint16_t m_sampleDecimation = 0;

  uint16_t m_samples[kNumSamples] = {};
  size_t   m_sampleCount          = 0;

  Reading      m_reading    = {};
  portMUX_TYPE m_readingMux = portMUX_INITIALIZER_UNLOCKED;
};
