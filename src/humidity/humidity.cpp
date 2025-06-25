#include "humidity.h"

#include "config/config.h"

const int dry = 2400;  // value for dry sensor
const int wet = 1750;  // value for wet sensor

int getSoilMoisture(int *value) {
  uint8_t sensor = Config::getConfig()->getMoistureSensor();
  pinMode(sensor, INPUT_PULLUP);
  if (sensor != 0) {
    uint8_t  maxMoisture       = Config::getConfig()->getMaxMoisture();
    unsigned percent           = 100 - maxMoisture;  // reverse percentage
    unsigned intervals         = (HUMIDITY_AIR - HUMIDITY_WATER) / (100 / percent);
    unsigned limit             = HUMIDITY_WATER + intervals;
    unsigned soilMoistureValue = analogRead(sensor);
    soilMoistureValue          = soilMoistureValue < HUMIDITY_WATER ? HUMIDITY_WATER : soilMoistureValue;
    soilMoistureValue          = soilMoistureValue > HUMIDITY_AIR ? HUMIDITY_AIR : soilMoistureValue;
    unsigned maxDiff           = HUMIDITY_AIR - HUMIDITY_WATER;
    unsigned tmp               = HUMIDITY_AIR - soilMoistureValue;
    unsigned diff              = maxDiff - tmp;
    *value                     = 100 - (diff * 100 / maxDiff);

    /*
    Serial.printf("soilMoistureValue: %u\n", soilMoistureValue);
    Serial.printf("maxMoisture: %u\n", maxMoisture);
    Serial.printf("percent: %u\n", percent);
    Serial.printf("intervals: %u\n", intervals);
    Serial.printf("maxDiff: %u\n", maxDiff);
    Serial.printf("tmp: %u\n", tmp);
    Serial.printf("diff: %u\n", diff);
    Serial.printf("value: %d\n", *value);
    */
    if (soilMoistureValue < (limit)) {
      Serial.printf("Humidity: %d (limit %d), WET\n", soilMoistureValue, limit);
      return HUMIDITY_WET;
    } else {
      Serial.printf("Humidity: %d (limit %d), DRY\n", soilMoistureValue, limit);
      return HUMIDITY_DRY;
    }
    *value = map(soilMoistureValue, wet, dry, 100, 0);
  }
  Serial.printf("Humidity: no sensor, DRY\n");
  *value = 50;
  return HUMIDITY_DRY;
}
