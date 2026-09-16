#pragma once

#include <cstddef>
#include <cstdint>
#include <cstring>
#include <string_view>
#include <ArduinoJson.h>
#include "Constants.h"
#include "types/FixedString.h"
#include "config/generated/ConfigReflection.h"
#include "config/runtime/JsonDeserializerResult.h"

struct LineFilter {

  template<typename Parent, typename Member>
  constexpr Reflection::VisitDecision operator()(
    const Reflection::Field<Parent, Member>& field) const {

    if constexpr (std::is_same_v<Parent, Config::Line>) {
      return field.name == "zoneId"
               ? Reflection::VisitDecision::Skip
               : Reflection::VisitDecision::Visit;
    }

    return Reflection::VisitDecision::Visit;
  }
};

struct ScheduleFilter {

  bool inverted = false;

  template<typename Parent, typename Member>
  constexpr Reflection::VisitDecision operator()(
    const Reflection::Field<Parent, Member>& field) const {

    if constexpr (std::is_same_v<Parent, Config::Schedule>) {
      if (field.name == "enabled")
        return Reflection::VisitDecision::Skip;

      const bool selected =
        field.name == "id"
        || field.name == "lineId"
        || field.name == "name";
      return selected != inverted
               ? Reflection::VisitDecision::Visit
               : Reflection::VisitDecision::Skip;
    }

    return Reflection::VisitDecision::Visit;
  }
};

struct WateringModelOnlyFilter {

  template<typename Parent, typename Member>
  constexpr Reflection::VisitDecision operator()(
    const Reflection::Field<Parent, Member>& field) const {

    if constexpr (std::is_same_v<Parent, Config::UserSettings>) {
      return field.name == "wateringModel"
               ? Reflection::VisitDecision::Visit
               : Reflection::VisitDecision::Skip;
    }

    return Reflection::VisitDecision::Visit;
  }
};

struct SchedulesOnlyFilter {

  template<typename Parent, typename Member>
  constexpr Reflection::VisitDecision operator()(
    const Reflection::Field<Parent, Member>& field) const {

    if constexpr (std::is_same_v<Parent, Config>) {
      return field.name == "schedules"
               ? Reflection::VisitDecision::Visit
               : Reflection::VisitDecision::Skip;
    }

    return Reflection::VisitDecision::Visit;
  }
};

class JsonConstContextStack {
public:
  void push(JsonVariantConst v) {
    if (m_top < m_items.size())
      m_items[m_top++] = v;
  }

  void pop() {
    if (m_top > 0) --m_top;
  }

  uint8_t depth() const { return m_top; }

  JsonVariantConst current() const {
    return m_top > 0 ? m_items[m_top - 1] : JsonVariantConst();
  }

private:
  std::array< JsonVariantConst, SchemaLimits::kMaxDepth>
    m_items{};

  uint8_t m_top = 0;
};

class JsonDeserializer {
public:
  using TraversalResult = Result<Deserialization::Error>;

  enum class SpecialCaseResult : uint8_t {
    Continue = 0,
    Handled
  };

  explicit JsonDeserializer(const JsonDocument& doc) {
    m_stack.push(doc.as<JsonVariantConst>());
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
        {
          JsonVariantConst json = jsonFor(field.name);

          if (!json.isNull()) {

            deserialize(json, value);
            if (!m_result)
              return;

            // Optional default values are semantically absent.
            if (Reflection::isAbsent(field, value))
              break;

            validate(json, field.fieldValidator);
            if (!m_result)
              return;

            break;
          }

          if (field.optional)
            break;

          return failMissing(Reflection::fieldType<Member>());
        }
    }

    m_pathBuilder.leave();
  }

  template<typename Parent, typename Member>
  void leave(const Reflection::Field<Parent, Member>&, Member&) {

    m_pathBuilder.leave();
    m_stack.pop();
  }

  template<typename Type>
  void schema(Type& value) {

    const auto* crossAction =
      Reflection::Schema<Type>::crossAction;

    if (!crossAction)
      return;

    if (Validation::Result result = crossAction->cross(value); !result)
      fail(Deserialization::Error::InvalidValue, result);
  }

  void finalize() {
    m_result.setPath(m_pathBuilder.view());
  }

