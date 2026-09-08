#pragma once

#include <functional>
#include "core/FaultManager.h"
#include "hardware/AdcSampler.h"
#include "hardware/sensors/FlowSensor.h"
#include "hardware/sensors/ACSensor.h"
#include "hardware/sensors/SoilMoistureSensor.h"

class SensorController {
public:
  static constexpr unsigned long kUpdateDelayMs = 500;

  static constexpr char kTag[] = "[Sensors]";

  enum class SensorId : uint8_t {
    FlowRate = 0,
    PumpCurrent,
    SoilMoisture,
    WaterTankHigh,
    WaterTankLow,
    WaterTankFilling,
  };

  struct SensorChange {
    SensorId sensor;
    float    value;
  };

  SensorController(FaultManager &faults);

  std::function<void(const SensorChange &)> onSensorValueChange;

  void begin();
  // Change detection + throttling: only send value if
  // changed and never more often than every UpdateDelayMs
  void update(unsigned long now);

  // Returns the flow in L/min
  float getFlowRate() const { return m_lastValues.flowRate; }
  // Returns the soil moisture in %
  float getSoilMoisture() const { return m_lastValues.soilMoisture; }

  // Returns true if water is at high level.
  // No software filtering; XKC-Y25-V sensor has a ~500 ms response delay.
  bool waterTankHighLevel() const {
    return m_lastValues.waterTank.high;
  }
  // Returns true if water is at low level.
  // No software filtering; XKC-Y25-V sensor has a ~500 ms response delay.
  bool waterTankLowLevel() const {
    return m_lastValues.waterTank.low;
  }
  // Returns true if water is filling tank.
  // No software filtering; XKC-Y26-V sensor has a ~500 ms response delay.
  bool waterTankFilling() const {
    return m_lastValues.waterTank.filling;
  }

private:
  static constexpr uint8_t kFaultAfterErrors    = 3;
  static constexpr uint8_t kClearAfterSuccesses = 10;

  struct lastValues {
    struct WaterTank {
      bool high    = false;
      bool low     = false;
      bool filling = false;
    } waterTank;
    float flowRate     = 0.0f;
    float soilMoisture = 0.0f;
    float pumpCurrent  = 0.0f;
  };

  struct ReportedValue {
    float value;
    bool  changed;
  };

  struct ReadingFaultState {
    uint8_t failures  = 0;
    uint8_t successes = 0;
    bool    active    = false;
  };

  // Updates all sensor readings and handles value changes with min/max
  // transitions (to prevent values from getting stuck near the limits)
  void updateSensorReadings();
  void updateReadingFault(
    bool valid, Fault::Code code, ReadingFaultState &state);

  // Processes a sensor value by snapping it to the configured min/max
  // when it is within minMaxDist of either boundary, then determines
  // whether the reported value should be considered changed.
  //
  // @param value        Current raw value.
  // @param lastValue    Previously reported value.
  // @param min          Minimum boundary, or NAN if not applicable.
  // @param max          Maximum boundary, or NAN if not applicable.
  // @param minMaxDist   Distance from a boundary within which the value
  //                     is snapped to that boundary.
  // @param threshold    Minimum change required to report a new value.
  ReportedValue processValue(
    float value,
    float lastValue,
    float min,
    float max,
    float snapDistance,
    float threshold);

  // Called around 20 times per sec at 20 kHz sample rate
  void processSamples(
    const adc_digi_output_data_t *samples,
    size_t                        sampleCount);

  FaultManager &m_faults;

  AdcSampler         m_adc;
  ACSensor           m_current;
  SoilMoistureSensor m_soil;
  FlowSensor         m_flow;

  bool m_initialized = false;

  ReadingFaultState m_currentFault;
  ReadingFaultState m_soilFault;
  ReadingFaultState m_flowFault;

  unsigned long m_lastUpdateTime = 0;

  lastValues m_lastValues;

  uint32_t m_adcErrorCount  = 0;
  bool     m_adcFaultActive = false;
};
