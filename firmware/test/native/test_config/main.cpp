#include <unity.h>

#include "config/generated/Config.h"
#include "config/generated/ConfigReflection.h"
#include "config/runtime/Validation.h"

static constexpr char kUnexpectedValidationErrorPath[] =
  "Unexpected validation error path";
static constexpr char kUnexpectedValidationErrorMessage[] =
  "Unexpected validation error message";

// -----------------------------------------------------------------------------
// Test fixtures
// -----------------------------------------------------------------------------

void setUp() {}

void tearDown() {}

// -----------------------------------------------------------------------------
// Helpers
// -----------------------------------------------------------------------------

Validation::Result validateConfig(Config& cfg) {
  Validation::ValidationVisitor v;
  Reflection::visit(cfg, v);
  return v.result();
}

// -----------------------------------------------------------------------------
// Field tests
// -----------------------------------------------------------------------------

void test_validation_visitor(void) {
  Config cfg;

  Validation::ValidationVisitor v;

  TEST_ASSERT_TRUE_MESSAGE(
    Reflection::visit(cfg, v) && v.result().ok(),
    "Traversal should fully complete if validation passed");

  cfg.userSettings.params.watering.seasonal.factor = 0;

  TEST_ASSERT_TRUE_MESSAGE(
    !Reflection::visit(cfg, v) && !v.result().ok(),
    "Traversal should stop early if validation failed");
}

void test_validate_default_config(void) {
  Config cfg;

  TEST_ASSERT_TRUE_MESSAGE(
    validateConfig(cfg).ok(),
    "The default config should pass validation");
}

void test_validate_string_length(void) {
  Config cfg;

  cfg.userSettings.params.wifi.mdns = "ab";

  auto res = validateConfig(cfg);
  TEST_ASSERT_EQUAL_STRING_MESSAGE(
    "userSettings.params.wifi.mdns",
    res.path().data(),
    kUnexpectedValidationErrorPath);
  auto& mdnsLength =
    kReflection::ConfigUserSettingsParamsWifiMdns_FieldValidator.length;
  const std::string mdnsLengthAssertMsg =
    "length must be between min=" + std::to_string(mdnsLength.min)
    + " and max=" + std::to_string(mdnsLength.max) + ", got 2";
  TEST_ASSERT_EQUAL_STRING_MESSAGE(
    mdnsLengthAssertMsg.c_str(),
    res.message().data(),
    kUnexpectedValidationErrorMessage);
  TEST_ASSERT_TRUE_MESSAGE(
    !res.ok() && res.error() == Validation::Error::LengthOutOfRange,
    "A string that is too small should not pass validation");

  cfg.userSettings.params.wifi.mdns = "abcdefghijklmnopqrst";

  TEST_ASSERT_EQUAL_STRING_MESSAGE(
    "abcdefghijklmno",
    cfg.userSettings.params.wifi.mdns.c_str(),
    "A string should be cropped to its max allowable size");
  TEST_ASSERT_TRUE_MESSAGE(
    validateConfig(cfg).ok(),
    "A string at maximum size should pass validation");

  cfg.userSettings.params.wifi.station.ssid = {};

  res = validateConfig(cfg);
  TEST_ASSERT_EQUAL_STRING_MESSAGE(
    "userSettings.params.wifi.station.ssid",
    res.path().data(),
    kUnexpectedValidationErrorPath);
  auto& ssidLength =
    kReflection::ConfigUserSettingsParamsWifiStationSsid_FieldValidator.length;
  const std::string ssidLengthAssertMsg =
    "length must be between min=" + std::to_string(ssidLength.min)
    + " and max=" + std::to_string(ssidLength.max) + ", got 0";
  TEST_ASSERT_EQUAL_STRING_MESSAGE(
    ssidLengthAssertMsg.c_str(),
    res.message().data(),
    kUnexpectedValidationErrorMessage);
  TEST_ASSERT_TRUE_MESSAGE(
    !res.ok() && res.error() == Validation::Error::LengthOutOfRange,
    "A non-optional string that is empty should not pass validation");

  cfg.userSettings.params.wifi.station.ssid     = "mynicewifi";
  cfg.userSettings.params.wifi.station.password = {};

  TEST_ASSERT_TRUE_MESSAGE(
    validateConfig(cfg).ok(),
    "An optional string that is empty should pass validation");
}