private:
  // JSON validation

  void validate(JsonVariantConst json, const Validation::FieldValidator* fieldValidator) {

    if (!fieldValidator)
      return;

    if (Validation::Result result = fieldValidator->validate(json); !result)
      fail(Deserialization::Error::InvalidValue, result);
  }

  // Specific mapping

  template<typename Filter, typename T>
  void deserializeObject(JsonVariantConst json, T& object,
                         const Filter& filter, bool skipSchema) {

    m_stack.push(json);
    Reflection::traverse(object, *this, filter, skipSchema);
    m_stack.pop();
  }

  void deserializeWateringModel(Config::UserSettings::WateringModel& model);

  void deserializeScheduleDefinition(JsonVariantConst json, Config::Schedule& schedule);
  void deserializeSchedules(Config::ScheduleCollection& schedules);

  template<typename Member>
  SpecialCaseResult tryHandleSpecialCase(Member& value) {
    using T = Reflection::Unqualified<Member>;

    if constexpr (std::is_same_v<T, Config::UserSettings::WateringModel>) {
      deserializeWateringModel(value);
      return SpecialCaseResult::Handled;
    }
    if constexpr (std::is_same_v<T, Config::ScheduleCollection>) {
      deserializeSchedules(value);
      return SpecialCaseResult::Handled;
    }

    return SpecialCaseResult::Continue;
  }

  // 1:1 mapping

  template<typename T, uint8_t N, typename Filter, typename Callback>
  void deserialize(JsonVariantConst json, Collection<T, N>& value,
                   const Filter& filter, bool skipSchema, Callback callback) {
    using C = Collection<T, N>;

    if (!json.is<JsonArrayConst>())
      return failWrongType(Reflection::FieldType::Collection, json);

    JsonArrayConst jsonArray = json.as<JsonArrayConst>();

    for (size_t i = 0; i < jsonArray.size(); ++i) {

      m_pathBuilder.index(i);

      T item{};

      JsonVariantConst jsonChild = jsonArray[i];

      deserializeObject(jsonChild, item,
                        filter, skipSchema);
      if (!m_result)
        return;

      callback(item, jsonChild, i);
      if (!m_result)
        return;

      auto collecAddRes = value.add(item);
      switch (collecAddRes) {
        case C::AddResult::Ok:
          break;
        case C::AddResult::DuplicateId:
          return fail(Deserialization::Error::DuplicateId,
                      "an element with this ID already exists");
        case C::AddResult::Full:
          return fail(Deserialization::Error::CapacityExceeded,
                      "storage capacity exceeded");
      }

      m_pathBuilder.leave();
    }
  }

  template<typename T, uint8_t N>
  void deserialize(JsonVariantConst json, Collection<T, N>& value) {
    deserialize(json, value,
                Reflection::NoFilter{}, false,
                [](T&, JsonVariantConst, size_t) {});
  }

  template<uint16_t N>
  void deserialize(JsonVariantConst json, FixedString<N>& value) {
    readAs<const char*>(json, Reflection::FieldType::String, value);
  }

  template<size_t N>
  void deserialize(JsonVariantConst json, std::bitset<N>& value) {
    if (!json.is<JsonArrayConst>())
      return failWrongType(Reflection::FieldType::Bitset, json);

    for (JsonVariantConst element : json.as<JsonArrayConst>()) {
      if (!element.is<uint32_t>())
        return failWrongType(Reflection::FieldType::Bitset, json);

      uint32_t elementValue = element.as<uint32_t>();

      if (elementValue < 1 || elementValue > N)
        return fail(Deserialization::Error::InvalidValue,
                    "elements must be between min=1"
                    " and max=%zu, got %u",
                    N, elementValue);

      value.set(elementValue - 1);
    }
  }

  void deserialize(JsonVariantConst json, WeekDays& value);
  void deserialize(JsonVariantConst json, Frequency& value);
  void deserialize(JsonVariantConst json, UUID& value);

  template<typename Type>
  void deserialize(JsonVariantConst json, Type& value) {
    using U = Reflection::Unqualified<Type>;

    if (Reflection::fieldType<U>() == Reflection::FieldType::Bool)
      readAs<bool>(json, Reflection::FieldType::Bool, value);

    else if (Reflection::fieldType<U>() == Reflection::FieldType::Int)
      readAs<int32_t>(json, Reflection::FieldType::Int, value);

    else if (Reflection::fieldType<U>() == Reflection::FieldType::UInt)
      readAs<uint32_t>(json, Reflection::FieldType::UInt, value);

    else if (Reflection::fieldType<U>() == Reflection::FieldType::Float)
      readAs<float>(json, Reflection::FieldType::Float, value);

    else readAs<U>(json, Reflection::FieldType::Unknown, value);
  }

  JsonVariantConst jsonFor(std::string_view key);

  template<typename JsonType, typename Type>
  void readAs(JsonVariantConst json, Reflection::FieldType fieldType, Type& value) {

    if (!json.is<JsonType>())
      return failWrongType(fieldType, json);

    value = static_cast<Type>(json.as<JsonType>());
  }

  const char* expectedJsonTypeName(Reflection::FieldType type);
  const char* jsonTypeName(JsonVariantConst json);

  void failMissing(Reflection::FieldType expectedType) {

    m_result = TraversalResult(
      Deserialization::Error::Missing,
      "missing key, expected %s",
      expectedJsonTypeName(expectedType));
  }

  void failWrongType(Reflection::FieldType expectedType,
                     JsonVariantConst      actual) {

    m_result = TraversalResult(
      Deserialization::Error::WrongType,
      "wrong type, expected %s, got %s",
      expectedJsonTypeName(expectedType),
      jsonTypeName(actual));
  }

  void fail(Deserialization::Error error, Validation::Result result) {

    m_result = TraversalResult(
      error, result.message());
  }

  template<typename... Args>
  void fail(Deserialization::Error error, const char* messageFormat, Args... args) {

    m_result = TraversalResult(
      error, messageFormat, args...);
  }

  JsonConstContextStack   m_stack;
  Reflection::PathBuilder m_pathBuilder;
  TraversalResult         m_result;
};
