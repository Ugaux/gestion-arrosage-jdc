#include <unity.h>

#include "config/generated/Config.h"
#include "config/generated/ConfigReflection.h"
#include "config/runtime/Validation.h"

// -----------------------------------------------------------------------------
// Test fixtures
// -----------------------------------------------------------------------------

void setUp() {}

void tearDown() {}

// -----------------------------------------------------------------------------
// Helpers
// -----------------------------------------------------------------------------

bool validateConfig(Config& cfg) {
  Validation::ValidationVisitor v;
  if (!Reflection::visit(cfg, v)) {
    printf("%.*s: %.*s",
           (int)v.path().size(), v.path().data(),
           (int)v.result().message().size(), v.result().message().data());
    return false;
  }
  //v.error();
  return true;
}

// -----------------------------------------------------------------------------
// Field tests
// -----------------------------------------------------------------------------

void test_validate_default_config(void) {
  Config cfg;

  bool ok = validateConfig(cfg);

  TEST_ASSERT_TRUE_MESSAGE(
    ok, "The default config should be valid");
}

void test_validate_string_length(void) {
  Config cfg;

  cfg.userSettings.params.wifi.mdns = "ab";

  bool ok = validateConfig(cfg);

  TEST_ASSERT_FALSE_MESSAGE(
    ok, "A string that is too small should be invalid");

  cfg.userSettings.params.wifi.mdns = "abcdefghijklmnopqrst";

  TEST_ASSERT_EQUAL_STRING_MESSAGE(
    cfg.userSettings.params.wifi.mdns.c_str(),
    "abcdefghijklmno",
    "A string should be cropped to max size");

  ok = validateConfig(cfg);

  TEST_ASSERT_TRUE_MESSAGE(
    ok, "A string at maximum size should be valid");

  cfg.userSettings.params.wifi.station.ssid = {};

  ok = validateConfig(cfg);

  TEST_ASSERT_FALSE_MESSAGE(
    ok, "A non-optional string that is empty should be invalid");

  cfg.userSettings.params.wifi.station.ssid     = "mynicewifi";
  cfg.userSettings.params.wifi.station.password = {};

  ok = validateConfig(cfg);

  TEST_ASSERT_TRUE_MESSAGE(
    ok, "An optional string that is empty should be valid");
}

void test_validate_integer_range(void) {
  Config cfg;

  cfg.userSettings.params.watering.duration.max = 222;

  bool ok = validateConfig(cfg);
  TEST_ASSERT_FALSE_MESSAGE(
    ok, "An integer that is too big should be invalid");

  cfg.userSettings.params.watering.duration.max = 0;

  ok = validateConfig(cfg);
  TEST_ASSERT_FALSE_MESSAGE(
    ok, "An integer that is too small should be invalid");

  cfg.userSettings.params.watering.duration.max = 45;

  ok = validateConfig(cfg);
  TEST_ASSERT_TRUE_MESSAGE(
    ok, "An integer in its range should be valid");
}

// -----------------------------------------------------------------------------
// Cross tests
// -----------------------------------------------------------------------------

