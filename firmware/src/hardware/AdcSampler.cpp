#include "AdcSampler.h"

#include <cstring>
#include <esp_adc_cal.h>
#include "esp_log.h"
#include "core/FaultManager.h"

namespace {

constexpr uint8_t adcWidthToBits(adc_bits_width_t width) {
  switch (width) {
    case ADC_WIDTH_BIT_9: return 9;
    case ADC_WIDTH_BIT_10: return 10;
    case ADC_WIDTH_BIT_11: return 11;
    case ADC_WIDTH_BIT_12: return 12;
    default: return 0;
  }
}

}  // namespace

AdcSampler::AdcSampler(FaultManager &faults)
  : m_faults(faults) {}

bool AdcSampler::begin(const Config &config,
                       DataCallback  callback) {

  // Configure I2S + ADC + DMA
  if (!adcDigiInit(config, callback))
    return false;

  // Start processing task
  BaseType_t result = xTaskCreate(
    taskEntry,
    "AdcSampler",
    4096, this,
    10, &m_task);

  if (result != pdPASS) {
    adc_digi_stop();
    return false;
  }

  esp_err_t err = adc_digi_start();

  if (err != ESP_OK) {
    log_d("%s adc_digi_start failed: %s",
          kTag, esp_err_to_name(err));
    stop();
    return false;
  }

  return true;
}

bool AdcSampler::adcDigiInit(const Config &config, DataCallback callback) {
  if (kSampleRateHz == 0 || kProcessingSamples == 0)
    return false;

  if (callback == nullptr)
    return false;

  m_callback = callback;

  if (config.channelCount > kMaxChannels)
    return false;

  m_channelCount = config.channelCount;

  // Configure ADC1 channels

  uint32_t adc1ChanMask = 0;

  adc_digi_pattern_config_t pattern[kMaxChannels] = {};

  for (size_t i = 0; i < config.channelCount; ++i) {

    const auto &channel = config.channels[i];

    int adc1Channel = digitalPinToAnalogChannel(channel.pin);

    if (adc1Channel == -1)
      return false;

    adc1ChanMask |= 1UL << adc1Channel;

    pattern[i] = {
      .atten     = static_cast<uint8_t>(channel.attenuation),
      .channel   = static_cast<uint8_t>(adc1Channel),
      .unit      = 0,  // ADC_UNIT_1 in the digital-controller/LL representation,
      .bit_width = adcWidthToBits(channel.bitsWidth),
    };

    m_channelPin[i] = channel.pin;

    // Characterize ADC for channel
    esp_adc_cal_value_t cal_type = esp_adc_cal_characterize(
      ADC_UNIT_1,
      channel.attenuation,
      channel.bitsWidth,
      kDefaultVref,
      &m_adcChars[i]);

    switch (cal_type) {
      case ESP_ADC_CAL_VAL_EFUSE_VREF:
        log_d("%s gpio %d (channel %d)"
              " ADC calibration: eFuse Vref",
              kTag, channel.pin, adc1Channel);
        break;

      case ESP_ADC_CAL_VAL_EFUSE_TP:
        log_d("%s gpio %d (channel %d)"
              " ADC calibration: Two Point",
              kTag, channel.pin, adc1Channel);
        break;

      default:
        log_w("%s gpio %d (channel %d)"
              " ADC calibration: Default Vref",
              kTag, channel.pin, adc1Channel);
        break;
    }
  }

  // Characterize default ADC
  esp_adc_cal_characterize(
    ADC_UNIT_1,
    ADC_ATTEN_DB_12,
    ADC_WIDTH_BIT_12,
    kDefaultVref,
    &m_adcDefaultChars);

  // ADC digital controller initialization
  adc_digi_init_config_t initConfig = {
    .max_store_buf_size = 4096,
    .conv_num_each_intr = 256,
    .adc1_chan_mask     = adc1ChanMask,
    .adc2_chan_mask     = 0,
  };

  esp_err_t err = adc_digi_initialize(&initConfig);

  if (err != ESP_OK) {
    log_d("%s adc_digi_initialize failed: %s",
          kTag, esp_err_to_name(err));
    return false;
  }

  // ADC digital controller configuration
  adc_digi_configuration_t digiConfig = {
    .conv_limit_en  = true,
    .conv_limit_num = 255,
    .pattern_num    = config.channelCount,
    .adc_pattern    = pattern,
    .sample_freq_hz = kSampleRateHz,  // has to be >= 20000
    .conv_mode      = ADC_CONV_SINGLE_UNIT_1,
    .format         = ADC_DIGI_OUTPUT_FORMAT_TYPE1,
  };

  err = adc_digi_controller_configure(&digiConfig);

  if (err != ESP_OK) {
    log_d("%s adc_digi_controller_configure failed: %s",
          kTag, esp_err_to_name(err));
    return false;
  }

  return true;
}

