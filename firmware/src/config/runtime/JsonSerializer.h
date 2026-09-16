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
  using TraversalResult = Result<Serialization::Error>;

  enum class SpecialCaseResult : uint8_t {
    Continue = 0,
    Handled
  };

  explicit JsonSerializer(JsonDocument& doc) {
    m_stack.push(doc.to<JsonObject>());
  }

  const auto& result() const {
    return m_result;
  }

  template<typename Parent, typename Member>
  Reflection::VisitDecision enter(const Reflection::Field<Parent, Member>& field, Member& value) {

    switch (tryHandleSpecialCase(value)) {

      case SpecialCaseResult::Handled:
        return Reflection::VisitDecision::Skip;

      case SpecialCaseResult::Continue:
        break;
    }

    m_stack.push(jsonFor(field.name));
    m_pathBuilder.enter(field.name);

    return Reflection::VisitDecision::Visit;
  }

  template<typename Parent, typename Member>
  void field(const Reflection::Field<Parent, Member>& field, Member& value) {

    m_pathBuilder.enter(field.name);

    switch (tryHandleSpecialCase(value)) {

      case SpecialCaseResult::Handled:
        if (!m_result)
          return;
        break;

      case SpecialCaseResult::Continue:

        // Every field's current value is written regardless of field.optional,
        // since JsonDeserializer treats "missing-and-optional" and
        // "present-with-the-default-value" identically on read. This is simpler
        // and still round-trips stably — just occasionally slightly more verbose JSON.
        serialize(jsonFor(field.name), value);
        if (!m_result)
          return;

        break;
    }

    m_pathBuilder.leave();
  }

  template<typename Parent, typename Member>
  void leave(const Reflection::Field<Parent, Member>&, Member&) {

    m_pathBuilder.leave();
    m_stack.pop();
  }

  // No-op: cross-field validation is a read-side concern (it protects
  // against untrusted input). A Config already in memory is assumed valid,
  // so there's nothing to check when writing it back out.
  template<typename Type>
  void schema(Type&) {}

  void finalize() {
    m_result.setPath(m_pathBuilder.view());
  }

private:
  // Specific mapping

  template<typename Filter, typename T>
  void serializeObject(JsonVariant json, T& object,
                       const Filter& filter, bool skipSchema) {

    m_stack.push(json);
    Reflection::traverse(object, *this, filter, skipSchema);
    m_stack.pop();
  }

  void serializeWateringModel(Config::UserSettings::WateringModel& model);

  void serializeScheduleDefinition(JsonVariant json, Config::Schedule& schedule);
  void serializeSchedules(Config::ScheduleCollection& schedules);

  template<typename Member>
  SpecialCaseResult tryHandleSpecialCase(Member& value) {
    using T = Reflection::Unqualified<Member>;

    if constexpr (std::is_same_v<T, Config::UserSettings::WateringModel>) {
      serializeWateringModel(value);
      return SpecialCaseResult::Handled;
    }
    if constexpr (std::is_same_v<T, Config::ScheduleCollection>) {
      serializeSchedules(value);
      return SpecialCaseResult::Handled;
    }

    return SpecialCaseResult::Continue;
  }

  // 1:1 mapping

  template<typename T, uint8_t N>
  void serialize(JsonVariant json, Collection<T, N>& value) {

    JsonArray jsonArray = json.to<JsonArray>();

    for (size_t i = 0; i < value.size(); ++i) {
      m_pathBuilder.index(i);

      JsonVariant itemJson = jsonArray.add<JsonVariant>();

      serializeObject(itemJson, value[i],
                      Reflection::NoFilter{}, true);
      if (!m_result)
        return;

      m_pathBuilder.leave();
    }
  }

  template<uint16_t N>
  void serialize(JsonVariant json, const FixedString<N>& value) {
    writeAs<const char*>(json, value.c_str());
  }

  template<size_t N>
  void serialize(JsonVariant json, const std::bitset<N>& value) {

    JsonArray jsonArray = json.to<JsonArray>();

    for (size_t i = 0; i < N; ++i) {
      if (value.test(i)) {
        if (!jsonArray.add(static_cast<uint32_t>(i + 1)))
          return fail(Serialization::Error::CapacityExceeded,
                      "out of space writing bitset element");
      }
    }
  }

  void serialize(JsonVariant json, const WeekDays& value);
  void serialize(JsonVariant json, const Frequency& value);
  void serialize(JsonVariant json, const UUID& value);

  template<typename Type>
  void serialize(JsonVariant json, const Type& value) {
    using U = Reflection::Unqualified<Type>;

    if (Reflection::fieldType<U>() == Reflection::FieldType::Bool)
      writeAs<bool>(json, value);

    else if (Reflection::fieldType<U>() == Reflection::FieldType::Int)
      writeAs<int32_t>(json, value);

    else if (Reflection::fieldType<U>() == Reflection::FieldType::UInt)
      writeAs<uint32_t>(json, value);

    else if (Reflection::fieldType<U>() == Reflection::FieldType::Float)
      writeAs<float>(json, value);

    else writeAs<U>(json, value);
  }

  JsonVariant jsonFor(std::string_view key);

  template<typename JsonType, typename Type>
  void writeAs(JsonVariant json, const Type& value) {

    if (!json.set(static_cast<JsonType>(value)))
      fail(Serialization::Error::CapacityExceeded,
           "out of space writing value%s",
           json.isNull() ? " (target was unbound/null)" : "");
  }

  template<typename... Args>
  void fail(Serialization::Error error, const char* messageFormat, Args... args) {

    m_result = TraversalResult(
      error, messageFormat, args...);
  }

  JsonContextStack        m_stack;
  Reflection::PathBuilder m_pathBuilder;
  TraversalResult         m_result;
};
