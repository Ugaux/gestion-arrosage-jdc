#include "Validation.h"

namespace Validation {

namespace CrossFn {

Result validatePumpFlow(Config::UserSettings::Params::Watering::Pump::Flow& cfg) {

  if (cfg.min >= cfg.max)
    return Result(Result::Error::UnsupportedValue,
                  "pump flow min/max (should have min:%u<max:%u)",
                  cfg.min, cfg.max);

  return {};
}

Result validateDuration(Config::UserSettings::Params::Watering::Duration& cfg) {

  if (cfg.min >= cfg.max)
    return Result(
      Result::Error::UnsupportedValue,
      "duration min/max (should have min:%u<max:%u)",
      cfg.min, cfg.max);

  if (cfg.step == 0 || cfg.step > (cfg.max - cfg.min))
    return Result(
      Result::Error::UnsupportedValue,
      "duration step (should have 0<step:%u<=%u)",
      cfg.step, cfg.max - cfg.min);

  if (cfg.base < cfg.min || cfg.max < cfg.base)
    return Result(
      Result::Error::UnsupportedValue,
      "duration base (should have min<=%u<=max)",
      cfg.base);

  return {};
}

Result validateLineZone(Config::UserSettings::WateringModel& cfg) {

  auto& zones = cfg.zones;
  auto& lines = cfg.lines;

  for (uint8_t i = 0; i < lines.size(); i++) {
    auto& line = lines[i];

    if (!zones.find(line.zoneId)) {
      auto id = line.id.unparse();
      return Result(
        Result::Error::InvalidReference,
        "line '%.*s' points to a non-existing zone",
        static_cast<int>(id.size()), id.data());
    }
  }

  return {};
}

Result validateSchedule(Config::Schedule& cfg) {

  auto id = cfg.id.unparse();

  const bool hasDays = !cfg.days.isEmpty();

  switch (cfg.frequency) {
    case Frequency::EveryDay:
    case Frequency::EvenDays:
    case Frequency::OddDays:
      if (hasDays)
        return Result(
          Result::Error::UnsupportedValue,
          "schedule '%.*s' has days without"
          " the specific days frequency",
          static_cast<int>(id.size()), id.data());
      break;
    case Frequency::SpecificDays:
      if (!hasDays)
        return Result(
          Result::Error::UnsupportedValue,
          "schedule '%.*s' has the specific"
          " days frequency but no days",
          static_cast<int>(id.size()), id.data());
      break;
  }

  return {};
}

Result normalizeAndValidateSchedules(Config& cfg) {

  auto& lines     = cfg.userSettings.wateringModel.lines;
  auto& schedules = cfg.schedules;

  // Normalize
  //
  // Don't increment after removal: removeAt() swaps the last element into
  // the current index. Re-check that index, while size() reflects the removal.

  for (uint8_t i = 0; i < schedules.size();) {

    if (lines.find(schedules[i].lineId))
      i++;
    else {
      if (!schedules.removeAt(i)) {
        auto id = schedules[i].id.unparse();
        return Result(
          Result::Error::OperationFailed,
          "schedule '%.*s' could not be removed",
          static_cast<int>(id.size()), id.data());
      }
    }
  }

  // Validate

  for (uint8_t i = 0; i < schedules.size(); i++) {

    auto& schedule = schedules[i];

    if (!lines.find(schedule.lineId)) {
      auto id = schedule.id.unparse();
      return Result(
        Result::Error::InvalidReference,
        "schedule '%.*s' points to a non-existing line",
        static_cast<int>(id.size()), id.data());
    }
  }

  return {};
}

}  // namespace CrossFn

}  // namespace Validation
