#pragma once

#include <cstring>
#include <cstdint>
#include <Arduino.h>
#include <ArduinoJson.h>
#include "config/generated/Config.h"
#include "config/runtime/Reflection.h"
#include "config/runtime/ValidationResult.h"

// validators + validation traversal
namespace Validation {

using Result = ::Result<Error>;

namespace CrossFn {

Result validatePumpFlow(Config::UserSettings::Params::Watering::Pump::Flow& cfg);
Result validateDuration(Config::UserSettings::Params::Watering::Duration& cfg);

Result validateLines(Config::UserSettings::WateringModel& cfg);

Result validateSchedule(Config::Schedule& cfg);
Result normalizeAndValidateSchedules(Config& cfg);

}  // namespace CrossFn

namespace Detail {

template<typename Self>
class CrossActionBase {
public:
  using ApplyFn =
    Result (*)(void*);

  constexpr explicit CrossActionBase(ApplyFn fn)
    : m_crossApply(fn) {}

  template<typename T>
  Result cross(T& value) const {

    if (!m_crossApply)
      return Result(Error::MissingAdapterFunction);

    return m_crossApply(&value);
  }

private:
  ApplyFn m_crossApply = nullptr;
};

}  // namespace Detail

class CrossAction : public Detail::CrossActionBase<CrossAction> {
  using Detail::CrossActionBase<CrossAction>::CrossActionBase;
};

template<typename T, Result (*Fn)(T&)>
Result crossAdapter(void* self) {
  return Fn(*static_cast< T*>(self));
}

class FieldValidator {
public:
  // Type-erased C++ value adapter
  using ValueApplyFn = Result (*)(const FieldValidator&, const void*);

  enum class Type : uint8_t {
    Count = 0,
    Range,
    Length,
  };

  struct CountData {
    uint32_t min, max;
  };

  struct RangeData {
    int32_t min, max;
  };

  struct LengthData {
    uint32_t min, max;
  };

  constexpr FieldValidator(ValueApplyFn valueFn, CountData data)
    : count(data), m_type(Type::Count), m_valueApply(valueFn) {}

  constexpr FieldValidator(ValueApplyFn valueFn, RangeData data)
    : range(data), m_type(Type::Range), m_valueApply(valueFn) {}

  constexpr FieldValidator(ValueApplyFn valueFn, LengthData data)
    : length(data), m_type(Type::Length), m_valueApply(valueFn) {}

  Type type() const { return m_type; }

  explicit operator bool() const {
    return m_valueApply != nullptr;
  }

  // C++ value
  template<typename T>
  Result validate(const T& value) const {

    if (!m_valueApply)
      return Result(Error::MissingAdapterFunction);

    return m_valueApply(*this, &value);
  }

  // JSON:
  // No JSON adapter is required in generated reflection.
  // The validator knows how its own rule maps from JSON.
  Result validate(JsonVariantConst json) const {

    switch (m_type) {

      case Type::Count:
        {
          if (!json.is<JsonArrayConst>())
            return Result(Error::UnexpectedValueType);

          return validateCount(json.as<JsonArrayConst>().size());
        }

      case Type::Range:
        {
          if (!json.is<int32_t>())
            return Result(Error::UnexpectedValueType);

          return validateRange(json.as<int32_t>());
        }

      case Type::Length:
        {
          if (!json.is<const char*>())
            return Result(Error::UnexpectedValueType);

          return validateLength(json.as<const char*>());
        }
    }

    return {};
  }

  Result validateCount(uint32_t value) const {
    if (value >= count.min && value <= count.max)
      return {};

    return Result(
      Error::CountOutOfRange,
      "count must be between min=%u and max=%u, got %u",
      count.min, count.max, value);
  }

  Result validateRange(const int32_t& value) const {
    if (value >= range.min && value <= range.max)
      return {};

    return Result(
      Error::ValueOutOfRange,
      "value must be between min=%d and max=%d, got %d",
      range.min, range.max, value);
  }

  template<typename T>
  Result validateLength(const T& value) const {
    const size_t len = valueLength(value);

    if (len >= length.min && len <= length.max)
      return {};

    return Result(
      Error::LengthOutOfRange,
      "length must be between min=%u and max=%u, got %zu",
      length.min, length.max, len);
  }

  union {
    CountData  count;
    RangeData  range;
    LengthData length;
  };

private:
  static size_t valueLength(const char* value) {
    return value ? std::strlen(value) : 0;
  }

  template<typename T>
  static auto valueLength(const T& value) {
    return value.size();
  }

  Type m_type;

  ValueApplyFn m_valueApply = nullptr;
};

template<typename T>
Result countAdapter(const FieldValidator& validator, const void* self) {

  const T& value = *static_cast<const T*>(self);

  return validator.validateCount(value.count());
}

template<typename T>
Result rangeAdapter(const FieldValidator& validator, const void* self) {

  const T& value = *static_cast<const T*>(self);

  return validator.validateRange(value);
}

template<typename T>
Result lengthAdapter(const FieldValidator& validator, const void* self) {

  const T& value = *static_cast<const T*>(self);

  return validator.validateLength(value);
}

class ValidationVisitor {
public:
  using Result = Validation::Result;  // required by reflection for traversal

  using VisitResult = Reflection::VisitResult<Result>;

  template<typename Parent, typename Member>
  VisitResult enter(const Reflection::Field<Parent, Member>& field, Member&) {

    m_pathBuilder.enter(field.name);
    return VisitResult::traverse();
  }

  template<typename Parent, typename Member>
  Result field(const Reflection::Field<Parent, Member>& field, Member& value) {

    m_pathBuilder.enter(field.name);

    if (!field.optional || !isDefaultValue(value)) {
      if (Result result = validate(value, field.fieldValidator); !result)
        return result;
    }

    m_pathBuilder.leave();
    return {};
  }

  template<typename Parent, typename Member>
  Result leave(const Reflection::Field<Parent, Member>&, Member&) {

    m_pathBuilder.leave();
    return {};
  }

  template<typename Type>
  Result schema(Type& value) {

    const auto* crossAction =
      Reflection::Schema<Type>::crossAction;

    if (!crossAction)
      return {};

    return crossAction->cross(value);
  }

  Result finalize(Result result) {
    return result.withPath(m_pathBuilder.view());
  }

private:
  template<typename T, uint8_t N>
  Result validate(Collection<T, N>& value, const Validation::FieldValidator* /*fieldValidator*/) {

    for (size_t i = 0; i < value.size(); ++i) {
      m_pathBuilder.index(i);
      if (Result result = Reflection::traverse(value[i], *this); !result)
        return result;
      m_pathBuilder.leave();
    }

    return {};
  }

  template<typename T>
  Result validate(const T& value, const Validation::FieldValidator* fieldValidator) {

    if (!fieldValidator)
      return {};

    return fieldValidator->validate(value);
  }

  template<typename T>
  bool isDefaultValue(const T& value) const {

    if constexpr (
      Reflection::is_collection_v<
        Reflection::Unqualified<T>>
      || Reflection::is_fixedstring_v<
        Reflection::Unqualified<T>>)
      return value.size() == 0;

    else  // required for compilation to succeed
      return value == T{};
  }

  Reflection::PathBuilder m_pathBuilder;
};

}  // namespace Validation
