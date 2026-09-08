#include "ACSensor.h"

#include <cmath>

ACSensor::Reading ACSensor::getCurrent() {
  Reading result;

  portENTER_CRITICAL(&m_readingMux);
  result = m_reading;
  portEXIT_CRITICAL(&m_readingMux);

  return result;
}

void ACSensor::processVoltage(uint32_t voltage) {
  m_rmsSumVoltage += static_cast<double>(voltage);
  m_rmsSumSquaredVoltage +=
    static_cast<double>(voltage)
    * static_cast<double>(voltage);

  m_rmsSampleCount += 1;

  // Do we have a complete 1600-sample window?
  if (m_rmsSampleCount < kRmsWindowSamples)
    return;

  processRMS();

  // Reset accumulators for the next sample window
  m_rmsSumVoltage        = 0.0;
  m_rmsSumSquaredVoltage = 0.0;

  m_rmsSampleCount = 0;
}

void ACSensor::processRMS() {
  // Calculate average/DC voltage
  const double dcBias =
    m_rmsSumVoltage
    / static_cast<double>(kRmsWindowSamples);

  // Calculate AC RMS:
  //  RMS = sqrt(mean(V²) - mean(V)²))
  double variance =
    (m_rmsSumSquaredVoltage
     / static_cast<double>(kRmsWindowSamples))
    - (dcBias * dcBias);

  // Protect against tiny floating-point
  // negative values
  if (variance < 0.0)
    variance = 0.0;

  // Convert calibrated ADC difference to RMS voltage
  // (this is an approximation because ADC calibration is
  // not perfectly linear)
  const float rms_mV = sqrtf(
    static_cast<float>(variance));

  const float rmsVoltage = rms_mV / 1000.0f;

  // Publish measurement
  Reading newReading = {
    .valid      = true,
    .dcBias     = static_cast<uint32_t>(dcBias),
    .rmsVoltage = rmsVoltage,
    .rmsCurrent = rmsVoltage * kSensorCurrentPerVolt,
  };

  portENTER_CRITICAL(&m_readingMux);
  m_reading = newReading;
  portEXIT_CRITICAL(&m_readingMux);
}
