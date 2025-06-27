#include "humidity.h"

#include "config/config.h"

const int dry = 2480;  // value for dry sensor
const int wet = 660;   // value for wet sensor

int getSoilMoisture(int *value) {
  uint8_t sensor = Config::getConfig()->getMoistureSensor();
  pinMode(sensor, INPUT_PULLUP);

  if (sensor != 0) {
    uint8_t  maxMoisture       = Config::getConfig()->getMaxMoisture();
    unsigned limitInPercent    = 100 - maxMoisture;  // reverse percentage
    unsigned soilMoistureValue = analogRead(sensor);
    unsigned maxDiff           = HUMIDITY_AIR - HUMIDITY_WATER;
    unsigned tmp               = HUMIDITY_AIR - soilMoistureValue;
    unsigned diff              = maxDiff - tmp;
    *value                     = map(constrain(soilMoistureValue, wet, dry), wet, dry, 100, 0);
    Serial.printf("Humidity: %d between [%d wet, %d dry] = %d%% (limit %d) => ", soilMoistureValue, wet, dry, *value, limitInPercent);
    if (*value < (limitInPercent)) {
      Serial.println("WET");
      return HUMIDITY_WET;
    } else {
      Serial.println("DRY");
      return HUMIDITY_DRY;
    }
  }

  *value = 0;
  Serial.printf("Humidity: no sensor declared, considered as DRY\n");
  return HUMIDITY_DRY;
}
