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

struct LineJsonFilter {

  template<typename Parent, typename Member>
  constexpr Reflection::FilterDecision operator()(
    const Reflection::Field<Parent, Member>& field) const {

    if constexpr (std::is_same_v<Parent, Config::Line>) {
      return field.name == "zoneId"
               ? Reflection::FilterDecision::Skip
               : Reflection::FilterDecision::Visit;
    }

    return Reflection::FilterDecision::Visit;
  }
};

struct ScheduleJsonFilter {

  bool inverted = false;

  template<typename Parent, typename Member>
  constexpr Reflection::FilterDecision operator()(
    const Reflection::Field<Parent, Member>& field) const {

    if constexpr (std::is_same_v<Parent, Config::Schedule>) {
      const bool selected =
        field.name == "id"
        || field.name == "lineId"
        || field.name == "enabled"
        || field.name == "name";
      return selected != inverted
               ? Reflection::FilterDecision::Visit
               : Reflection::FilterDecision::Skip;
    }

    return Reflection::FilterDecision::Visit;
  }
};

class JsonContextStack {
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
  std::array<
    JsonVariantConst,
    SchemaLimits::kMaxDepth>
          m_items{};
  uint8_t m_top = 0;
};

class JsonDeserializer {
public:
  // required by reflection for traversal
  using TraversalResult = Result<Deserialization::Error>;
  using VisitResult     = Reflection::VisitResult<TraversalResult>;

  explicit JsonDeserializer(const JsonDocument& doc) {
    m_stack.push(doc.as<JsonVariantConst>());
  }

  template<typename Parent, typename Member>
  VisitResult enter(const Reflection::Field<Parent, Member>& field, Member& value) {

    VisitResult visitResult = tryHandleSpecialCase(value);
    if (visitResult.decision != Reflection::VisitDecision::Traverse)
      return visitResult;

    JsonVariantConst json = jsonFor(field.name);

    m_stack.push(json);
    m_pathBuilder.enter(field.name);

    return VisitResult::traverse();
  }

