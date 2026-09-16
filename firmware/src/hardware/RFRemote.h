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

#include <cstdint>
#include <functional>
#include <freertos/FreeRTOS.h>
#include <freertos/queue.h>
#include <freertos/task.h>
#include "driver/rmt.h"

class FaultManager;

class RFRemote {
public:
  static constexpr char kTag[] = "[RFRemote]";

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

  // Maximum RF bits per command
  static constexpr size_t kMaxBits     = 32;
  static constexpr size_t kQueueLength = 4;

  enum class State : uint8_t {
    Uninitialized = 0,
    Idle,
    Transmitting,
    Error
  };

  enum class SendResult {
    Queued,
    NotInitialized,
    InvalidCommand,
    TaskUnavailable,
    QueueUnavailable,
    QueueFull,
  };

  // Uses pin `Pinout::ESP::RadioTX` and channel `RMT_CHANNEL_0`
  RFRemote(FaultManager &faults);
  RFRemote(uint8_t pin, rmt_channel_t channel, FaultManager &faults);

  bool begin();

  // Sets the number of repetition of signal.
  // The minimum is minimum 1, the maximum is 20.
  void setRepeatTransmit(uint8_t repeat);

  // Non-blocking send: queues the RF command and returns immediately.
  SendResult sendRF(std::string_view command);

  // Returns true if a transmission is currently active or
  // if one or more commands are waiting in the queue.
  // State::Transmitting only represents the active transmission;
  // isBusy() also accounts for pending commands in the queue.
  // This is a snapshot and must not be used for synchronization.
  bool      isBusy() const;
  State     state() const { return m_state; }
  esp_err_t error() const { return m_error; }

private:
  struct Command {
    char   bits[kMaxBits + 1] = {};  // +1 for the null terminator
    size_t bitCount           = 0;
  };

  // RF background task
  static void transmitTask(void *args);
  // Build + transmit RF waveform
  esp_err_t transmit(const Command &command);
  bool      isTaskAlive() const;

  void enterState(State state);
  void enterError(esp_err_t err);

  void handleTransmissionFailure(esp_err_t status);
  void handleTransmissionSuccess();

  bool failInit();

  gpio_num_t    m_pin;
  rmt_channel_t m_channel;
  uint8_t       m_nRepeats = 5;

  FaultManager &m_faults;

  static constexpr uint8_t kFaultAfterErrors    = 3;
  static constexpr uint8_t kClearAfterSuccesses = 10;

  uint8_t m_transmissionErrorCount   = 0;
  uint8_t m_transmissionSuccessCount = 0;
  bool    m_transmissionFaultActive  = false;

  QueueHandle_t m_queue = nullptr;
  TaskHandle_t  m_task  = nullptr;

  volatile State     m_state = State::Uninitialized;
  volatile esp_err_t m_error = ESP_OK;

  StaticQueue_t m_queueControl;
  uint8_t       m_queueStorage[kQueueLength * sizeof(Command)] = {};

  rmt_item32_t m_items[kMaxBits + 1] = {};  // +1 for sync item
};