void test_validate_integer_range(void) {
  Config cfg;

  auto& durationMaxRange =
    kReflection::ConfigUserSettingsParamsWateringDurationMax_FieldValidator.range;

  cfg.userSettings.params.watering.duration.max = 222;

  auto res = validateConfig(cfg);
  TEST_ASSERT_EQUAL_STRING_MESSAGE(
    "userSettings.params.watering.duration.max",
    res.path().data(),
    kUnexpectedValidationErrorPath);
  const std::string value1AssertMsg =
    "value must be between min=" + std::to_string(durationMaxRange.min)
    + " and max=" + std::to_string(durationMaxRange.max) + ", got 222";
  TEST_ASSERT_EQUAL_STRING_MESSAGE(
    value1AssertMsg.c_str(),
    res.message().data(),
    kUnexpectedValidationErrorMessage);
  TEST_ASSERT_TRUE_MESSAGE(
    !res.ok() && res.error() == Validation::Error::ValueOutOfRange,
    "An integer that is too big should not pass validation");

  cfg.userSettings.params.watering.duration.max = 0;

  res = validateConfig(cfg);
  TEST_ASSERT_EQUAL_STRING_MESSAGE(
    "userSettings.params.watering.duration.max",
    res.path().data(),
    kUnexpectedValidationErrorPath);
  const std::string value2AssertMsg =
    "value must be between min=" + std::to_string(durationMaxRange.min)
    + " and max=" + std::to_string(durationMaxRange.max) + ", got 0";
  TEST_ASSERT_EQUAL_STRING_MESSAGE(
    value2AssertMsg.c_str(),
    res.message().data(),
    kUnexpectedValidationErrorMessage);
  TEST_ASSERT_TRUE_MESSAGE(
    !res.ok() && res.error() == Validation::Error::ValueOutOfRange,
    "An integer that is too small should not pass validation");

  cfg.userSettings.params.watering.duration.max = 45;

  TEST_ASSERT_TRUE_MESSAGE(
    validateConfig(cfg).ok(),
    "An integer in its range should pass validation");
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

  TEST_ASSERT_TRUE_MESSAGE(
    validateConfig(cfg).ok(), "Duration with min < max should pass validation");

  cfg.userSettings.params.watering.duration.min = 12;
  cfg.userSettings.params.watering.duration.max = 6;

  auto res = validateConfig(cfg);
  TEST_ASSERT_EQUAL_STRING_MESSAGE(
    "userSettings.params.watering.duration",
    res.path().data(),
    kUnexpectedValidationErrorPath);
  TEST_ASSERT_EQUAL_STRING_MESSAGE(
    "value must be less than max=6, got 12",
    res.message().data(),
    kUnexpectedValidationErrorMessage);
  TEST_ASSERT_TRUE_MESSAGE(
    !res.ok() && res.error() == Validation::Error::UnsupportedValue,
    "Duration with min > max should fail validation");

  cfg.userSettings.params.watering.duration.min  = 6;
  cfg.userSettings.params.watering.duration.max  = 12;
  cfg.userSettings.params.watering.duration.step = 0;

  res = validateConfig(cfg);
  TEST_ASSERT_EQUAL_STRING_MESSAGE(
    "userSettings.params.watering.duration",
    res.path().data(),
    kUnexpectedValidationErrorPath);
  TEST_ASSERT_EQUAL_STRING_MESSAGE(
    "value must be > 0 and <= max-min=6, got 0",
    res.message().data(),
    kUnexpectedValidationErrorMessage);
  TEST_ASSERT_TRUE_MESSAGE(
    !res.ok() && res.error() == Validation::Error::UnsupportedValue,
    "Duration with a null step should fail validation");

  cfg.userSettings.params.watering.duration.step = 7;

  res = validateConfig(cfg);
  TEST_ASSERT_EQUAL_STRING_MESSAGE(
    "userSettings.params.watering.duration",
    res.path().data(),
    kUnexpectedValidationErrorPath);
  TEST_ASSERT_EQUAL_STRING_MESSAGE(
    "value must be > 0 and <= max-min=6, got 7",
    res.message().data(),
    kUnexpectedValidationErrorMessage);
  TEST_ASSERT_TRUE_MESSAGE(
    !res.ok() && res.error() == Validation::Error::UnsupportedValue,
    "Duration with a step > max-min should fail validation");

  cfg.userSettings.params.watering.duration.step = 6;

  TEST_ASSERT_TRUE_MESSAGE(
    validateConfig(cfg).ok(), "Duration with a step <= max-min should pass validation");

  cfg.userSettings.params.watering.duration.base = 5;

  res = validateConfig(cfg);
  TEST_ASSERT_EQUAL_STRING_MESSAGE(
    "userSettings.params.watering.duration",
    res.path().data(),
    kUnexpectedValidationErrorPath);
  TEST_ASSERT_EQUAL_STRING_MESSAGE(
    "value must be between min=6 and max=12, got 5",
    res.message().data(),
    kUnexpectedValidationErrorMessage);
  TEST_ASSERT_TRUE_MESSAGE(
    !res.ok() && res.error() == Validation::Error::UnsupportedValue,
    "Duration with a base value < min should fail validation");

  cfg.userSettings.params.watering.duration.base = 13;

  res = validateConfig(cfg);
  TEST_ASSERT_EQUAL_STRING_MESSAGE(
    "userSettings.params.watering.duration",
    res.path().data(),
    kUnexpectedValidationErrorPath);
  TEST_ASSERT_EQUAL_STRING_MESSAGE(
    "value must be between min=6 and max=12, got 13",
    res.message().data(),
    kUnexpectedValidationErrorMessage);
  TEST_ASSERT_TRUE_MESSAGE(
    !res.ok() && res.error() == Validation::Error::UnsupportedValue,
    "Duration with a base value > max should fail validation");

  cfg.userSettings.params.watering.duration.base = 6;

  TEST_ASSERT_TRUE_MESSAGE(
    validateConfig(cfg).ok(), "Duration with a base value >= min should pass validation");

  cfg.userSettings.params.watering.duration.base = 12;

  TEST_ASSERT_TRUE_MESSAGE(
    validateConfig(cfg).ok(), "Duration with a base value <= max should pass validation");
}

