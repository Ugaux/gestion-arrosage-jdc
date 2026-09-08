#include "SensorController.h"

#include <Arduino.h>
#include "Pinout.h"

SensorController::SensorController(FaultManager &faults)
  : m_faults(faults),
    m_adc(faults),
    m_flow(Pinout::ESP::Sensor::Flow,
           FlowSensor::Type::YFB10) {}

void SensorController::begin() {
  log_d("SensorController is initializing...");

  if (!onSensorValueChange) {
    log_e("onSensorValueChange callback is not configured!");
    while (true) delay(1000);
  }

  pinMode(Pinout::ESP::Sensor::WaterTank::HighLevel, INPUT);
  pinMode(Pinout::ESP::Sensor::WaterTank::LowLevel, INPUT);
  pinMode(Pinout::ESP::Sensor::WaterTank::Filling, INPUT);

  AdcSampler::ChannelConfig adcChannelConfig[] = {
    { .pin         = Pinout::ESP::Sensor::SoilMoisture,
      .attenuation = ADC_ATTEN_DB_12,
      .bitsWidth   = ADC_WIDTH_BIT_12 },
    { .pin         = Pinout::ESP::Sensor::PumpCurrent,
      .attenuation = ADC_ATTEN_DB_12,
      .bitsWidth   = ADC_WIDTH_BIT_12 },
  };
  AdcSampler::Config adcConfig = {
    .channels     = adcChannelConfig,
    .channelCount = 2,
  };
  if (!m_adc.begin(
        adcConfig,
        [this](
          const adc_digi_output_data_t *samples,
          size_t                        sampleCount) {
          processSamples(samples, sampleCount);
        })) {
    m_faults.set(Fault::Component::SensorController,
                 Fault::Code::AdcSamplerInitFailed);
    return;
  }

  if (!m_flow.begin()) {
    m_faults.set(Fault::Component::SensorController,
                 Fault::Code::FlowSensorInitFailed);
    return;
  }

  // Allow the first update() call to execute immediately.
  m_lastUpdateTime = millis() - kUpdateDelayMs;

  m_initialized = true;
  log_i("SensorController has initialized");
}

void SensorController::update(unsigned long now) {
  if (!m_initialized)
    return;

  if (now - m_lastUpdateTime < kUpdateDelayMs)
    return;

  m_lastUpdateTime = now;

  updateSensorReadings();
}

