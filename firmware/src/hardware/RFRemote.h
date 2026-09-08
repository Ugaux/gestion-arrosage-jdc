/*
  RFRemote - ESP32 library for transmitting 433 MHz OOK signals

  Uses the ESP32 RMT peripheral (legacy RMT driver, ESP-IDF 4.4.7).
  Transmission is non-blocking and has very low CPU overhead.
  The CPU prepares the RF frame and starts the transmission;
  the RMT peripheral handles the waveform generation and precise timing.

  Target: Classic ESP32

  Author: Hugo Muller
  Version: 1.0.0
  Date: 2026
*/

#pragma once

#include <functional>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include "driver/rmt.h"

class RFRemote {
public:
  struct Protocol1 {
    static constexpr uint32_t kPulseLength = 350;
    static constexpr uint32_t kSyncHigh    = kPulseLength;
    static constexpr uint32_t kSyncLow     = 31 * kPulseLength;
    static constexpr uint32_t kZeroHigh    = kPulseLength;
    static constexpr uint32_t kZeroLow     = 3 * kPulseLength;
    static constexpr uint32_t kOneHigh     = 3 * kPulseLength;
    static constexpr uint32_t kOneLow      = kPulseLength;
  };

  static constexpr uint32_t kTxTimeoutMs = 100;
  static constexpr size_t   kMaxBits     = 128;

  enum class State : uint8_t {
    Idle = 0,
    Transmitting,
    Error
  };

  RFRemote(uint8_t pin, rmt_channel_t channel);

  std::function<void(esp_err_t)> onTxFinished;

  bool begin();

  // Sets the number of repetition of signal.
  // The minimum is minimum 1, the maximum is 20.
  void setRepeatTransmit(uint8_t repeat);

  // Non-blocking send.
  // This function copies the bits and returns immediately.
  bool sendRF(const char *bits);

  bool      isBusy() const;
  State     getState() const { return m_state; }
  esp_err_t getError() const { return m_error; }

private:
  // RF background task
  static void transmitTask(void *args);
  // Build + transmit RF waveform
  esp_err_t transmit();
  bool      isTaskAlive() const;

  void enterOkState(State state);
  void enterErrorState(esp_err_t err);

  TaskHandle_t m_task = nullptr;

  volatile State     m_state = State::Idle;
  volatile esp_err_t m_error = ESP_OK;

  gpio_num_t    m_pin;
  rmt_channel_t m_channel;
  uint8_t       m_nRepeats = 5;

  char         m_bits[kMaxBits + 1]  = {};
  rmt_item32_t m_items[kMaxBits + 1] = {};

  size_t m_bitCount = 0;
};
