#include <unity.h>
#include <ArduinoJson.h>

#include "config/generated/Config.h"
#include "config/generated/ConfigReflection.h"
#include "config/runtime/Validation.h"
#include "config/runtime/JsonSerializer.h"
#include "config/runtime/JsonDeserializer.h"

static constexpr char kUnexpectedDeserializationErrorPath[] =
  "Unexpected deserialization error path";
static constexpr char kUnexpectedDeserializationErrorMessage[] =
  "Unexpected deserialization error message";

// -----------------------------------------------------------------------------
// Test fixtures
// -----------------------------------------------------------------------------

void setUp() {}

void tearDown() {}

// -----------------------------------------------------------------------------
// Test data
// -----------------------------------------------------------------------------

const char *goodUserSettingsJsonPretty() {
  return R"JSON({
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
        { "name": "Drip Line", "valves": [1] },
        { "name": "Sprinklers", "valves": [2] }
      ] }
    ]
  })JSON";
}

const char *goodUserSettingsJsonMinified() {
  return "{\"params\":{\"wifi\":{\"mdns\":\"jdc-watering\",\"useAPMode\":true,"
         "\"station\":{\"ssid\":\"YourWifiNetwork\",\"password\":\"12345678\"},"
         "\"access-point\":{\"ssid\":\"WateringController\",\"password\":\"\"}},"
         "\"watering\":{\"duration\":{\"min\":1,\"max\":45,\"base\":15,\"step\":5},"
         "\"seasonal\":{\"factor\":100},\"soil\":{\"moisture\":{\"threshold\":60}},"
         "\"pump\":{\"flow\":{\"min\":2,\"max\":80}}}},"
         "\"zones\":[{\"id\":\"209c8ed3-e83a-486f-b5b3-c46abb96d10e\","
         "\"name\":\"My Zone\",\"lines\":[{\"id\":\"a5127d03-ac72-4ca9-905d-f2fc66889e86\","
         "\"name\":\"My Line\",\"valves\":[1,4]}]}]}";
}

const char *tooManyLinesJsonPretty() {
  return R"JSON({
    "zones": [
      { "name": "Front Yard", "lines": [ 
        { "name": "Drip Line 1", "valves": [1] },
        { "name": "Drip Line 2", "valves": [2] },
        { "name": "Drip Line 3", "valves": [3] },
        { "name": "Drip Line 4", "valves": [4] },
        { "name": "Drip Line 5", "valves": [5] },
        { "name": "Drip Line 6", "valves": [6] },
        { "name": "Drip Line 7", "valves": [7] },
        { "name": "Drip Line 8", "valves": [8] },
        { "name": "Drip Line 9", "valves": [8] }
      ] }
    ]
  })JSON";
}

const char *goodSchedulesJsonPretty() {
  return R"JSON({
    "schedules": [
      {
        "id": "e27eb9df-f08d-4f88-84fb-a7a553dbbc83",
        "lineId": "e6d2c9a5-ff4c-48d6-9bb4-cc2c7df9fb74",
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
        "name": "Early Morning",
        "definition": "01:00,17,1,s-1"
      },
      {
        "id": "c7c779e5-cc8e-4b81-a556-17ba87a5192d",
        "lineId": "e6d2c9a5-ff4c-48d6-9bb4-cc2c7df9fb74",
        "definition": "02:00,1,1,e"
      },
      {
        "id": "f808dc0b-1006-4670-82d7-0705ce78fd31",
        "lineId": "e6d2c9a5-ff4c-48d6-9bb4-cc2c7df9fb74",
        "definition": "09:55,40,0,*"
      },
      {
        "id": "61f1d0d5-c664-4343-a2c0-267dc271b5c5",
        "lineId": "e6d2c9a5-ff4c-48d6-9bb4-cc2c7df9fb74",
        "definition": "19:12,32,0,o"
      },
      {
        "id": "0c753464-818b-4726-85aa-e01afe29c980",
        "lineId": "e6d2c9a5-ff4c-48d6-9bb4-cc2c7df9fb74",
        "definition": "13:00,10,1,e"
      },
      {
        "id": "06ebbaf9-fcab-4ffc-b7ff-5e3dd351abcb",
        "lineId": "e6d2c9a5-ff4c-48d6-9bb4-cc2c7df9fb74",
        "definition": "15:41,20,0,s-100"
      }
    ]
  })JSON";
}