void test_cross_validate_flow(void) {
  Config cfg;

  cfg.userSettings.params.watering.pump.flow.min = 10;
  cfg.userSettings.params.watering.pump.flow.max = 80;

  TEST_ASSERT_TRUE_MESSAGE(
    validateConfig(cfg).ok(), "Flow with min < max should pass validation");

  cfg.userSettings.params.watering.pump.flow.min = 80;
  cfg.userSettings.params.watering.pump.flow.max = 10;

  auto res = validateConfig(cfg);
  TEST_ASSERT_EQUAL_STRING_MESSAGE(
    "userSettings.params.watering.pump.flow",
    res.path().data(),
    kUnexpectedValidationErrorPath);
  TEST_ASSERT_EQUAL_STRING_MESSAGE(
    "value must be less than max=10, got 80",
    res.message().data(),
    kUnexpectedValidationErrorMessage);
  TEST_ASSERT_TRUE_MESSAGE(
    !res.ok() && res.error() == Validation::Error::UnsupportedValue,
    "Flow with min >= max should fail validation");
}

void test_cross_validate_line_zone(void) {
  using WateringModel = Config::UserSettings::WateringModel;

  Config cfg;

  Config::Zone zone;
  zone.id = UUID::generate();

  Config::Line line0;
  line0.id = UUID::generate();
  line0.valves.set(3);
  line0.zoneId = zone.id;
  Config::Line line1;
  line1.id = line0.id;
  line1.valves.set(5);
  line1.zoneId = UUID::generate();

  TEST_ASSERT_TRUE_MESSAGE(
    cfg.userSettings.wateringModel.zones.add(zone)
        == WateringModel::ZoneCollection::AddResult::Ok
      && cfg.userSettings.wateringModel.zones.size() == 1,
    "Zone should have been added successfully");
  TEST_ASSERT_TRUE_MESSAGE(
    cfg.userSettings.wateringModel.lines.add(line0)
        == WateringModel::LineCollection::AddResult::Ok
      && cfg.userSettings.wateringModel.lines.size() == 1,
    "Line 0 should have been added successfully");
  TEST_ASSERT_TRUE_MESSAGE(
    cfg.userSettings.wateringModel.lines.add(line1)
        != WateringModel::LineCollection::AddResult::Ok
      && cfg.userSettings.wateringModel.lines.size() == 1,
    "Line with existing ID should not be added to config");

  TEST_ASSERT_TRUE_MESSAGE(
    validateConfig(cfg).ok(),
    "Line with a reference to existing zone should pass validation");

  line1.id = UUID::generate();
  TEST_ASSERT_TRUE_MESSAGE(
    cfg.userSettings.wateringModel.lines.add(line1)
        == WateringModel::LineCollection::AddResult::Ok
      && cfg.userSettings.wateringModel.lines.size() == 2,
    "Line 1 should have been added successfully");

  auto res = validateConfig(cfg);
  TEST_ASSERT_EQUAL_STRING_MESSAGE(
    "userSettings.wateringModel",
    res.path().data(),
    kUnexpectedValidationErrorPath);
  TEST_ASSERT_EQUAL_STRING_MESSAGE(
    "lines[1] points to a non-existing zone",
    res.message().data(),
    kUnexpectedValidationErrorMessage);
  TEST_ASSERT_TRUE_MESSAGE(
    !res.ok() && res.error() == Validation::Error::InvalidReference,
    "Line with a reference to non-existing zone should fail validation");
}

