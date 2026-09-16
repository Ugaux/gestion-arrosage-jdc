#include "JsonSerializer.h"

#include "config/runtime/JsonDeserializer.h"

void JsonSerializer::serializeWateringModel(Config::UserSettings::WateringModel& model) {

  // Mirrors deserializeWateringModel(): "zones" and "lines" are flattened
  // directly into the enclosing JSON object rather than nested under a
  // "wateringModel" key, and each zone's "lines" array is reconstructed
  // from model.lines by matching zoneId, in the same way the deserializer
  // stamps zoneId onto each line as it flattens the nested shape back out.

  const auto zoneArrayKey = "zones";
  m_pathBuilder.enter(zoneArrayKey);

  JsonArray zoneArrayJson = jsonFor(zoneArrayKey).to<JsonArray>();

  for (size_t i = 0; i < model.zones.size(); ++i) {
    Config::Zone& zone = model.zones[i];

    m_pathBuilder.index(i);

    JsonVariant zoneJson = zoneArrayJson.add<JsonVariant>();

    serializeObject(zoneJson, zone,
                    Reflection::NoFilter{}, true);
    if (!m_result)
      return;

    const auto lineArrayKey = "lines";
    m_pathBuilder.enter(lineArrayKey);

    JsonArray lineArrayJson = zoneJson[lineArrayKey].to<JsonArray>();

    size_t lineIndex = 0;
    for (size_t j = 0; j < model.lines.size(); ++j) {
      Config::Line& line = model.lines[j];

      if (!(line.zoneId == zone.id))
        continue;

      m_pathBuilder.index(lineIndex);

      JsonVariant lineJson = lineArrayJson.add<JsonVariant>();

      // LineFilter skips "zoneId": it's implied by nesting under this
      // zone, same as on the read side.
      serializeObject(lineJson, line,
                      LineFilter{}, true);
      if (!m_result)
        return;

      m_pathBuilder.leave();
      ++lineIndex;
    }

    m_pathBuilder.leave();  // "lines"
    m_pathBuilder.leave();  // zone index
  }

  m_pathBuilder.leave();  // "zones"
}

void JsonSerializer::serializeScheduleDefinition(JsonVariant json, Config::Schedule& schedule) {

  // Inverse of deserializeScheduleDefinition(): format the schedule's
  // definition fields into the compact string expected by the deserializer.

  char buf[32];

  unsigned char freqChar;
  if (!toChar(schedule.frequency, freqChar))
    return fail(Serialization::Error::InvalidValue,
                "frequency has no known text representation");

  int written = snprintf(
    buf, sizeof(buf),
    Config::kScheduleDefinition,
    static_cast<int32_t>(schedule.hour),
    static_cast<int32_t>(schedule.minute),
    static_cast<int32_t>(schedule.duration),
    schedule.onlyIfDrySoil ? 1 : 0,
    freqChar,
    static_cast<uint32_t>(schedule.days.mask()));

  if (written < 0 || static_cast<size_t>(written) >= sizeof(buf))
    return fail(Serialization::Error::CapacityExceeded,
                "definition string truncated");

  writeAs<const char*>(json["definition"].to<JsonVariant>(), buf);
}

void JsonSerializer::serializeSchedules(Config::ScheduleCollection& schedules) {

  JsonArray schedulesJson = jsonFor("schedules").to<JsonArray>();

  for (size_t i = 0; i < schedules.size(); ++i) {
    m_pathBuilder.index(i);

    JsonVariant scheduleJson = schedulesJson.add<JsonVariant>();

    serializeObject(scheduleJson, schedules[i],
                    ScheduleFilter{}, true);
    if (!m_result)
      return;

    serializeScheduleDefinition(scheduleJson, schedules[i]);
    if (!m_result)
      return;

    m_pathBuilder.leave();
  }
}

void JsonSerializer::serialize(JsonVariant json, const WeekDays& value) {

  writeAs<uint32_t>(json, value.mask());
}

void JsonSerializer::serialize(JsonVariant json, const Frequency& value) {

  unsigned char c;

  if (!toChar(value, c))
    return fail(Serialization::Error::InvalidValue,
                "frequency has no known text representation");

  writeAs<unsigned char>(json, c);
}

void JsonSerializer::serialize(JsonVariant json, const UUID& value) {

  UUID::String buf = value.unparse();
  writeAs<const char*>(json, buf.data());
}

JsonVariant JsonSerializer::jsonFor(std::string_view key) {
  return m_stack.current()[key].to<JsonVariant>();
}
