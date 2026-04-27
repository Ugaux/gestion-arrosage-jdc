#include "sensors.h"

#include "config/config.h"

#define DEBUG false

const int dry = 2480;  // value for 100% dry sensor
const int wet = 660;   // value for 100% wet sensor

int getSoilMoisture(int *value) {
  uint8_t sensor = Config::getConfig()->getMoistureSensor();
  pinMode(sensor, INPUT_PULLUP);

  if (sensor != 0) {
    analogSetPinAttenuation(sensor, ADC_0db);  // See if it's necessary
    uint8_t  maxMoisture       = Config::getConfig()->getMaxMoisture();
    unsigned limitInPercent    = 100 - maxMoisture;  // reverse percentage
    unsigned soilMoistureValue = analogRead(sensor);
    *value                     = map(constrain(soilMoistureValue, wet, dry), wet, dry, 100, 0);
    if (DEBUG)
      Serial.printf("Humidity: %d between [%d wet, %d dry] = %d%% (limit %d) => ", soilMoistureValue, wet, dry, *value, limitInPercent);
    if (*value < (limitInPercent)) {
      if (DEBUG)
        Serial.println("DRY");
      return HUMIDITY_DRY;
    } else {
      if (DEBUG)
        Serial.println("WET");
      return HUMIDITY_WET;
    }
  }

  *value = 0;
  if (DEBUG)
    Serial.printf("Humidity: no sensor declared, considered as DRY\n");
  return HUMIDITY_DRY;
}

float getFlow(void) {
  float flowRate = 0;
  return flowRate;
}
