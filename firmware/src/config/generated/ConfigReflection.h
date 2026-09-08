#pragma once

#include <tuple>
#include "config/generated/Config.h"
#include "config/runtime/Reflection.h"
#include "config/runtime/Validation.h"

namespace kReflection {

inline constexpr uint8_t SchemaVersion = 1;

// ============================================================
// Config::Zone
// ============================================================

inline constexpr Validation::FieldValidator
  ConfigZoneName_FieldValidator{
    &Validation::lengthAdapter<
      decltype(Config::Zone::name)>,
    Validation::FieldValidator::LengthData{
      .min = 3, .max = 15 },
  };

// ============================================================
// Config::Line
// ============================================================

inline constexpr Validation::FieldValidator
  ConfigLineName_FieldValidator{
    &Validation::lengthAdapter<
      decltype(Config::Line::name)>,
    Validation::FieldValidator::LengthData{
      .min = 3, .max = 15 },
  };

inline constexpr Validation::FieldValidator
  ConfigLineValves_FieldValidator{
    &Validation::rangeAdapter<
      decltype(Config::Line::valves)>,
    Validation::FieldValidator::RangeData{
      .min = 1, .max = kValveCount },
  };

// ============================================================
// Config::Schedule
// ============================================================

inline constexpr Validation::CrossAction
  ConfigSchedule_CrossValidator{
    &Validation::crossAdapter<
      Config::Schedule,
      Validation::CrossFn::validateSchedule>
  };

inline constexpr Validation::FieldValidator
  ConfigScheduleName_FieldValidator{
    &Validation::lengthAdapter<
      decltype(Config::Schedule::name)>,
    Validation::FieldValidator::LengthData{
      .min = 3, .max = 15 },
  };

inline constexpr Validation::FieldValidator
  ConfigScheduleHour_FieldValidator{
    &Validation::rangeAdapter<
      decltype(Config::Schedule::hour)>,
    Validation::FieldValidator::RangeData{
      .min = 0, .max = 23 },
  };

inline constexpr Validation::FieldValidator
  ConfigScheduleMinute_FieldValidator{
    &Validation::rangeAdapter<
      decltype(Config::Schedule::minute)>,
    Validation::FieldValidator::RangeData{
      .min = 0, .max = 59 },
  };

inline constexpr Validation::FieldValidator
  ConfigScheduleDuration_FieldValidator{
    &Validation::rangeAdapter<
      decltype(Config::Schedule::duration)>,
    Validation::FieldValidator::RangeData{
      .min = 1, .max = 45 },
  };

// ============================================================
// Config::UserSettings::Params::Wifi::Station
// ============================================================

inline constexpr Validation::FieldValidator
  ConfigUserSettingsParamsWifiStationSsid_FieldValidator{
    &Validation::lengthAdapter<
      decltype(Config::UserSettings::Params::Wifi::Station::ssid)>,
    Validation::FieldValidator::LengthData{
      .min = 5, .max = 32 },
  };

inline constexpr Validation::FieldValidator
  ConfigUserSettingsParamsWifiStationPassword_FieldValidator{
    &Validation::lengthAdapter<
      decltype(Config::UserSettings::Params::Wifi::Station::password)>,
    Validation::FieldValidator::LengthData{
      .min = 8, .max = 63 },
  };

// ============================================================
// Config::UserSettings::Params::Wifi::AP
// ============================================================

inline constexpr Validation::FieldValidator
  ConfigUserSettingsParamsWifiAPSsid_FieldValidator{
    &Validation::lengthAdapter<
      decltype(Config::UserSettings::Params::Wifi::AP::ssid)>,
    Validation::FieldValidator::LengthData{
      .min = 5, .max = 32 },
  };

inline constexpr Validation::FieldValidator
  ConfigUserSettingsParamsWifiAPPassword_FieldValidator{
    &Validation::lengthAdapter<
      decltype(Config::UserSettings::Params::Wifi::AP::password)>,
    Validation::FieldValidator::LengthData{
      .min = 8, .max = 63 },
  };

// ============================================================
// Config::UserSettings::Params::Wifi
// ============================================================

inline constexpr Validation::FieldValidator
  ConfigUserSettingsParamsWifiMdns_FieldValidator{
    &Validation::lengthAdapter<
      decltype(Config::UserSettings::Params::Wifi::mdns)>,
    Validation::FieldValidator::LengthData{
      .min = 5, .max = 15 },
  };

// ============================================================
// Config::UserSettings::Params::Watering::Duration
// ============================================================

inline constexpr Validation::CrossAction
  ConfigUserSettingsParamsWateringDuration_CrossValidator{
    &Validation::crossAdapter<
      Config::UserSettings::Params::Watering::Duration,
      Validation::CrossFn::validateDuration>
  };

inline constexpr Validation::FieldValidator
  ConfigUserSettingsParamsWateringDurationMin_FieldValidator{
    &Validation::rangeAdapter<
      decltype(Config::UserSettings::Params::Watering::Duration::min)>,
    Validation::FieldValidator::RangeData{
      .min = 1, .max = 60 },
  };

inline constexpr Validation::FieldValidator
  ConfigUserSettingsParamsWateringDurationMax_FieldValidator{
    &Validation::rangeAdapter<
      decltype(Config::UserSettings::Params::Watering::Duration::max)>,
    Validation::FieldValidator::RangeData{
      .min = 1, .max = 60 },
  };

inline constexpr Validation::FieldValidator
  ConfigUserSettingsParamsWateringDurationBase_FieldValidator{
    &Validation::rangeAdapter<
      decltype(Config::UserSettings::Params::Watering::Duration::base)>,
    Validation::FieldValidator::RangeData{
      .min = 1, .max = 60 },
  };

inline constexpr Validation::FieldValidator
  ConfigUserSettingsParamsWateringDurationStep_FieldValidator{
    &Validation::rangeAdapter<
      decltype(Config::UserSettings::Params::Watering::Duration::step)>,
    Validation::FieldValidator::RangeData{
      .min = 1, .max = 15 },
  };

// ============================================================
// Config::UserSettings::Params::Watering::Seasonal
// ============================================================

inline constexpr Validation::FieldValidator
  ConfigUserSettingsParamsWateringSeasonalFactor_FieldValidator{
    &Validation::rangeAdapter<
      decltype(Config::UserSettings::Params::Watering::Seasonal::factor)>,
    Validation::FieldValidator::RangeData{
      .min = 1, .max = 100 },
  };

// ============================================================
// Config::UserSettings::Params::Watering::Soil::Moisture
// ============================================================

inline constexpr Validation::FieldValidator
  ConfigUserSettingsParamsWateringSoilMoistureThreshold_FieldValidator{
    &Validation::rangeAdapter<
      decltype(Config::UserSettings::Params::Watering::Soil::Moisture::threshold)>,
    Validation::FieldValidator::RangeData{
      .min = 10, .max = 90 },
  };

// ============================================================
// Config::UserSettings::Params::Watering::Pump::Flow
// ============================================================

inline constexpr Validation::CrossAction
  ConfigUserSettingsParamsWateringPumpFlow_CrossValidator{
    &Validation::crossAdapter<
      Config::UserSettings::Params::Watering::Pump::Flow,
      Validation::CrossFn::validatePumpFlow>
  };

inline constexpr Validation::FieldValidator
  ConfigUserSettingsParamsWateringPumpFlowMin_FieldValidator{
    &Validation::rangeAdapter<
      decltype(Config::UserSettings::Params::Watering::Pump::Flow::min)>,
    Validation::FieldValidator::RangeData{
      .min = 2, .max = 200 },
  };

inline constexpr Validation::FieldValidator
  ConfigUserSettingsParamsWateringPumpFlowMax_FieldValidator{
    &Validation::rangeAdapter<
      decltype(Config::UserSettings::Params::Watering::Pump::Flow::max)>,
    Validation::FieldValidator::RangeData{
      .min = 2, .max = 200 },
  };

// ============================================================
// Config::UserSettings::WateringModel
// ============================================================

inline constexpr Validation::CrossAction
  ConfigUserSettingsWateringModel_CrossValidator{
    &Validation::crossAdapter<
      Config::UserSettings::WateringModel,
      Validation::CrossFn::validateLineZone>
  };

// ============================================================
// Config
// ============================================================

inline constexpr Validation::CrossAction
  Config_CrossValidator{
    &Validation::crossAdapter<
      Config,
      Validation::CrossFn::normalizeAndValidateSchedules>
  };

}  // namespace kReflection