void test_cross_validate_line_valves(void) {
  using WateringModel = Config::UserSettings::WateringModel;

  Config cfg;

  Config::Zone zone;
  zone.id = UUID::generate();

  Config::Line line0;
  line0.id = UUID::generate();
  line0.valves.set(3);
  line0.zoneId = zone.id;
  Config::Line line1;
  line1.id = UUID::generate();
  line1.valves.set(5);
  line1.zoneId = zone.id;
  Config::Line line3;
  line3.id = UUID::generate();
  line3.valves.set(5);
  line3.zoneId = zone.id;

  TEST_ASSERT_TRUE_MESSAGE(
    cfg.userSettings.wateringModel.zones.add(zone)
        == WateringModel::ZoneCollection::AddResult::Ok
      && cfg.userSettings.wateringModel.zones.size() == 1,
    "Zone should have been added successfully");
  TEST_ASSERT_TRUE_MESSAGE(
    cfg.userSettings.wateringModel.lines.add(line0)
        == WateringModel::LineCollection::AddResult::Ok
      && cfg.userSettings.wateringModel.lines.size() == 1,
    "Line 0 should have been added successfully");
  TEST_ASSERT_TRUE_MESSAGE(
    cfg.userSettings.wateringModel.lines.add(line1)
        == WateringModel::LineCollection::AddResult::Ok
      && cfg.userSettings.wateringModel.lines.size() == 2,
    "Line 1 should have been added successfully");

  TEST_ASSERT_TRUE_MESSAGE(
    validateConfig(cfg).ok(),
    "Multiple lines with different valves should pass validation");

  TEST_ASSERT_TRUE_MESSAGE(
    cfg.userSettings.wateringModel.lines.add(line3)
        == WateringModel::LineCollection::AddResult::Ok
      && cfg.userSettings.wateringModel.lines.size() == 3,
    "Line 2 should have been added successfully");

  auto res = validateConfig(cfg);
  TEST_ASSERT_EQUAL_STRING_MESSAGE(
    "userSettings.wateringModel",
    res.path().data(),
    kUnexpectedValidationErrorPath);
  TEST_ASSERT_EQUAL_STRING_MESSAGE(
    "lines[2] uses valve 6, which is already used by another line",
    res.message().data(),
    kUnexpectedValidationErrorMessage);
  TEST_ASSERT_TRUE_MESSAGE(
    !res.ok() && res.error() == Validation::Error::DuplicateValue,
    "Multiple lines using the same valves shoud fail validation");
}