  template<typename Parent, typename Member>
  TraversalResult field(const Reflection::Field<Parent, Member>& field, Member& value) {

    m_pathBuilder.enter(field.name);

    VisitResult visitResult = tryHandleSpecialCase(value);
    switch (visitResult.decision) {

      case Reflection::VisitDecision::Traverse:
        {
          JsonVariantConst json = jsonFor(field.name);

          if (!json.isNull()) {

            if (Result result = deserialize(json, value); !result)
              return result;

            if (Result result = validate(json, field.fieldValidator); !result)
              return result;

            break;
          }

          if (field.optional)
            break;

          return failMissing(Reflection::fieldType<Member>());
        }

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

  template<typename Type>
  TraversalResult schema(Type& value) {

    const auto* crossAction =
      Reflection::Schema<Type>::crossAction;

    if (!crossAction)
      return {};

    if (Validation::Result result = crossAction->cross(value); !result)
      return fail(Deserialization::Error::InvalidValue, result);

    return {};
  }

  TraversalResult finalize(TraversalResult result) {
    return result.withPath(m_pathBuilder.view());
  }

private:
  // JSON validation

  TraversalResult validate(JsonVariantConst json, const Validation::FieldValidator* fieldValidator) {

    if (!fieldValidator)
      return {};

    if (Validation::Result result = fieldValidator->validate(json); !result)
      return fail(Deserialization::Error::InvalidValue, result);

    return {};
  }

  // Specific mapping

  template<typename Filter, typename T>
  TraversalResult deserializeObject(JsonVariantConst json, T& object,
                                    const Filter& filter, bool skipSchema) {

    m_stack.push(json);
    TraversalResult result = Reflection::traverse(
      object, *this, filter, skipSchema);
    m_stack.pop();

    return result;
  }

  TraversalResult deserializeWateringModel(Config::UserSettings::WateringModel& model);

  TraversalResult deserializeScheduleDefinition(JsonVariantConst json, Config::Schedule& schedule);
  TraversalResult deserializeSchedules(Config::ScheduleCollection& schedules);

  template<typename Member>
  VisitResult tryHandleSpecialCase(Member& value) {
    using T = Reflection::Unqualified<Member>;

    if constexpr (std::is_same_v<T, Config::UserSettings::WateringModel>) {
      if (Result result = deserializeWateringModel(value); !result)
        return VisitResult::error(result);
      return VisitResult::handled();
    }
    if constexpr (std::is_same_v<T, Config::ScheduleCollection>) {
      if (Result result = deserializeSchedules(value); !result)
        return VisitResult::error(result);
      return VisitResult::handled();
    }

    return VisitResult::traverse();
  }

  // 1:1 mapping

  template<typename T, uint8_t N, typename Filter, typename Callback>
  TraversalResult deserialize(JsonVariantConst json, Collection<T, N>& value,
                              const Filter& filter, bool skipSchema, Callback callback) {
    using C = Collection<T, N>;

    if (!json.is<JsonArrayConst>())
      return failWrongType(Reflection::FieldType::Collection, json);

    JsonArrayConst jsonArray = json.as<JsonArrayConst>();

    for (size_t i = 0; i < jsonArray.size(); ++i) {
      m_pathBuilder.index(i);

      T item{};

      JsonVariantConst jsonChild = jsonArray[i];

      if (Result result = deserializeObject(jsonChild, item, filter, skipSchema); !result)
        return result;

      if (Result result = callback(item, jsonChild, i); !result)
        return result;

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

    return {};
  }

  template<typename T, uint8_t N>
  TraversalResult deserialize(JsonVariantConst json, Collection<T, N>& value) {
    return deserialize(
      json, value,
      Reflection::NoFilter{}, false,
      [](T&, JsonVariantConst, size_t) -> TraversalResult {
        return {};
      });
  }

  template<uint16_t N>
  TraversalResult deserialize(JsonVariantConst json, FixedString<N>& value) {
    if (!json.is<const char*>())
      return failWrongType(Reflection::FieldType::String, json);

    value = json.as<const char*>();
    return {};
  }

  template<size_t N>
  TraversalResult deserialize(JsonVariantConst json, std::bitset<N>& value) {
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

    return {};
  }

  TraversalResult deserialize(JsonVariantConst json, WeekDays& value);
  TraversalResult deserialize(JsonVariantConst json, Frequency& value);
  TraversalResult deserialize(JsonVariantConst json, UUID& value);

  template<typename Type>
  TraversalResult deserialize(JsonVariantConst json, Type& value) {
    using U = Reflection::Unqualified<Type>;

    if (Reflection::fieldType<U>() == Reflection::FieldType::Bool) {
      if (!json.is<bool>())
        return failWrongType(Reflection::FieldType::Bool, json);

      value = json.as<bool>();
    }

    else if (Reflection::fieldType<U>() == Reflection::FieldType::Int) {
      if (!json.is<int32_t>())
        return failWrongType(Reflection::FieldType::Int, json);

      value = static_cast<U>(json.as<int32_t>());
    }

    else if (Reflection::fieldType<U>() == Reflection::FieldType::UInt) {
      if (!json.is<uint32_t>())
        return failWrongType(Reflection::FieldType::UInt, json);

      value = static_cast<U>(json.as<uint32_t>());
    }

    else if (Reflection::fieldType<U>() == Reflection::FieldType::Float) {
      if (!json.is<float>())
        return failWrongType(Reflection::FieldType::Float, json);

      value = static_cast<U>(json.as<float>());
    }

    else {
      if (!json.is<U>())
        return failWrongType(Reflection::FieldType::Unknown, json);

      value = json.as<U>();
    }

    return {};
  }

  JsonVariantConst jsonFor(std::string_view key);

  const char* expectedJsonTypeName(Reflection::FieldType type);
  const char* jsonTypeName(JsonVariantConst json);

  TraversalResult failMissing(Reflection::FieldType expectedType) {

    return TraversalResult(Deserialization::Error::Missing,
                           "missing key, expected %s",
                           expectedJsonTypeName(expectedType));
  }

  TraversalResult failWrongType(Reflection::FieldType expectedType,
                                JsonVariantConst      actual) {

    return TraversalResult(Deserialization::Error::WrongType,
                           "wrong type, expected %s, got %s",
                           expectedJsonTypeName(expectedType),
                           jsonTypeName(actual));
  }

  TraversalResult fail(Deserialization::Error error, Validation::Result result) {
    return TraversalResult(error, result.message());
  }

  template<typename... Args>
  TraversalResult fail(Deserialization::Error error, const char* messageFormat, Args... args) {
    return TraversalResult(error, messageFormat, args...);
  }

  JsonContextStack        m_stack;
  Reflection::PathBuilder m_pathBuilder;
};
