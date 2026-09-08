#pragma once

#include <functional>
#include <Arduino.h>
#include <esp_adc_cal.h>
#include <driver/adc.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

class FaultManager;

class AdcSampler {
public:
  static constexpr char kTag[] = "[AdcSampler]";

  // ADC digital controller pattern. The pattern is repeated:
  // CH0, CH1, CH0, CH1, CH0, CH1, CH0, CH1, CH0, CH1...
  // So having 2 channels at 20 kHz means each channel runs at 10 kHz.
  static constexpr uint16_t kSampleRateHz = 20000;
  // Number of 16-bit DMA ADC conversions processed at once.
  // 1000 @ 20 kHz = 50 ms, so with 2 channels it would be
  // 500 ADC conversions per channel in 50ms.
  static constexpr uint16_t kProcessingSamples = 1000;

  static constexpr uint16_t kDefaultVref   = 1100;
  static constexpr uint8_t  kMaxChannels   = 4;
  static constexpr uint32_t kReadTimeoutMs = 1000;

  struct ChannelConfig {
    uint8_t          pin;
    adc_atten_t      attenuation;
    adc_bits_width_t bitsWidth;
  };

  struct Config {
    const ChannelConfig *channels;
    uint8_t              channelCount = 0;
  };

  using DataCallback = std::function<void(
    const adc_digi_output_data_t *samples,
    size_t                        sampleCount)>;

  AdcSampler(FaultManager &faults);
  ~AdcSampler() { stop(); }

  bool begin(const Config &config,
             DataCallback  callback);

  // Convert raw to voltage in mV
  uint32_t convertRawToVoltage(uint8_t pin, uint16_t raw);

private:
  bool adcDigiInit(const Config &config,
                   DataCallback  callback);

  void stop();

  static void taskEntry(void *arg);
  void        task();

  void handleReadFailure(esp_err_t status);
  void handleReadSuccess();

  FaultManager &m_faults;

  DataCallback m_callback = nullptr;

  TaskHandle_t m_task = nullptr;

  uint8_t m_channelPin[kMaxChannels];
  uint8_t m_channelCount = 0;

  // ADC calibration
  esp_adc_cal_characteristics_t m_adcChars[kMaxChannels];
  esp_adc_cal_characteristics_t m_adcDefaultChars;

  static constexpr size_t kBufferBytes =
    kProcessingSamples * sizeof(adc_digi_output_data_t);

  uint8_t m_dmaBuffer[kBufferBytes];

  bool m_unknownPinWarningLogged = false;

  static constexpr uint8_t kFaultAfterErrors    = 3;
  static constexpr uint8_t kClearAfterSuccesses = 10;

  uint8_t m_adcErrorCount   = 0;
  uint8_t m_adcSuccessCount = 0;
  bool    m_adcFaultActive  = false;
};