const char *goodSchedulesJsonMinified() {
  return "{\"schedules\":[{\"id\":\"ce5c41dd-e944-4e66-a1c7-c650eca4931c\","
         "\"lineId\":\"a5127d03-ac72-4ca9-905d-f2fc66889e86\",\"name\":\"My Schedule\","
         "\"definition\":\"7:30,20,0,e-0\"}]}";
}

const char *duplicateIdSchedulesJsonPretty() {
  return R"JSON({
    "schedules": [
      {
        "id": "e27eb9df-f08d-4f88-84fb-a7a553dbbc83",
        "lineId": "e6d2c9a5-ff4c-48d6-9bb4-cc2c7df9fb74",
        "name": "Morning",
        "definition": "8:00,30,0,s-53"
      },
      {
        "id": "e27eb9df-f08d-4f88-84fb-a7a553dbbc83",
        "lineId": "e6d2c9a5-ff4c-48d6-9bb4-cc2c7df9fb74",
        "definition": "23:00,10,1,e"
      }
    ]
  })JSON";
}

// -----------------------------------------------------------------------------
// Helpers
// -----------------------------------------------------------------------------

template<typename Type, typename Filter>
JsonDeserializer::TraversalResult deserializeInto(
  const char *json, Type &cfg, const Filter &filter) {

  JsonDocument doc;

  DeserializationError err =
    deserializeJson(doc, json);

  if (err)
    return JsonDeserializer::TraversalResult(
      Deserialization::Error::InvalidJson, "bad JSON: %s", err.c_str());

  JsonDeserializer d(doc);
  return Reflection::visit(cfg, d, filter, false);
}

template<typename Type>
JsonDeserializer::TraversalResult deserializeInto(
  const char *json, Type &cfg) {

  return deserializeInto(json, cfg, Reflection::NoFilter{});
}

template<typename Type, typename Filter>
JsonSerializer::TraversalResult serializeInto(
  Type &cfg, JsonDocument &doc, const Filter &filter) {

  JsonSerializer s(doc);

  return Reflection::visit(cfg, s, filter, false);
}

template<typename Type>
JsonSerializer::TraversalResult serializeInto(
  Type &cfg, JsonDocument &doc) {

  return serializeInto(cfg, doc, Reflection::NoFilter{});
}

// #### Usage
// `message_debug(res.path().data());`
// `message_debug(res.message().data());`
void message_debug(const char *msg) {
  TEST_ASSERT_EQUAL_STRING_MESSAGE(
    "...", msg, "(debugging message)");
}

// -----------------------------------------------------------------------------
// Deserialization Tests
// -----------------------------------------------------------------------------

void test_deserialize_valid_json() {
  Config::UserSettings settings;

  auto res = deserializeInto(goodUserSettingsJsonPretty(), settings);
  TEST_ASSERT_TRUE_MESSAGE(
    res.ok(), "The good JSON userSettings should deserialize successfully");
  TEST_ASSERT_TRUE_MESSAGE(
    res.path() == "",
    "No path should be present if config passed deserialization");
  TEST_ASSERT_TRUE_MESSAGE(
    res.message() == "",
    "No error message should be present if config passed deserialization");

  TEST_ASSERT_EQUAL_UINT(
    1, settings.wateringModel.zones.size());
  TEST_ASSERT_EQUAL_UINT(
    2, settings.wateringModel.lines.size());
}