void test_cross_validate_schedule(void) {
  using WateringModel = Config::UserSettings::WateringModel;

  Config cfg;

  Config::Zone zone;
  zone.id = UUID::generate();

  Config::Line line;
  line.id = UUID::generate();
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

  Config::Schedule schedule0;
  schedule0.id        = UUID::generate();
  schedule0.lineId    = line.id;
  schedule0.frequency = Frequency::SpecificDays;
  schedule0.days      = WeekDay::Monday | WeekDay::Sunday;
  Config::Schedule schedule1;
  schedule1.id        = UUID::generate();
  schedule1.lineId    = line.id;
  schedule1.frequency = Frequency::SpecificDays;
  schedule1.days.clear();
  Config::Schedule schedule2;
  schedule2.id        = UUID::generate();
  schedule2.lineId    = line.id;
  schedule2.frequency = Frequency::EveryDay;
  schedule2.days      = WeekDay::Monday | WeekDay::Sunday;

  TEST_ASSERT_TRUE_MESSAGE(
    cfg.schedules.add(schedule0)
        == Config::ScheduleCollection::AddResult::Ok
      && cfg.schedules.size() == 1,
    "Schedule 0 should have been added successfully");

  TEST_ASSERT_TRUE_MESSAGE(
    validateConfig(cfg).ok(),
    "Schedule with specific frequency that"
    " contains days should pass validation");

  TEST_ASSERT_TRUE_MESSAGE(
    cfg.schedules.add(schedule1)
        == Config::ScheduleCollection::AddResult::Ok
      && cfg.schedules.size() == 2,
    "Schedule 1 should have been added successfully");

  auto res = validateConfig(cfg);
  TEST_ASSERT_EQUAL_STRING_MESSAGE(
    "schedules[1]",
    res.path().data(),
    kUnexpectedValidationErrorPath);
  TEST_ASSERT_EQUAL_STRING_MESSAGE(
    "has specific days frequency but contains no days",
    res.message().data(),
    kUnexpectedValidationErrorMessage);
  TEST_ASSERT_TRUE_MESSAGE(
    !res.ok() && res.error() == Validation::Error::UnsupportedValue,
    "Schedule containing no days with specific frequency should not pass validation");

  TEST_ASSERT_TRUE_MESSAGE(
    cfg.schedules.remove(schedule1.id)
      && cfg.schedules.size() == 1,
    "Schedule 1 should have been removed successfully");
  TEST_ASSERT_TRUE_MESSAGE(
    cfg.schedules.add(schedule2)
        == Config::ScheduleCollection::AddResult::Ok
      && cfg.schedules.size() == 2,
    "Schedule 2 should have been added successfully");

  res = validateConfig(cfg);
  TEST_ASSERT_EQUAL_STRING_MESSAGE(
    "schedules[1]",
    res.path().data(),
    kUnexpectedValidationErrorPath);
  TEST_ASSERT_EQUAL_STRING_MESSAGE(
    "contains days without specific days frequency",
    res.message().data(),
    kUnexpectedValidationErrorMessage);
  TEST_ASSERT_TRUE_MESSAGE(
    !res.ok() && res.error() == Validation::Error::UnsupportedValue,
    "Schedule containing days without specific frequency should not pass validation");
}