namespace Reflection {

template<>
struct Schema<Config::Zone> {
  static constexpr bool reflected = true;

  static constexpr auto fields = std::tuple{
    makeField(
      &Config::Zone::id,
      "id",
      nullptr,
      "",
      true),

    makeField(
      &Config::Zone::name,
      "name",
      &kReflection::ConfigZoneName_FieldValidator,
      "",
      false),
  };

  static constexpr const Validation::CrossAction*
    crossAction = nullptr;
};

template<>
struct Schema<Config::Line> {
  static constexpr bool reflected = true;

  static constexpr auto fields = std::tuple{
    makeField(
      &Config::Line::id,
      "id",
      nullptr,
      "",
      true),

    makeField(
      &Config::Line::zoneId,
      "zoneId",
      nullptr,
      "",
      false),

    makeField(
      &Config::Line::name,
      "name",
      &kReflection::ConfigLineName_FieldValidator,
      "",
      false),

    makeField(
      &Config::Line::valves,
      "valves",
      &kReflection::ConfigLineValves_FieldValidator,
      "",
      false),
  };

  static constexpr const Validation::CrossAction*
    crossAction = nullptr;
};

template<>
struct Schema<Config::Schedule> {
  static constexpr bool reflected = true;

  static constexpr auto fields = std::tuple{
    makeField(
      &Config::Schedule::id,
      "id",
      nullptr,
      "",
      false),

    makeField(
      &Config::Schedule::lineId,
      "lineId",
      nullptr,
      "",
      false),

    makeField(
      &Config::Schedule::enabled,
      "enabled",
      nullptr,
      "",
      true),

    makeField(
      &Config::Schedule::name,
      "name",
      &kReflection::ConfigScheduleName_FieldValidator,
      "",
      true),

    makeField(
      &Config::Schedule::hour,
      "hour",
      &kReflection::ConfigScheduleHour_FieldValidator,
      "",
      false),

    makeField(
      &Config::Schedule::minute,
      "minute",
      &kReflection::ConfigScheduleMinute_FieldValidator,
      "",
      false),

    makeField(
      &Config::Schedule::duration,
      "duration",
      &kReflection::ConfigScheduleDuration_FieldValidator,
      "minutes",
      false),

    makeField(
      &Config::Schedule::onlyIfDrySoil,
      "onlyIfDrySoil",
      nullptr,
      "",
      false),

    makeField(
      &Config::Schedule::frequency,
      "frequency",
      nullptr,
      "",
      false),

    makeField(
      &Config::Schedule::days,
      "days",
      nullptr,
      "",
      true),
  };

