#include "Validation.h"

namespace Validation {

namespace CrossFn {

Result validatePumpFlow(Config::UserSettings::Params::Watering::Pump::Flow& cfg) {

  if (cfg.min >= cfg.max)
    return Result(
      Error::UnsupportedValue,
      "min must be less than max=%u, got %u",
      cfg.max, cfg.min);

  return {};
}

Result validateDuration(Config::UserSettings::Params::Watering::Duration& cfg) {

  if (cfg.min >= cfg.max)
    return Result(
      Error::UnsupportedValue,
      "min must be less than max=%u, got %u",
      cfg.max, cfg.min);

  if (cfg.base < cfg.min || cfg.base > cfg.max)
    return Result(
      Error::UnsupportedValue,
      "base must be between min=%d and max=%d, got %u",
      cfg.min, cfg.max, cfg.base);

  if (cfg.step <= 0 || cfg.step > (cfg.max - cfg.min))
    return Result(
      Error::UnsupportedValue,
      "step must be > 0 and <= max-min=%u, got %u",
      cfg.max - cfg.min, cfg.step);

  return {};
}

Result validateLines(Config::UserSettings::WateringModel& cfg) {

  auto& zones = cfg.zones;
  auto& lines = cfg.lines;

  for (uint8_t i = 0; i < lines.size(); i++) {

    if (!zones.find(lines[i].zoneId))
      return Result(
        Error::InvalidReference,
        "lines[%u] points to a non-existing zone", i);
  }

  Config::Line::ValveSet           seenValves;
  std::array<uint8_t, kValveCount> valveOwners;

  for (uint8_t lineIdx = 0; lineIdx < lines.size(); lineIdx++) {
    const auto duplicatedValves = seenValves & lines[lineIdx].valves;

    if (duplicatedValves.any()) {
      for (size_t valve = 0; valve < duplicatedValves.size(); valve++) {
        if (duplicatedValves.test(valve)) {
          return Result(
            Error::DuplicateValue,
            "lines[%u]: valve %zu is already used by lines[%u]",
            lineIdx, valve + 1, valveOwners[valve]);
        }
      }
    }

    for (size_t valve = 0; valve < lines[lineIdx].valves.size(); valve++) {
      if (lines[lineIdx].valves.test(valve))
        valveOwners[valve] = lineIdx;
    }

    seenValves |= lines[lineIdx].valves;
  }

  return {};
}

Result validateSchedule(Config::Schedule& cfg) {

  const bool hasDays = !cfg.days.isEmpty();

  switch (cfg.frequency) {
    case Frequency::EveryDay:
    case Frequency::EvenDays:
    case Frequency::OddDays:
      if (hasDays)
        return Result(
          Error::UnsupportedValue,
          "contains days without specific days frequency");
      break;
    case Frequency::SpecificDays:
      if (!hasDays)
        return Result(
          Error::UnsupportedValue,
          "has specific days frequency but contains no days");
      break;
  }

  return {};
}

Result normalizeAndValidateSchedules(Config& cfg) {

  auto& lines     = cfg.userSettings.wateringModel.lines;
  auto& schedules = cfg.schedules;

  // Normalize (filters schedules that point to non-existing lines)
  //
  // Don't increment after removal: removeAt() swaps the last element into
  // the current index. Re-check that index, while size() reflects the removal.

  for (uint8_t i = 0; i < schedules.size();) {

    if (lines.find(schedules[i].lineId))
      i++;
    else {
      if (!schedules.removeAt(i))
        return Result(
          Error::OperationFailed,
          "schedules[%u] could not be removed", i);
    }
  }

  // Validate line ID

  for (uint8_t i = 0; i < schedules.size(); i++) {

    if (!lines.find(schedules[i].lineId))
      return Result(
        Error::InvalidReference,
        "schedules[%u] points to a non-existing line", i);
  }

  // Validate max schedules per line

  std::array< uint8_t,
              Config::UserSettings::WateringModel::
                LineCollection::kCapacity>
    scheduleCounts{};

  for (uint8_t i = 0; i < schedules.size(); i++) {

    for (uint8_t j = 0; j < lines.size(); j++) {

      if (schedules[i].lineId == lines[j].id) {

        if (++scheduleCounts[j] > Config::kMaxSchedulePerLine)
          return Result(
            Error::UnsupportedValue,
            "lines[%u] should have at most %u schedules, got %u",
            j, Config::kMaxSchedulePerLine, scheduleCounts[j]);

        break;
      }
    }
  }

  return {};
}

}  // namespace CrossFn

}  // namespace Validation