void AdcSampler::stop() {
  if (m_task != nullptr) {
    vTaskDelete(m_task);
    m_task = nullptr;
  }

  adc_digi_stop();
  adc_digi_deinitialize();
}

uint32_t AdcSampler::convertRawToVoltage(uint8_t pin, uint16_t raw) {
  for (uint8_t i = 0; i < m_channelCount; ++i) {
    if (m_channelPin[i] == pin) {
      return esp_adc_cal_raw_to_voltage(
        raw, &m_adcChars[i]);
    }
  }

  if (!m_unknownPinWarningLogged) {
    log_w("%s ADC: unexpected pin %u; "
          "falling back to default calibration",
          kTag, pin);
    m_unknownPinWarningLogged = true;
  }

  return esp_adc_cal_raw_to_voltage(
    raw, &m_adcDefaultChars);
}

void AdcSampler::taskEntry(void *arg) {
  AdcSampler *self = static_cast<AdcSampler *>(arg);

  while (true)
    self->task();
}

void AdcSampler::task() {
  uint32_t byteCount = 0;

  // WAIT for DMA. This is NOT polling.
  // The task sleeps inside the I2S driver until a DMA
  // buffer contains data.
  esp_err_t err = adc_digi_read_bytes(
    m_dmaBuffer,
    sizeof(m_dmaBuffer),
    &byteCount, kReadTimeoutMs);

  if (err != ESP_OK) {
    handleReadFailure(err);
    return;
  }

  if (byteCount == 0) {
    handleReadFailure(ESP_ERR_INVALID_SIZE);
    return;
  }

  // Number of ADC results actually returned.
  const size_t sampleCount =
    byteCount / sizeof(adc_digi_output_data_t);

  const adc_digi_output_data_t *samples =
    reinterpret_cast<const adc_digi_output_data_t *>(m_dmaBuffer);

  handleReadSuccess();

  // Give the completed DMA block to
  // the owner of the sampler
  // -> AdcSampler does not interpret it
  m_callback(samples, sampleCount);
}

void AdcSampler::handleReadFailure(esp_err_t err) {
  m_adcSuccessCount = 0;

  if (m_adcFaultActive)
    return;

  if (++m_adcErrorCount >= kFaultAfterErrors) {
    m_faults.set(
      Fault::Component::SensorController,
      Fault::Code::AdcDmaReadFailed);

    m_adcFaultActive = true;
    m_adcErrorCount  = 0;
    log_d("%s failure: %s", kTag, esp_err_to_name(err));
  }
}

void AdcSampler::handleReadSuccess() {
  m_adcErrorCount = 0;

  if (!m_adcFaultActive)
    return;

  if (++m_adcSuccessCount >= kClearAfterSuccesses) {
    m_faults.clear(
      Fault::Component::SensorController,
      Fault::Code::AdcDmaReadFailed);

    m_adcFaultActive  = false;
    m_adcSuccessCount = 0;
    log_d("%s past failure cleared", kTag);
  }
}
