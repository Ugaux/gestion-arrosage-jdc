#include "RFRemote.h"

#include <cstring>
#include <Arduino.h>
#include "Pinout.h"
#include "core/FaultManager.h"

RFRemote::RFRemote(
  FaultManager &faults)
  : m_pin(static_cast<gpio_num_t>(Pinout::ESP::RadioTX)),
    m_channel(RMT_CHANNEL_0),
    m_faults(faults) {}

RFRemote::RFRemote(
  uint8_t pin, rmt_channel_t channel, FaultManager &faults)
  : m_pin(static_cast<gpio_num_t>(pin)),
    m_channel(channel),
    m_faults(faults) {}

bool RFRemote::begin() {
  rmt_config_t config = RMT_DEFAULT_CONFIG_TX(
    m_pin,
    m_channel);

  config.rmt_mode = RMT_MODE_TX;

  // 80 MHz / 80 = 1 MHz
  // Therefore 1 RMT tick = 1 µs
  config.clk_div = 80;

  config.mem_block_num = 1;

  config.tx_config.loop_en    = false;
  config.tx_config.carrier_en = false;

  config.tx_config.idle_output_en = true;
  config.tx_config.idle_level     = RMT_IDLE_LEVEL_LOW;

  if (rmt_config(&config) != ESP_OK)
    return failInit();

  if (rmt_driver_install(m_channel, 0, 0) != ESP_OK)
    return failInit();

  // Create the command queue.
  m_queue = xQueueCreateStatic(
    kQueueLength,
    sizeof(Command),
    m_queueStorage,
    &m_queueControl);

  if (m_queue == nullptr) {
    return failInit();
  }

  // Create the RF background task
  BaseType_t result = xTaskCreate(
    transmitTask,
    "RFTransmit",
    4096, this,
    5, &m_task);

  if (result != pdPASS) {
    return failInit();
  }

  m_state = State::Idle;
  return true;
}

void RFRemote::setRepeatTransmit(uint8_t repeat) {
  if (repeat == 0 || repeat > 20)
    return;

  m_nRepeats = repeat;
}

RFRemote::SendResult RFRemote::sendRF(std::string_view command) {
  if (m_state == State::Uninitialized)
    return SendResult::NotInitialized;

  const size_t bitCount = command.size();

  if (bitCount == 0 || bitCount > kMaxBits)
    return SendResult::InvalidCommand;

  // Make sure background task exists.
  if (!isTaskAlive()) {
    m_faults.set(Fault::Component::RFRemote,
                 Fault::Code::TaskUnavailable);
    return SendResult::TaskUnavailable;
  }

  if (m_queue == nullptr) {
    m_faults.set(Fault::Component::RFRemote,
                 Fault::Code::QueueUnavailable);
    return SendResult::QueueUnavailable;
  }

  Command queuedCommand{};

  // Copy the command into the queue.
  memcpy(queuedCommand.bits, command.data(), bitCount);
  queuedCommand.bits[bitCount] = '\0';
  queuedCommand.bitCount       = bitCount;

  // Queue it without blocking the caller
  if (xQueueSend(m_queue, &queuedCommand, 0) != pdPASS) {
    m_faults.set(Fault::Component::RFRemote,
                 Fault::Code::TransmissionQueueFull);
    return SendResult::QueueFull;
  }

  return SendResult::Queued;
}

bool RFRemote::isBusy() const {
  if (m_state == State::Transmitting)
    return true;

  return m_queue != nullptr
         && uxQueueMessagesWaiting(m_queue) > 0;
}

void RFRemote::transmitTask(void *args) {
  RFRemote *self = static_cast<RFRemote *>(args);

  Command command{};

  while (true) {

    // Wait until a command is queued.
    // This task consumes no CPU while waiting.
    if (xQueueReceive(
          self->m_queue,
          &command,
          portMAX_DELAY)
        != pdPASS) {
      continue;
    }

    // Transmission requested
    self->enterState(State::Transmitting);

    const esp_err_t result = self->transmit(command);

    if (result == ESP_OK) {
      self->enterState(State::Idle);
      self->handleTransmissionSuccess();
    } else {
      self->enterError(result);
      self->handleTransmissionFailure(result);
    }
  }
}

esp_err_t RFRemote::transmit(const Command &command) {
  const size_t bitCount = command.bitCount;

  // Defensive check: make sure the frame fits
  // in the fixed-size buffers
  if (bitCount == 0 || bitCount > kMaxBits)
    return ESP_ERR_INVALID_ARG;

  // Encode bits
  for (size_t i = 0; i < bitCount; i++) {

    if (command.bits[i] == '0') {

      m_items[i].level0    = 1;
      m_items[i].duration0 = Protocol1::kZeroHigh;

      m_items[i].level1    = 0;
      m_items[i].duration1 = Protocol1::kZeroLow;

    } else if (command.bits[i] == '1') {

      m_items[i].level0    = 1;
      m_items[i].duration0 = Protocol1::kOneHigh;

      m_items[i].level1    = 0;
      m_items[i].duration1 = Protocol1::kOneLow;

    } else {

      // Invalid character in bit string
      return ESP_ERR_INVALID_ARG;
    }
  }

  // Sync
  m_items[bitCount].level0    = 1;
  m_items[bitCount].duration0 = Protocol1::kSyncHigh;

  m_items[bitCount].level1    = 0;
  m_items[bitCount].duration1 = Protocol1::kSyncLow;

  // Send N times
  for (uint8_t repeat = 0; repeat < m_nRepeats; repeat++) {

    // Start RMT hardware
    esp_err_t err = rmt_write_items(
      m_channel,
      m_items,
      bitCount + 1,  // +1 for sync
      false);

    if (err != ESP_OK)
      return err;

    // Wait for hardware
    // The RF task BLOCKS here.
    // It does NOT continuously consume CPU.
    err = rmt_wait_tx_done(
      m_channel,
      pdMS_TO_TICKS(kTxTimeoutMs));

    if (err != ESP_OK)
      return err;
  }

  return ESP_OK;
}

bool RFRemote::isTaskAlive() const {
  if (m_task == nullptr)
    return false;

  eTaskState state = eTaskGetState(m_task);

  return state != eDeleted;
}

void RFRemote::enterState(State state) {
  m_state = state;
  m_error = ESP_OK;
}

void RFRemote::enterError(esp_err_t err) {
  m_state = State::Error;
  m_error = err;
}

void RFRemote::handleTransmissionFailure(esp_err_t err) {
  m_transmissionSuccessCount = 0;

  if (m_transmissionFaultActive)
    return;

  if (++m_transmissionErrorCount >= kFaultAfterErrors) {
    m_faults.set(
      Fault::Component::RFRemote,
      Fault::Code::TransmissionFailed);

    m_transmissionFaultActive = true;
    m_transmissionErrorCount  = 0;
    log_d("%s failure: %s", kTag, esp_err_to_name(err));
  }
}

void RFRemote::handleTransmissionSuccess() {
  m_transmissionErrorCount = 0;

  if (!m_transmissionFaultActive)
    return;

  if (++m_transmissionSuccessCount >= kClearAfterSuccesses) {
    m_faults.clear(
      Fault::Component::RFRemote,
      Fault::Code::TransmissionFailed);

    m_transmissionFaultActive  = false;
    m_transmissionSuccessCount = 0;
    log_d("%s past failure cleared", kTag);
  }
}

bool RFRemote::failInit() {
  enterError(ESP_ERR_INVALID_STATE);
  m_faults.set(Fault::Component::RFRemote,
               Fault::Code::InitFailed);
  return false;
}
