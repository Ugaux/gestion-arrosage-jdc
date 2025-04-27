#include <Arduino.h>

#include "../config/config.h"
#include "flow.h"

#define SENSOR_INTERRUPT digitalPinToInterrupt(FLOW_SENSOR)

float calibrationFactor = 7.5;

static volatile byte pulseCount;
static unsigned long total;

unsigned long oldTime;

void IRAM_ATTR pulseCounter() {
  pulseCount++;
}

void flowInit(void) {
  uint8_t sensor = Config::getConfig()->getFlowSensor();
  if (sensor != 0) {
    pinMode(sensor, INPUT_PULLUP);
    attachInterrupt(digitalPinToInterrupt(sensor), pulseCounter, FALLING);
  }
}

float getFlow(void) {
  float        flowRate = 0;
  unsigned int milliLiters;

  if (Config::getConfig()->getFlowSensor() == 0) {
    return 0.0;
  }
  if (millis() - oldTime == 0) {
    return 0;
  }
  noInterrupts();
  flowRate    = ((1000.0 / (millis() - oldTime)) * pulseCount) / calibrationFactor;
  oldTime     = millis();
  milliLiters = (flowRate / 60) * 1000;
  total += milliLiters;
  pulseCount = 0;
  interrupts();
  //  Serial.printf("flow: %f\n", flowRate);
  return flowRate;
}