void test_deserialize_invalid_json() {
  Config::UserSettings settings;

  auto res = deserializeInto("", settings);
  TEST_ASSERT_TRUE_MESSAGE(
    !res.ok(), "A bad JSON should not deserialize");
  TEST_ASSERT_TRUE_MESSAGE(
    res.path() == "",
    "No path should be present if JSON is bad");
  TEST_ASSERT_TRUE_MESSAGE(
    res.message() == "bad JSON: EmptyInput",
    "Error message should be present if JSON is bad");

  TEST_ASSERT_EQUAL_UINT(
    0, settings.wateringModel.zones.size());
  TEST_ASSERT_EQUAL_UINT(
    0, settings.wateringModel.lines.size());
}

void test_deserialize_missing_key() {
  const char *goodDuration = R"JSON({
    "min": 5, "base": 15, "step": 5
  })JSON";

  Config::UserSettings::Params::Watering::Duration duration;

  auto res = deserializeInto(goodDuration, duration);
  TEST_ASSERT_EQUAL_STRING_MESSAGE(
    "max",
    res.path().data(),
    kUnexpectedDeserializationErrorPath);
  TEST_ASSERT_EQUAL_STRING_MESSAGE(
    "missing key, expected uint",
    res.message().data(),
    kUnexpectedDeserializationErrorMessage);
  TEST_ASSERT_TRUE_MESSAGE(
    !res.ok() && res.error() == Deserialization::Error::Missing,
    "A missing key should make deserialization fail");
}

void test_deserialize_wrong_type() {
  const char *goodDuration = R"JSON({
    "min": 5, "max": "40", "base": 15, "step": 5
  })JSON";

  Config::UserSettings::Params::Watering::Duration duration;

  auto res = deserializeInto(goodDuration, duration);
  TEST_ASSERT_EQUAL_STRING_MESSAGE(
    "max",
    res.path().data(),
    kUnexpectedDeserializationErrorPath);
  TEST_ASSERT_EQUAL_STRING_MESSAGE(
    "wrong type, expected uint, got text",
    res.message().data(),
    kUnexpectedDeserializationErrorMessage);
  TEST_ASSERT_TRUE_MESSAGE(
    !res.ok() && res.error() == Deserialization::Error::WrongType,
    "A wrong type should make deserialization fail");
}

void test_deserialize_invalid_field() {
  Config::UserSettings::Params::Watering watering;

  const char *badWatering1 = R"JSON({
    "duration": { "min": 999, "max": 40, "base": 15, "step": 5 },
    "seasonal": { "factor": 100 },
    "soil": { "moisture": { "threshold": 60 } },
    "pump": { "flow": { "min": 2, "max": 80 } }
  })JSON";

  auto res = deserializeInto(badWatering1, watering);
  TEST_ASSERT_EQUAL_STRING_MESSAGE(
    "duration.min",
    res.path().data(),
    kUnexpectedDeserializationErrorPath);
  TEST_ASSERT_EQUAL_STRING_MESSAGE(
    "value must be between min=1 and max=60, got 999",
    res.message().data(),
    kUnexpectedDeserializationErrorMessage);
  TEST_ASSERT_TRUE_MESSAGE(
    !res.ok() && res.error() == Deserialization::Error::InvalidValue,
    "Duration with min > max should make deserialization fail");

  const char *badWatering2 = R"JSON({
      "duration": { "min": 41, "max": 40, "base": 15, "step": 5 },
      "seasonal": { "factor": 100 },
      "soil": { "moisture": { "threshold": 60 } },
      "pump": { "flow": { "min": 2, "max": 80 } }
    })JSON";

  res = deserializeInto(badWatering2, watering);
  TEST_ASSERT_EQUAL_STRING_MESSAGE(
    "duration",
    res.path().data(),
    kUnexpectedDeserializationErrorPath);
  TEST_ASSERT_EQUAL_STRING_MESSAGE(
    "min must be less than max=40, got 41",
    res.message().data(),
    kUnexpectedDeserializationErrorMessage);
  TEST_ASSERT_TRUE_MESSAGE(
    !res.ok() && res.error() == Deserialization::Error::InvalidValue,
    "Duration with min > max should make deserialization fail");
}