  static constexpr const Validation::CrossAction*
    crossAction = &kReflection::ConfigSchedule_CrossValidator;
};

template<>
struct Schema<Config::UserSettings::Params::Wifi::Station> {
  static constexpr bool reflected = true;

  static constexpr auto fields = std::tuple{
    makeField(
      &Config::UserSettings::Params::Wifi::Station::ssid,
      "ssid",
      &kReflection::ConfigUserSettingsParamsWifiStationSsid_FieldValidator,
      "",
      false),

    makeField(
      &Config::UserSettings::Params::Wifi::Station::password,
      "password",
      &kReflection::ConfigUserSettingsParamsWifiStationPassword_FieldValidator,
      "",
      true),
  };

  static constexpr const Validation::CrossAction*
    crossAction = nullptr;
};

template<>
struct Schema<Config::UserSettings::Params::Wifi::AP> {
  static constexpr bool reflected = true;

  static constexpr auto fields = std::tuple{
    makeField(
      &Config::UserSettings::Params::Wifi::AP::ssid,
      "ssid",
      &kReflection::ConfigUserSettingsParamsWifiAPSsid_FieldValidator,
      "",
      false),

    makeField(
      &Config::UserSettings::Params::Wifi::AP::password,
      "password",
      &kReflection::ConfigUserSettingsParamsWifiAPPassword_FieldValidator,
      "",
      true),
  };

