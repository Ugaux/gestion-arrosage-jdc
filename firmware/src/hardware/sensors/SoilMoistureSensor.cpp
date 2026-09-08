#include "SoilMoistureSensor.h"

#include <algorithm>
#include <Arduino.h>

namespace {

float mapFloat(float x, float inMin, float inMax,
               float outMin, float outMax) {
  return (x - inMin) * (outMax - outMin) / (inMax - inMin) + outMin;
}

}  // namespace

SoilMoistureSensor::Reading SoilMoistureSensor::getSoilMoisture() {
  Reading result;

  portENTER_CRITICAL(&m_readingMux);
  result = m_reading;
  portEXIT_CRITICAL(&m_readingMux);

  return result;
}

void SoilMoistureSensor::processSample(uint16_t raw) {

  if (++m_sampleDecimation < 167)
    return;

  m_sampleDecimation = 0;

  // Keep this raw sample
  m_samples[m_sampleCount++] = raw;

  // Do we have our samples?
  if (m_sampleCount < kNumSamples)
    return;

  processSamples();

  m_sampleCount = 0;
}

void SoilMoistureSensor::processSamples() {

  // Sort samples
  std::sort(m_samples, m_samples + m_sampleCount);

  // Discard lowest and highest samples, average the remaining samples
  uint32_t sum = 0;

  for (size_t i = kTrimCount; i < m_sampleCount - kTrimCount; ++i)
    sum += m_samples[i];

  const uint16_t rawTrimmedMean = sum / (m_sampleCount - 2 * kTrimCount);

  constexpr uint16_t minCal =
    std::min(kSoilMoistureSensorWaterCal,
             kSoilMoistureSensorAirCal);
  constexpr uint16_t maxCal =
    std::max(kSoilMoistureSensorWaterCal,
             kSoilMoistureSensorAirCal);

  // ADC value below 50% of the minimum calibration value indicates
  // a likely disconnected or invalid sensor.
  if (rawTrimmedMean < minCal / 2) {
    Reading newReading = { .valid = false };

    portENTER_CRITICAL(&m_readingMux);
    m_reading = newReading;
    portEXIT_CRITICAL(&m_readingMux);

    return;
  }

  const float clampedPercentage = mapFloat(
    constrain(rawTrimmedMean, minCal, maxCal),
    kSoilMoistureSensorAirCal,
    kSoilMoistureSensorWaterCal,
    0.0f,
    100.0f);

  // Publish measurement
  Reading newReading = {
    .valid      = true,
    .percentage = clampedPercentage,
    .rawAdc     = rawTrimmedMean
  };

  portENTER_CRITICAL(&m_readingMux);
  m_reading = newReading;
  portEXIT_CRITICAL(&m_readingMux);
}