void test_deserialize_bad_collections() {
  Config cfg;

  auto res = deserializeInto(duplicateIdSchedulesJsonPretty(), cfg,
                             SchedulesOnlyFilter{});
  TEST_ASSERT_EQUAL_STRING_MESSAGE(
    "schedules[1]",
    res.path().data(),
    kUnexpectedDeserializationErrorPath);
  TEST_ASSERT_EQUAL_STRING_MESSAGE(
    "an element with this ID already exists",
    res.message().data(),
    kUnexpectedDeserializationErrorMessage);
  TEST_ASSERT_TRUE_MESSAGE(
    !res.ok() && res.error() == Deserialization::Error::DuplicateId,
    "Duplicate IDs should should make deserialization fail");

  Config::UserSettings userSettings;

  res = deserializeInto(tooManyLinesJsonPretty(), userSettings,
                        WateringModelOnlyFilter{});
  TEST_ASSERT_EQUAL_STRING_MESSAGE(
    "zones[0].lines[8]",
    res.path().data(),
    kUnexpectedDeserializationErrorPath);
  TEST_ASSERT_EQUAL_STRING_MESSAGE(
    "storage capacity exceeded",
    res.message().data(),
    kUnexpectedDeserializationErrorMessage);
  TEST_ASSERT_TRUE_MESSAGE(
    !res.ok() && res.error() == Deserialization::Error::CapacityExceeded,
    "Adding too many elements should make deserialization fail");
}

void test_deserialize_cross_validation() {
  const char *badModel = R"JSON({
    "zones": [
      { 
        "name": "Front Yard",
        "lines": [ 
          { "name": "Drip Line", "valves": [1, 5] }
        ] 
      },
      { 
        "name": "Back Yard",
        "lines": [ 
          { "name": "Drip Line", "valves": [6] },
          { "name": "Sprinklers", "valves": [5] }
        ] 
      }
    ]
  })JSON";

  Config::UserSettings userSettings;

  auto res = deserializeInto(badModel, userSettings,
                             WateringModelOnlyFilter{});
  TEST_ASSERT_EQUAL_STRING_MESSAGE(
    "",
    res.path().data(),
    kUnexpectedDeserializationErrorPath);
  TEST_ASSERT_EQUAL_STRING_MESSAGE(
    "lines[2]: valve 5 is already used by lines[0]",
    res.message().data(),
    kUnexpectedDeserializationErrorMessage);
  TEST_ASSERT_TRUE_MESSAGE(
    !res.ok() && res.error() == Deserialization::Error::InvalidValue,
    "A failed cross-validation should make deserialization fail");
}

// -----------------------------------------------------------------------------
// Round-trip Tests
// -----------------------------------------------------------------------------

void test_round_trip_userSettings() {
  Config::UserSettings settings;

  auto deserializeRes = deserializeInto(goodUserSettingsJsonMinified(), settings);
  TEST_ASSERT_TRUE_MESSAGE(
    deserializeRes.ok(), "The good JSON user settings should deserialize successfully");
  TEST_ASSERT_TRUE_MESSAGE(
    deserializeRes.path() == "",
    "No path should be present if JSON user settings passed deserialization");
  TEST_ASSERT_TRUE_MESSAGE(
    deserializeRes.message() == "",
    "No error message should be present if JSON user settings passed deserialization");

  TEST_ASSERT_EQUAL_UINT(
    1, settings.wateringModel.zones.size());
  TEST_ASSERT_EQUAL_UINT(
    1, settings.wateringModel.lines.size());

  JsonDocument doc;

  auto serializeRes = serializeInto(settings, doc);
  TEST_ASSERT_TRUE_MESSAGE(
    serializeRes.ok(), "The user settings should serialize successfully");
  TEST_ASSERT_TRUE_MESSAGE(
    serializeRes.path() == "",
    "No path should be present if user settings passed serialization");
  TEST_ASSERT_TRUE_MESSAGE(
    serializeRes.message() == "",
    "No error message should be present if user settings passed serialization");

  std::string jsonString;
  serializeJson(doc, jsonString);  // now reloadable by JsonDeserializer as-is
  TEST_ASSERT_EQUAL_STRING_MESSAGE(
    goodUserSettingsJsonMinified(),
    jsonString.c_str(),
    "Serialized user settings JSON string should match original JSON");
}