  static constexpr const Validation::CrossAction*
    crossAction = nullptr;
};

template<>
struct Schema<Config::UserSettings::Params::Wifi> {
  static constexpr bool reflected = true;

  static constexpr auto fields = std::tuple{
    makeField(
      &Config::UserSettings::Params::Wifi::mdns,
      "mdns",
      &kReflection::ConfigUserSettingsParamsWifiMdns_FieldValidator,
      "",
      false),

    makeField(
      &Config::UserSettings::Params::Wifi::useAPMode,
      "useAPMode",
      nullptr,
      "",
      false),

    makeField(
      &Config::UserSettings::Params::Wifi::station,
      "station",
      nullptr,
      "",
      false),

    makeField(
      &Config::UserSettings::Params::Wifi::ap,
      "access-point",
      nullptr,
      "",
      false),
  };

  static constexpr const Validation::CrossAction*
    crossAction = nullptr;
};

template<>
struct Schema<Config::UserSettings::Params::Watering::Duration> {
  static constexpr bool reflected = true;

  static constexpr auto fields = std::tuple{
    makeField(
      &Config::UserSettings::Params::Watering::Duration::min,
      "min",
      &kReflection::ConfigUserSettingsParamsWateringDurationMin_FieldValidator,
      "minutes",
      false),

    makeField(
      &Config::UserSettings::Params::Watering::Duration::max,
      "max",
      &kReflection::ConfigUserSettingsParamsWateringDurationMax_FieldValidator,
      "minutes",
      false),

    makeField(
      &Config::UserSettings::Params::Watering::Duration::base,
      "base",
      &kReflection::ConfigUserSettingsParamsWateringDurationBase_FieldValidator,
      "minutes",
      false),

    makeField(
      &Config::UserSettings::Params::Watering::Duration::step,
      "step",
      &kReflection::ConfigUserSettingsParamsWateringDurationStep_FieldValidator,
      "minutes",
      false),
  };

  static constexpr const Validation::CrossAction*
    crossAction = &kReflection::ConfigUserSettingsParamsWateringDuration_CrossValidator;
};

template<>
struct Schema<Config::UserSettings::Params::Watering::Seasonal> {
  static constexpr bool reflected = true;

  static constexpr auto fields = std::tuple{
    makeField(
      &Config::UserSettings::Params::Watering::Seasonal::factor,
      "factor",
      &kReflection::ConfigUserSettingsParamsWateringSeasonalFactor_FieldValidator,
      "%",
      false),
  };

  static constexpr const Validation::CrossAction*
    crossAction = nullptr;
};

template<>
struct Schema<Config::UserSettings::Params::Watering::Soil::Moisture> {
  static constexpr bool reflected = true;

  static constexpr auto fields = std::tuple{
    makeField(
      &Config::UserSettings::Params::Watering::Soil::Moisture::threshold,
      "threshold",
      &kReflection::ConfigUserSettingsParamsWateringSoilMoistureThreshold_FieldValidator,
      "%",
      false),
  };

  static constexpr const Validation::CrossAction*
    crossAction = nullptr;
};

template<>
struct Schema<Config::UserSettings::Params::Watering::Soil> {
  static constexpr bool reflected = true;

  static constexpr auto fields = std::tuple{
    makeField(
      &Config::UserSettings::Params::Watering::Soil::moisture,
      "moisture",
      nullptr,
      "",
      false),
  };

