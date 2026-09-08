#include "RFRemote.h"

#include <cstring>
#include <Arduino.h>

RFRemote::RFRemote(uint8_t pin, rmt_channel_t channel)
  : m_pin(static_cast<gpio_num_t>(pin)),
    m_channel(channel) {}

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
    return false;
  if (rmt_driver_install(m_channel, 0, 0) != ESP_OK)
    return false;
  // Create the RF background task
  BaseType_t result = xTaskCreate(
    transmitTask,
    "RFTransmit",
    4096, this,
    5, &m_task);

  if (result != pdPASS) {
    enterErrorState(ESP_ERR_NO_MEM);
    return false;
  }

  return true;
}

void RFRemote::setRepeatTransmit(uint8_t repeat) {
  if (repeat == 0 || repeat > 20)
    return;
  m_nRepeats = repeat;
}

bool RFRemote::sendRF(const char *bits) {

  if (bits == nullptr) {
    return false;
  }

  size_t len = strlen(bits);

  if (len == 0 || len > kMaxBits) {
    return false;
  }

  // Make sure background task exists.
  if (!isTaskAlive()) {
    return false;
  }

  // Don't overwrite an active transmission.
  if (m_state == State::Transmitting) {
    return false;
  }

  // Copy the data because the caller's buffer may
  // disappear after sendRF() returns.
  strcpy(m_bits, bits);
  m_bitCount = len;

  enterOkState(State::Transmitting);

  // Tell the background task to perform it.
  xTaskNotifyGive(m_task);

  return true;
}

bool RFRemote::isBusy() const {
  return m_state == State::Transmitting;
}

void RFRemote::transmitTask(void *args) {
  RFRemote *self = static_cast<RFRemote *>(args);

  while (true) {

    // Wait until sendRF() asks us to transmit.
    // This task consumes no CPU while waiting.
    ulTaskNotifyTake(pdTRUE, portMAX_DELAY);

    // Transmission requested
    esp_err_t err = self->transmit();

    if (err == ESP_OK)
      self->enterOkState(State::Idle);
    else
      self->enterErrorState(err);

    if (self->onTxFinished)
      self->onTxFinished(err);
  }
}

esp_err_t RFRemote::transmit() {
  // Defensive check: make sure the frame fits
  // in the fixed-size buffers
  if (m_bitCount == 0 || m_bitCount > kMaxBits) {
    return ESP_ERR_INVALID_ARG;
  }

  // Encode bits
  for (size_t i = 0; i < m_bitCount; i++) {

    if (m_bits[i] == '0') {

      m_items[i].level0    = 1;
      m_items[i].duration0 = Protocol1::kZeroHigh;

      m_items[i].level1    = 0;
      m_items[i].duration1 = Protocol1::kZeroLow;

    } else if (m_bits[i] == '1') {

      m_items[i].level0    = 1;
      m_items[i].duration0 = Protocol1::kOneHigh;

      m_items[i].level1    = 0;
      m_items[i].duration1 = Protocol1::kOneLow;

    } else {

      // Invalid character in bit string.
      return ESP_ERR_INVALID_ARG;
    }
  }

  // Sync
  m_items[m_bitCount].level0    = 1;
  m_items[m_bitCount].duration0 = Protocol1::kSyncHigh;

  m_items[m_bitCount].level1    = 0;
  m_items[m_bitCount].duration1 = Protocol1::kSyncLow;

  // Send N times
  for (int repeat = 0; repeat < m_nRepeats; repeat++) {

    // Start RMT hardware
    esp_err_t err = rmt_write_items(
      m_channel,
      m_items,
      m_bitCount + 1,  // +1 for sync
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

  return true;
}

bool RFRemote::isTaskAlive() const {
  if (m_task == nullptr) {
    return false;
  }

  eTaskState state = eTaskGetState(m_task);

  return state != eDeleted;
}

void RFRemote::enterOkState(State state) {
  m_state = state;
  m_error = ESP_OK;
}

void RFRemote::enterErrorState(esp_err_t err) {
  m_state = State::Error;
  m_error = err;
}
