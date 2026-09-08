/*
  Allow current measuring using a SCT-013-030 30A/1V, 
  it requires using an ADC1 input.
  The ADC input should be biased around 1.65 V.
  We DON'T assume that the bias is exactly 1.650 V.
  Instead, the code calculates the mean of every block and
  subtracts it before calculating RMS.

  IMPORTANT:
  With a 1V RMS SCT signal: 1V RMS = 1.414V peak.
  At 1.65V bias: minimum ~= 0.236V and maximum ~= 3.064V.
  This is close to the upper end of the ESP32 ADC input range.
  But in our case, the current sensor won't measure more than
  10-15 amp for a few ms, stabilizing around 5A continuous
  afterward, so it's more than fine.
  
  Acquisition (ESP-IDF 4.4.7):
    I2S0
    ADC DMA

  No application polling:
    i2s_read() blocks until DMA data is available.

  Target: Classic ESP32

  Author: Hugo Muller
  Version: 1.0.0
  Date: 2026
*/

#pragma once

#include <cstdint>
#include <Arduino.h>

class ACSensor {
public:
  // SCT-013-030: 30 A RMS -> 1 V RMS. Therefore: 30A/1V = 30 A/V
  static constexpr float  kSensorCurrentPerVolt = 30.0f;
  static constexpr size_t kRmsWindowSamples     = 1600;

  // @param ok starts at true and is
  // set to false if an error occured
  struct Reading {
    bool     valid      = true;
    uint32_t dcBias     = 0;     // in mV
    float    rmsVoltage = 0.0f;  // in V
    float    rmsCurrent = 0.0f;  // in A
  };

  Reading getCurrent();

  // @param voltage in mV
  void processVoltage(uint32_t voltage);

private:
  void processRMS();

  size_t m_rmsSampleCount       = 0;
  double m_rmsSumVoltage        = 0.0;
  double m_rmsSumSquaredVoltage = 0.0;

  Reading      m_reading    = {};
  portMUX_TYPE m_readingMux = portMUX_INITIALIZER_UNLOCKED;
};