  static constexpr const Validation::CrossAction*
    crossAction = nullptr;
};

template<>
struct Schema<Config::UserSettings::Params::Watering::Pump::Flow> {
  static constexpr bool reflected = true;

  static constexpr auto fields = std::tuple{
    makeField(
      &Config::UserSettings::Params::Watering::Pump::Flow::min,
      "min",
      &kReflection::ConfigUserSettingsParamsWateringPumpFlowMin_FieldValidator,
      "L/min",
      false),

    makeField(
      &Config::UserSettings::Params::Watering::Pump::Flow::max,
      "max",
      &kReflection::ConfigUserSettingsParamsWateringPumpFlowMax_FieldValidator,
      "L/min",
      false),
  };

  static constexpr const Validation::CrossAction*
    crossAction = &kReflection::ConfigUserSettingsParamsWateringPumpFlow_CrossValidator;
};

template<>
struct Schema<Config::UserSettings::Params::Watering::Pump> {
  static constexpr bool reflected = true;

  static constexpr auto fields = std::tuple{
    makeField(
      &Config::UserSettings::Params::Watering::Pump::flow,
      "flow",
      nullptr,
      "",
      false),
  };

  static constexpr const Validation::CrossAction*
    crossAction = nullptr;
};

template<>
struct Schema<Config::UserSettings::Params::Watering> {
  static constexpr bool reflected = true;

  static constexpr auto fields = std::tuple{
    makeField(
      &Config::UserSettings::Params::Watering::duration,
      "duration",
      nullptr,
      "",
      false),

    makeField(
      &Config::UserSettings::Params::Watering::seasonal,
      "seasonal",
      nullptr,
      "",
      false),

    makeField(
      &Config::UserSettings::Params::Watering::soil,
      "soil",
      nullptr,
      "",
      false),

    makeField(
      &Config::UserSettings::Params::Watering::pump,
      "pump",
      nullptr,
      "",
      false),
  };

  static constexpr const Validation::CrossAction*
    crossAction = nullptr;
};

template<>
struct Schema<Config::UserSettings::Params> {
  static constexpr bool reflected = true;

  static constexpr auto fields = std::tuple{
    makeField(
      &Config::UserSettings::Params::wifi,
      "wifi",
      nullptr,
      "",
      false),

    makeField(
      &Config::UserSettings::Params::watering,
      "watering",
      nullptr,
      "",
      false),
  };

  static constexpr const Validation::CrossAction*
    crossAction = nullptr;
};

template<>
struct Schema<Config::UserSettings::WateringModel> {
  static constexpr bool reflected = true;

  static constexpr auto fields = std::tuple{
    makeField(
      &Config::UserSettings::WateringModel::zones,
      "zones",
      nullptr,
      "",
      false),

    makeField(
      &Config::UserSettings::WateringModel::lines,
      "lines",
      nullptr,
      "",
      false),
  };

  static constexpr const Validation::CrossAction*
    crossAction = &kReflection::ConfigUserSettingsWateringModel_CrossValidator;
};

template<>
struct Schema<Config::UserSettings> {
  static constexpr bool reflected = true;

  static constexpr auto fields = std::tuple{
    makeField(
      &Config::UserSettings::params,
      "params",
      nullptr,
      "",
      false),

    makeField(
      &Config::UserSettings::wateringModel,
      "wateringModel",
      nullptr,
      "",
      false),
  };

  static constexpr const Validation::CrossAction*
    crossAction = nullptr;
};

template<>
struct Schema<Config> {
  static constexpr bool reflected = true;

  static constexpr auto fields = std::tuple{
    makeField(
      &Config::userSettings,
      "userSettings",
      nullptr,
      "",
      false),

    makeField(
      &Config::schedules,
      "schedules",
      nullptr,
      "",
      false),
  };

  static constexpr const Validation::CrossAction*
    crossAction = &kReflection::Config_CrossValidator;
};

}  // namespace Reflection
