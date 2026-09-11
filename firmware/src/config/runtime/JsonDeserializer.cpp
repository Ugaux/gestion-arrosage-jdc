#include "JsonDeserializer.h"

JsonDeserializer::TraversalResult JsonDeserializer::deserializeWateringModel(
  Config::UserSettings::WateringModel& model) {

  const auto zoneArrayKey = "zones";
  m_pathBuilder.enter(zoneArrayKey);

  JsonVariantConst zoneArrayJson =
    jsonFor(zoneArrayKey);

  bool skipSchema = false;

  if (TraversalResult result = deserialize(
        zoneArrayJson, model.zones,
        Reflection::NoFilter{}, skipSchema,
        [&](Config::Zone& zone, JsonVariantConst zoneJson, size_t) -> TraversalResult {
          // Because UUID is optional, it is created if it is not present
          if (zone.id.isDefault())
            zone.id = UUID::generate();

          const auto lineArrayKey = "lines";
          m_pathBuilder.enter(lineArrayKey);

          JsonVariantConst lineArrayJson =
            zoneJson[lineArrayKey];

          if (TraversalResult result = deserialize(
                lineArrayJson, model.lines,
                LineJsonFilter{}, skipSchema,
                [&](Config::Line& line, JsonVariantConst, size_t) -> TraversalResult {
                  // Because UUID is optional, create it if it is not present.
                  if (line.id.isDefault())
                    line.id = UUID::generate();

                  line.zoneId = zone.id;

                  return {};
                });
              !result)
            return result;

          m_pathBuilder.leave();

          return {};
        });
      !result)
    return result;

  m_pathBuilder.leave();

  return schema(model);
}

JsonDeserializer::TraversalResult JsonDeserializer::deserializeScheduleDefinition(
  JsonVariantConst json, Config::Schedule& schedule) {

  // The other Schedule members come from "definition".
  JsonVariantConst rawDef = json["definition"];

  if (!rawDef.is<const char*>())
    return failWrongType(Reflection::FieldType::String, rawDef);

  int32_t       hour, minute;
  int32_t       duration;
  int32_t       onlyIfDrySoil;
  unsigned char frequency;
  uint32_t      days;

  int n = sscanf(
    rawDef,
    "%d:%d,%d,%d,%c-%u",
    &hour, &minute,
    &duration,
    &onlyIfDrySoil,
    &frequency,
    &days);

  JsonDocument convertedDef;
  if (n >= 1) convertedDef["hour"] = hour;
  if (n >= 2) convertedDef["minute"] = minute;
  if (n >= 3) convertedDef["duration"] = duration;
  if (n >= 4) convertedDef["onlyIfDrySoil"] = onlyIfDrySoil != 0;
  if (n >= 5) convertedDef["frequency"] = frequency;
  if (n >= 6) convertedDef["days"] = days;

  bool skipSchema = false;

  return deserializeObject(
    convertedDef, schedule,
    ScheduleJsonFilter{ .inverted = true }, skipSchema);
}

JsonDeserializer::TraversalResult JsonDeserializer::deserializeSchedules(
  Config::ScheduleCollection& schedules) {

  JsonVariantConst schedulesJson = jsonFor("schedules");

  bool skipSchema = true;

  return deserialize(
    schedulesJson,
    schedules,
    ScheduleJsonFilter{},
    skipSchema,
    [&](Config::Schedule& schedule, JsonVariantConst scheduleJson, size_t) {
      return deserializeScheduleDefinition(scheduleJson, schedule);
    });
}

JsonDeserializer::TraversalResult JsonDeserializer::deserialize(
  JsonVariantConst json, WeekDays& value) {

  if (!json.is<uint32_t>())
    return failWrongType(Reflection::FieldType::UInt, json);

  uint32_t mask = json.as<uint32_t>();

  if (!value.set(mask))
    return fail(Deserialization::Error::InvalidValue,
                "bitmask must be <= %u, got %u",
                value.kAllDaysMask, mask);

  return {};
}

JsonDeserializer::TraversalResult JsonDeserializer::deserialize(
  JsonVariantConst json, Frequency& value) {

  if (!json.is<unsigned char>())
    return failWrongType(Reflection::FieldType::Frequency, json);

  unsigned char c = json.as<unsigned char>();

  switch (c) {
    case '*': value = Frequency::EveryDay; break;
    case 'e': value = Frequency::EvenDays; break;
    case 'o': value = Frequency::OddDays; break;
    case 's': value = Frequency::SpecificDays; break;
    default:
      return fail(Deserialization::Error::InvalidValue,
                  "must be one of '*', 'e', 'o' or 's', got '%u'",
                  c);
  }

  return {};
}

JsonDeserializer::TraversalResult JsonDeserializer::deserialize(
  JsonVariantConst json, UUID& value) {
  if (!json.is<const char*>())
    return failWrongType(Reflection::FieldType::UUID, json);

  if (!UUID::parse(json.as<const char*>(), value)) {
    return fail(Deserialization::Error::InvalidValue,
                "must be UUID version 4 - variant RFC 4122");
  }

  return {};
}

JsonVariantConst JsonDeserializer::jsonFor(std::string_view key) {
  JsonVariantConst current = m_stack.current();

  if (current.isNull())
    return JsonVariantConst();

  return current[key];
}

const char* JsonDeserializer::expectedJsonTypeName(Reflection::FieldType type) {

  using Type = Reflection::FieldType;

  switch (type) {
    case Type::Bool: return "bool";
    case Type::Weekdays:  // "uint"
    case Type::UInt: return "uint";
    case Type::Int: return "int";
    case Type::Float: return "float";
    case Type::Frequency: return "letter";
    case Type::UUID:  // "text";
    case Type::String: return "text";
    case Type::Bitset: return "array[uint]";
    case Type::Collection: return "array[object]";
    case Type::Unknown: break;
  }

  return "unknown";
}

const char* JsonDeserializer::jsonTypeName(JsonVariantConst json) {

  if (json.isNull()) return "null";
  if (json.is<bool>()) return "bool";
  if (json.is<uint32_t>()) return "uint";
  if (json.is<int32_t>()) return "int";
  if (json.is<float>()) return "float";
  if (json.is<double>()) return "double";
  if (json.is<unsigned char>()) return "letter";
  if (json.is<const char*>()) return "text";
  if (json.is<JsonObjectConst>()) return "object";
  if (json.is<JsonArrayConst>()) return "array[*]";

  return "unknown";
}