void test_round_trip_schedules() {
  using WateringModel = Config::UserSettings::WateringModel;

  Config cfg;

  Config::Zone zone;
  TEST_ASSERT_TRUE_MESSAGE(
    UUID::parse("209c8ed3-e83a-486f-b5b3-c46abb96d10e", zone.id),
    "Zone UUID should have been parsed successfully");
  zone.name = "My Zone";

  Config::Line line;
  TEST_ASSERT_TRUE_MESSAGE(
    UUID::parse("a5127d03-ac72-4ca9-905d-f2fc66889e86", line.id),
    "Line UUID should have been parsed successfully");
  line.name = "My Line";
  line.valves.set(3);
  line.zoneId = zone.id;

  TEST_ASSERT_TRUE_MESSAGE(
    cfg.userSettings.wateringModel.zones.add(zone)
        == WateringModel::ZoneCollection::AddResult::Ok
      && cfg.userSettings.wateringModel.zones.size() == 1,
    "Zone should have been added successfully");
  TEST_ASSERT_TRUE_MESSAGE(
    cfg.userSettings.wateringModel.lines.add(line)
        == WateringModel::LineCollection::AddResult::Ok
      && cfg.userSettings.wateringModel.lines.size() == 1,
    "Line should have been added successfully");

  auto deserializeRes = deserializeInto(
    goodSchedulesJsonMinified(), cfg, SchedulesOnlyFilter{});
  TEST_ASSERT_TRUE_MESSAGE(
    deserializeRes.ok(), "The good JSON schedules should deserialize successfully");
  TEST_ASSERT_TRUE_MESSAGE(
    deserializeRes.path() == "",
    "No path should be present if JSON schedules passed deserialization");
  TEST_ASSERT_TRUE_MESSAGE(
    deserializeRes.message() == "",
    "No error message should be present if JSON schedules passed deserialization");

  TEST_ASSERT_EQUAL_UINT(
    1, cfg.schedules.size());

  JsonDocument doc;

  auto serializeRes = serializeInto(cfg, doc, SchedulesOnlyFilter{});
  TEST_ASSERT_TRUE_MESSAGE(
    serializeRes.ok(), "The schedules should serialize successfully");
  TEST_ASSERT_TRUE_MESSAGE(
    serializeRes.path() == "",
    "No path should be present if schedules passed serialization");
  TEST_ASSERT_TRUE_MESSAGE(
    serializeRes.message() == "",
    "No error message should be present if schedules passed serialization");

  std::string jsonString;
  serializeJson(doc, jsonString);  // now reloadable by JsonDeserializer as-is
  TEST_ASSERT_EQUAL_STRING_MESSAGE(
    goodSchedulesJsonMinified(),
    jsonString.c_str(),
    "Serialized schedules JSON string should match original JSON");
}

// -----------------------------------------------------------------------------
// Main
// -----------------------------------------------------------------------------

int main() {
  UNITY_BEGIN();

  // Deserialize

  RUN_TEST(test_deserialize_valid_json);
  RUN_TEST(test_deserialize_invalid_json);
  RUN_TEST(test_deserialize_missing_key);
  RUN_TEST(test_deserialize_wrong_type);

  RUN_TEST(test_deserialize_invalid_field);
  RUN_TEST(test_deserialize_bad_collections);

  RUN_TEST(test_deserialize_cross_validation);

  // Round-trip

  RUN_TEST(test_round_trip_userSettings);
  RUN_TEST(test_round_trip_schedules);

  return UNITY_END();
}