void test_cross_normalize_and_validate_schedules(void) {
  using WateringModel = Config::UserSettings::WateringModel;

  Config cfg;

  Config::Zone zone;
  zone.id = UUID::generate();

  Config::Line line;
  line.id = UUID::generate();
  line.valves.set(3);
  line.zoneId = zone.id;

  Config::Schedule schedule0;
  schedule0.id     = UUID::generate();
  schedule0.lineId = line.id;
  Config::Schedule schedule1;
  schedule1.id     = UUID::generate();
  schedule1.lineId = UUID::generate();

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
  TEST_ASSERT_TRUE_MESSAGE(
    cfg.schedules.add(schedule0)
        == Config::ScheduleCollection::AddResult::Ok
      && cfg.schedules.size() == 1,
    "Schedule 1 should have been added successfully");

  TEST_ASSERT_TRUE_MESSAGE(
    validateConfig(cfg).ok() && cfg.schedules.size() == 1,
    "Schedule with a reference to existing line"
    " should not be normalized and pass validation");

  TEST_ASSERT_TRUE_MESSAGE(
    cfg.schedules.add(schedule1)
        == Config::ScheduleCollection::AddResult::Ok
      && cfg.schedules.size() == 2,
    "Schedule 2 should have been added successfully");

  TEST_ASSERT_TRUE_MESSAGE(
    validateConfig(cfg).ok() && cfg.schedules.size() == 1,
    "Schedule with a reference to non-existing line"
    " should be normalized and pass validation");

  uint8_t totalSchedules     = 1;
  uint8_t currentScheduleIdx = 3;
  for (uint8_t i = 0; i < Config::kMaxSchedulePerLine; i++) {
    Config::Schedule scheduleForMaxTest;
    scheduleForMaxTest.id     = UUID::generate();
    scheduleForMaxTest.lineId = line.id;

    const std::string assertMsg =
      "Schedule " + std::to_string(currentScheduleIdx + i)
      + " should have been added successfully";
    TEST_ASSERT_TRUE_MESSAGE(
      cfg.schedules.add(scheduleForMaxTest)
          == Config::ScheduleCollection::AddResult::Ok
        && cfg.schedules.size() == ++totalSchedules,
      assertMsg.c_str());
  }

  auto res = validateConfig(cfg);

  TEST_ASSERT_EQUAL_STRING_MESSAGE(
    "", res.path().data(),
    kUnexpectedValidationErrorPath);
  const std::string expected =
    "lines[0] should have at most "
    + std::to_string(Config::kMaxSchedulePerLine)
    + " schedules, got " + std::to_string(totalSchedules);
  TEST_ASSERT_EQUAL_STRING_MESSAGE(
    expected.c_str(), res.message().data(),
    kUnexpectedValidationErrorMessage);
  TEST_ASSERT_TRUE_MESSAGE(
    !res.ok() && res.error() == Validation::Error::UnsupportedValue,
    "Line containing too many schedules should not pass validation");
}

// -----------------------------------------------------------------------------
// Main
// -----------------------------------------------------------------------------

int main() {
  UNITY_BEGIN();

  RUN_TEST(test_validation_visitor);

  RUN_TEST(test_validate_default_config);
  RUN_TEST(test_validate_string_length);
  RUN_TEST(test_validate_integer_range);

  RUN_TEST(test_cross_validate_duration);
  RUN_TEST(test_cross_validate_flow);
  RUN_TEST(test_cross_validate_line_zone);
  RUN_TEST(test_cross_validate_line_valves);
  RUN_TEST(test_cross_validate_schedule);
  RUN_TEST(test_cross_normalize_and_validate_schedules);

  return UNITY_END();
}