void test_cross_validate_duration(void) {
  Config cfg;

  cfg.userSettings.params.watering.duration.min  = 6;
  cfg.userSettings.params.watering.duration.max  = 12;
  cfg.userSettings.params.watering.duration.base = 9;
  cfg.userSettings.params.watering.duration.step = 3;

  bool ok = validateConfig(cfg);

  TEST_ASSERT_TRUE_MESSAGE(
    ok, "Duration with min < max should pass validation");

  cfg.userSettings.params.watering.duration.min = 12;
  cfg.userSettings.params.watering.duration.max = 6;

  ok = validateConfig(cfg);

  TEST_ASSERT_FALSE_MESSAGE(
    ok, "Duration with min > max should fail validation");

  cfg.userSettings.params.watering.duration.min  = 6;
  cfg.userSettings.params.watering.duration.max  = 12;
  cfg.userSettings.params.watering.duration.step = 0;

  ok = validateConfig(cfg);

  TEST_ASSERT_FALSE_MESSAGE(
    ok, "Duration with a null step should fail validation");

  cfg.userSettings.params.watering.duration.step = 7;

  ok = validateConfig(cfg);

  TEST_ASSERT_FALSE_MESSAGE(
    ok, "Duration with a step > max-min should fail validation");

  cfg.userSettings.params.watering.duration.step = 6;

  ok = validateConfig(cfg);

  TEST_ASSERT_TRUE_MESSAGE(
    ok, "Duration with a step <= max-min should pass validation");

  cfg.userSettings.params.watering.duration.base = 5;

  ok = validateConfig(cfg);

  TEST_ASSERT_FALSE_MESSAGE(
    ok, "Duration with a base value < min should fail validation");

  cfg.userSettings.params.watering.duration.base = 13;

  ok = validateConfig(cfg);

  TEST_ASSERT_FALSE_MESSAGE(
    ok, "Duration with a base value > max should fail validation");

  cfg.userSettings.params.watering.duration.base = 6;

  ok = validateConfig(cfg);

  TEST_ASSERT_TRUE_MESSAGE(
    ok, "Duration with a base value >= min should pass validation");

  cfg.userSettings.params.watering.duration.base = 12;

  ok = validateConfig(cfg);

  TEST_ASSERT_TRUE_MESSAGE(
    ok, "Duration with a base value <= max should pass validation");
}

void test_cross_validate_flow(void) {
  Config cfg;

  cfg.userSettings.params.watering.pump.flow.min = 10;
  cfg.userSettings.params.watering.pump.flow.max = 80;

  bool ok = validateConfig(cfg);

  TEST_ASSERT_TRUE_MESSAGE(
    ok, "Flow with min < max should pass validation");

  cfg.userSettings.params.watering.pump.flow.min = 80;
  cfg.userSettings.params.watering.pump.flow.max = 10;

  ok = validateConfig(cfg);

  TEST_ASSERT_FALSE_MESSAGE(
    ok, "Flow with min >= max should fail validation");
}

void test_cross_validate_line_zone(void) {
  using WateringModel = Config::UserSettings::WateringModel;

  Config cfg;

  Config::Zone zone;
  zone.id = UUID::generate();

  Config::Line line;
  line.id = UUID::generate();
  line.valves.set(3);
  line.zoneId = zone.id;

  Config::Line line2;
  line2.id = line.id;
  //line2.valves.set(5);

  auto zoneAddResult = cfg.userSettings.wateringModel.zones.add(zone);

  TEST_ASSERT_TRUE_MESSAGE(
    zoneAddResult == WateringModel::ZoneCollection::AddResult::Ok,
    "Zone should be added to config");

  auto lineAddResult = cfg.userSettings.wateringModel.lines.add(line);

  TEST_ASSERT_TRUE_MESSAGE(
    lineAddResult == WateringModel::LineCollection::AddResult::Ok,
    "Line should be added to config");

  lineAddResult = cfg.userSettings.wateringModel.lines.add(line2);

  TEST_ASSERT_TRUE_MESSAGE(
    lineAddResult != WateringModel::LineCollection::AddResult::Ok,
    "Line with existing ID should not be added to config");

  bool ok = validateConfig(cfg);

  TEST_ASSERT_TRUE_MESSAGE(
    ok, "Line with a reference to existing zone should pass validation");

  line2.id      = UUID::generate();
  lineAddResult = cfg.userSettings.wateringModel.lines.add(line2);

  TEST_ASSERT_TRUE_MESSAGE(
    lineAddResult == WateringModel::LineCollection::AddResult::Ok,
    "Line with different ID should be added to config");

  ok = validateConfig(cfg);

  TEST_ASSERT_FALSE_MESSAGE(
    ok, "Line with a reference to non-existing zone should fail validation");
}

// -----------------------------------------------------------------------------
// Main
// -----------------------------------------------------------------------------

int main() {
  UNITY_BEGIN();

  RUN_TEST(test_validate_default_config);
  RUN_TEST(test_validate_string_length);
  RUN_TEST(test_validate_integer_range);

  RUN_TEST(test_cross_validate_duration);
  RUN_TEST(test_cross_validate_flow);
  RUN_TEST(test_cross_validate_line_zone);

  return UNITY_END();
}
