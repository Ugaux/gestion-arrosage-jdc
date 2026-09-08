#include <unity.h>
#include <ArduinoJson.h>

#include "config/generated/Config.h"
#include "config/generated/ConfigReflection.h"
#include "config/runtime/JsonDeserializer.h"
#include "config/runtime/Validation.h"

// -----------------------------------------------------------------------------
// Test fixtures
// -----------------------------------------------------------------------------

void setUp() {}

void tearDown() {}

// -----------------------------------------------------------------------------
// Test data
// -----------------------------------------------------------------------------

const char* goodJson = R"JSON({
    "params": {
      "wifi": {
        "mdns": "jdc-watering",
        "useAPMode": true,
        "station": { "ssid": "MyHomeWifi", "password": "supersecret" },
        "access-point": { "ssid": "WateringController", "password": "changeme1" }
      },
      "watering": {
        "duration": { "min": 1, "max": 40, "base": 15, "step": 5 },
        "seasonal": { "factor": 100 },
        "soil": { "moisture": { "threshold": 60 } },
        "pump": { "flow": { "min": 2, "max": 80 } }
      }
    },
    "zones": [
      { "name": "Front Yard", "lines": [ 
        { "name": "Drip Line", "valves": [1] }
      ] }
    ]
  })JSON";

const char* goodJson2 = R"JSON({
    "schedules": [
      {
        "id": "e27eb9df-f08d-4f88-84fb-a7a553dbbc83",
        "lineId": "e6d2c9a5-ff4c-48d6-9bb4-cc2c7df9fb74",
        "name": "Morning",
        "definition": "22:00,30,0,s-53"
      },
      {
        "id": "67c48a50-2802-421b-bbde-e0564e095b18",
        "lineId": "e6d2c9a5-ff4c-48d6-9bb4-cc2c7df9fb74",
        "definition": "23:00,10,1,e"
      },
      {
        "id": "76ebce0c-8dbd-4e4a-a30f-883e5acdc15b",
        "lineId": "0a47fd50-15f5-4f53-b7cd-a65e49fd6184",
        "definition": "23:00,45,1,o"
      },
      {
        "id": "6ed732d7-a600-4231-a7ad-3dc9d11c552a",
        "lineId": "0a47fd50-15f5-4f53-b7cd-a65e49fd6184",
        "definition": "01:00,17,1,s-1"
      },
      {
        "id": "c7c779e5-cc8e-4b81-a556-17ba87a5192d",
        "lineId": "e6d2c9a5-ff4c-48d6-9bb4-cc2c7df9fb74",
        "definition": "02:00,1,1,e"
      }
    ]
  })JSON";

// -----------------------------------------------------------------------------
// Helpers
// -----------------------------------------------------------------------------

static bool deserializeInto(const char* json, Config::UserSettings& settings) {
  JsonDocument         doc;
  DeserializationError err = deserializeJson(doc, json);
  if (err)
    return false;

  JsonDeserializer deserializer(doc);
  if (!Reflection::visit(settings, deserializer, Reflection::NoFilter{})) {
    char buf[128];
    deserializer.formatError(buf, sizeof(buf));
    return false;
  }

  return true;
}

static bool validateConfig(Config& cfg) {
  Validation::ValidationVisitor v;
  if (!Reflection::visit(cfg, v)) {
    printf("validation failed at %.*s: %.*s\n",
           (int)v.path().size(), v.path().data(),
           (int)v.result().msg().size(), v.result().msg().data());
    return false;
  }
  return true;
}

// -----------------------------------------------------------------------------
// Tests
// -----------------------------------------------------------------------------

void test_deserialize_valid_user_settings(void) {
  Config::UserSettings settings;

  bool ok = deserializeInto(goodJson, settings);

  TEST_ASSERT_TRUE_MESSAGE(
    ok, "Valid JSON should deserialize successfully");

  TEST_ASSERT_EQUAL_UINT(
    1, settings.wateringModel.zones.size());

  TEST_ASSERT_EQUAL_UINT(
    1, settings.wateringModel.lines.size());
}

void test_validate_valid_config(void) {
  Config cfg;

  bool ok = deserializeInto(
    goodJson,
    cfg.userSettings);

  TEST_ASSERT_TRUE_MESSAGE(
    ok, "Configuration should deserialize successfully");

  ok = validateConfig(cfg);

  TEST_ASSERT_TRUE_MESSAGE(
    ok, "Valid configuration should pass validation");
}

void test_validate_inverted_duration(void) {
  Config cfg;

  bool ok = deserializeInto(
    goodJson,
    cfg.userSettings);

  TEST_ASSERT_TRUE_MESSAGE(
    ok, "Configuration should deserialize successfully");

  // Create an invalid configuration:
  // min > max
  cfg.userSettings.params.watering.duration.min = 40;
  cfg.userSettings.params.watering.duration.max = 1;

  ok = validateConfig(cfg);

  TEST_ASSERT_FALSE_MESSAGE(
    ok, "Duration with min > max should fail validation");
}

// -----------------------------------------------------------------------------
// Main
// -----------------------------------------------------------------------------

int main() {
  UNITY_BEGIN();
  //
  //RUN_TEST(test_deserialize_valid_user_settings);
  //RUN_TEST(test_validate_valid_config);
  //RUN_TEST(test_validate_inverted_duration);
  //
  return UNITY_END();
}
