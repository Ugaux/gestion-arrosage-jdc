#pragma once

#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <string_view>
#include <ArduinoJson.h>
#include "Constants.h"
#include "types/FixedString.h"
#include "config/generated/ConfigReflection.h"
#include "config/runtime/JsonSerializerResult.h"

// Mutable counterpart of JsonConstContextStack (JsonDeserializer.h),
// holding JsonVariant instead of JsonVariantConst.
class JsonContextStack {
public:
  void push(JsonVariant v) {
    if (m_top < m_items.size())
      m_items[m_top++] = v;
  }

  void pop() {
    if (m_top > 0) --m_top;
  }

  uint8_t depth() const { return m_top; }

  JsonVariant current() const {
    return m_top > 0 ? m_items[m_top - 1] : JsonVariant();
  }

private:
  std::array< JsonVariant, SchemaLimits::kMaxDepth>
    m_items{};

  uint8_t m_top = 0;
};

class JsonSerializer {
public:
  // required by reflection for traversal
  using TraversalResult = Result<Serialization::Error>;
  using VisitResult     = Reflection::VisitResult<TraversalResult>;

  explicit JsonSerializer(JsonDocument& doc) {
    m_stack.push(doc.to<JsonObject>());
  }

  template<typename Parent, typename Member>
  VisitResult enter(const Reflection::Field<Parent, Member>& field, Member& value) {

    VisitResult visitResult = tryHandleSpecialCase(value);
    if (visitResult.decision != Reflection::VisitDecision::Traverse)
      return visitResult;

    m_stack.push(jsonFor(field.name));
    m_pathBuilder.enter(field.name);

    return VisitResult::traverse();
  }

  template<typename Parent, typename Member>
  TraversalResult field(const Reflection::Field<Parent, Member>& field, Member& value) {

    m_pathBuilder.enter(field.name);

    VisitResult visitResult = tryHandleSpecialCase(value);
    switch (visitResult.decision) {

      case Reflection::VisitDecision::Traverse:

        // Every field's current value is written regardless of field.optional,
        // since JsonDeserializer treats "missing-and-optional" and
        // "present-with-the-default-value" identically on read. This is simpler
        // and still round-trips stably — just occasionally slightly more verbose JSON.
        if (auto result = serialize(jsonFor(field.name), value); !result)
          return result;

        break;

      case Reflection::VisitDecision::Handled:
        break;

      case Reflection::VisitDecision::Error:
        return visitResult.result;
    }

    m_pathBuilder.leave();
    return {};
  }

  template<typename Parent, typename Member>
  TraversalResult leave(const Reflection::Field<Parent, Member>&, Member&) {

    m_pathBuilder.leave();
    m_stack.pop();
    return {};
  }

  // No-op: cross-field validation is a read-side concern (it protects
  // against untrusted input). A Config already in memory is assumed valid,
  // so there's nothing to check when writing it back out.
  template<typename Type>
  TraversalResult schema(Type&) {
    return {};
  }

  TraversalResult finalize(TraversalResult result) {
    return result.withPath(m_pathBuilder.view());
  }

private:
  // Specific mapping

  template<typename Filter, typename T>
  TraversalResult serializeObject(JsonVariant json, T& object,
                                  const Filter& filter, bool skipSchema) {

    m_stack.push(json);
    TraversalResult result = Reflection::traverse(
      object, *this, filter, skipSchema);
    m_stack.pop();

    return result;
  }

  TraversalResult serializeWateringModel(Config::UserSettings::WateringModel& model);

  TraversalResult serializeScheduleDefinition(JsonVariant json, Config::Schedule& schedule);
  TraversalResult serializeSchedules(Config::ScheduleCollection& schedules);

  template<typename Member>
  VisitResult tryHandleSpecialCase(Member& value) {
    using T = Reflection::Unqualified<Member>;

    if constexpr (std::is_same_v<T, Config::UserSettings::WateringModel>) {
      if (auto result = serializeWateringModel(value); !result)
        return VisitResult::error(result);
      return VisitResult::handled();
    }
    if constexpr (std::is_same_v<T, Config::ScheduleCollection>) {
      if (auto result = serializeSchedules(value); !result)
        return VisitResult::error(result);
      return VisitResult::handled();
    }

    return VisitResult::traverse();
  }

  // 1:1 mapping

  template<typename T, uint8_t N>
  TraversalResult serialize(JsonVariant json, Collection<T, N>& value) {

    JsonArray jsonArray = json.to<JsonArray>();

    for (size_t i = 0; i < value.size(); ++i) {
      m_pathBuilder.index(i);

      JsonVariant itemJson = jsonArray.add<JsonVariant>();

      if (auto result = serializeObject(itemJson, value[i], Reflection::NoFilter{}, true); !result)
        return result;

      m_pathBuilder.leave();
    }

    return {};
  }

  template<uint16_t N>
  TraversalResult serialize(JsonVariant json, const FixedString<N>& value) {
    return writeAs<const char*>(json, value.c_str());
  }

  template<size_t N>
  TraversalResult serialize(JsonVariant json, const std::bitset<N>& value) {

    JsonArray jsonArray = json.to<JsonArray>();

    for (size_t i = 0; i < N; ++i) {
      if (value.test(i)) {
        if (!jsonArray.add(static_cast<uint32_t>(i + 1)))
          return fail(Serialization::Error::CapacityExceeded,
                      "out of space writing bitset element");
      }
    }

    return {};
  }

  TraversalResult serialize(JsonVariant json, const WeekDays& value);
  TraversalResult serialize(JsonVariant json, const Frequency& value);
  TraversalResult serialize(JsonVariant json, const UUID& value);

  template<typename Type>
  TraversalResult serialize(JsonVariant json, const Type& value) {
    using U = Reflection::Unqualified<Type>;

    if (Reflection::fieldType<U>() == Reflection::FieldType::Bool)
      return writeAs<bool>(json, value);

    if (Reflection::fieldType<U>() == Reflection::FieldType::Int)
      return writeAs<int32_t>(json, value);

    if (Reflection::fieldType<U>() == Reflection::FieldType::UInt)
      return writeAs<uint32_t>(json, value);

    if (Reflection::fieldType<U>() == Reflection::FieldType::Float)
      return writeAs<float>(json, value);

    return writeAs<U>(json, value);
  }

  JsonVariant jsonFor(std::string_view key);

  template<typename JsonType, typename Type>
  TraversalResult writeAs(JsonVariant json, const Type& value) {

    if (!json.set(static_cast<JsonType>(value)))
      return fail(Serialization::Error::CapacityExceeded,
                  "out of space writing value%s",
                  json.isNull() ? " (target was unbound/null)" : "");

    return {};
  }

  template<typename... Args>
  TraversalResult fail(Serialization::Error error, const char* messageFormat, Args... args) {
    return TraversalResult(error, messageFormat, args...);
  }

  JsonContextStack        m_stack;
  Reflection::PathBuilder m_pathBuilder;
};