void SensorController::updateSensorReadings() {
  ACSensor::Reading acCurrentReading =
    m_current.getCurrent();
  updateReadingFault(
    acCurrentReading.valid,
    Fault::Code::PumpCurrentReadingFailed,
    m_currentFault);
  if (acCurrentReading.valid) {
    ReportedValue r = processValue(
      acCurrentReading.rmsCurrent,
      m_lastValues.pumpCurrent,
      0.0f, 30.0f, 0.2f, 0.1f);
    if (r.changed) {
      log_d("%s pump current changed: DC_bias=%dmV,"
            " RMS_V=%0.4fV, RMS_A=%0.2fA",
            kTag,
            acCurrentReading.dcBias,
            acCurrentReading.rmsVoltage,
            r.value);
      m_lastValues.pumpCurrent = r.value;
      onSensorValueChange({ SensorId::PumpCurrent,
                            r.value });
    }
  }

  FlowSensor::Reading flowReading =
    m_flow.getFlow();
  updateReadingFault(
    flowReading.valid,
    Fault::Code::FlowReadingFailed,
    m_flowFault);
  if (flowReading.valid) {
    ReportedValue r = processValue(
      flowReading.flow,
      m_lastValues.flowRate,
      0.0f, NAN, 0.2f, 0.5f);
    if (r.changed) {
      log_d("%s Flow rate changed:"
            " raw=%.2f/reported=%.2f L/min (%d pulses)",
            kTag,
            flowReading.flow,
            r.value,
            flowReading.pulses);
      m_lastValues.flowRate = r.value;
      onSensorValueChange({ SensorId::FlowRate,
                            r.value });
    }
  }

  SoilMoistureSensor::Reading soilMoistureReading =
    m_soil.getSoilMoisture();
  updateReadingFault(
    soilMoistureReading.valid,
    Fault::Code::SoilMoistureReadingFailed,
    m_soilFault);
  if (soilMoistureReading.valid) {
    ReportedValue r = processValue(
      soilMoistureReading.percentage,
      m_lastValues.soilMoisture,
      0.0f, 100.0f, 0.2f, 0.5f);
    if (r.changed) {
      log_d("%s soil moisture changed: raw=%.2f/reported=%.2f %%"
            " (ADC %d, clamped to [%d=dry, %d=wet])",
            kTag,
            soilMoistureReading.percentage,
            r.value,
            soilMoistureReading.rawAdc,
            SoilMoistureSensor::kSoilMoistureSensorAirCal,
            SoilMoistureSensor::kSoilMoistureSensorWaterCal);
      m_lastValues.soilMoisture = r.value;
      onSensorValueChange({ SensorId::SoilMoisture,
                            r.value });
    }
  }

  bool waterTankHighLevel =
    digitalRead(Pinout::ESP::Sensor::WaterTank::HighLevel)
    == LOW;
  if (waterTankHighLevel != m_lastValues.waterTank.high) {
    log_d("%s water tank high level changed: %d",
          kTag, waterTankHighLevel);
    m_lastValues.waterTank.high = waterTankHighLevel;
    onSensorValueChange(
      { SensorId::WaterTankHigh,
        static_cast<float>(waterTankHighLevel) });
  }

  bool waterTankLowLevel =
    digitalRead(Pinout::ESP::Sensor::WaterTank::LowLevel)
    == HIGH;
  if (waterTankLowLevel != m_lastValues.waterTank.low) {
    log_d("%s water tank low level changed: %d",
          kTag, waterTankLowLevel);
    m_lastValues.waterTank.low = waterTankLowLevel;
    onSensorValueChange(
      { SensorId::WaterTankLow,
        static_cast<float>(waterTankLowLevel) });
  }

  bool waterTankFilling =
    digitalRead(Pinout::ESP::Sensor::WaterTank::Filling)
    == HIGH;
  if (waterTankFilling != m_lastValues.waterTank.filling) {
    log_d("%s water tank filling changed: %d",
          kTag, waterTankFilling);
    m_lastValues.waterTank.filling = waterTankFilling;
    onSensorValueChange(
      { SensorId::WaterTankFilling,
        static_cast<float>(waterTankFilling) });
  }
}

void SensorController::updateReadingFault(
  bool valid, Fault::Code code, ReadingFaultState &state) {

  if (!valid) {
    state.successes = 0;

    if (state.active)
      return;

    if (++state.failures >= kFaultAfterErrors) {
      m_faults.set(
        Fault::Component::SensorController,
        code);

      state.active   = true;
      state.failures = 0;
    }

    return;
  }

  state.failures = 0;

  if (!state.active)
    return;

  if (++state.successes >= kClearAfterSuccesses) {
    m_faults.clear(
      Fault::Component::SensorController,
      code);

    state.active    = false;
    state.successes = 0;
  }
}

SensorController::ReportedValue SensorController::processValue(
  float value,
  float lastValue,
  float min,
  float max,
  float snapDistance,
  float threshold) {

  float reportedValue = value;

  if (!isnan(min) && value <= min + snapDistance) {
    reportedValue = min;
  } else if (!isnan(max) && value >= max - snapDistance) {
    reportedValue = max;
  }

  const bool exceededThreshold =
    fabsf(reportedValue - lastValue) > threshold;

  const bool enteredBoundary =
    (!isnan(min) && reportedValue == min && lastValue != min)
    || (!isnan(max) && reportedValue == max && lastValue != max);

  return {
    .value   = reportedValue,
    .changed = exceededThreshold
               || enteredBoundary
  };
}

void SensorController::processSamples(
  const adc_digi_output_data_t *samples,
  size_t                        sampleCount) {

  for (size_t i = 0; i < sampleCount; ++i) {

    const auto &sample = samples[i].type1;

    auto pin = analogChannelToDigitalPin(sample.channel);

    if (pin == Pinout::ESP::Sensor::PumpCurrent) {
      uint32_t voltage = m_adc.convertRawToVoltage(pin, sample.data);
      m_current.processVoltage(voltage);
    } else if (pin == Pinout::ESP::Sensor::SoilMoisture) {
      m_soil.processSample(sample.data);
    }
  }
}
