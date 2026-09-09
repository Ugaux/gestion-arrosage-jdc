#pragma once

#include <cstring>
#include <cstdint>
#include <Arduino.h>
#include <ArduinoJson.h>
#include "config/generated/Config.h"
#include "config/runtime/Reflection.h"

// validators + validation traversal
namespace Validation {

class Result;

namespace CrossFn {

Result validatePumpFlow(Config::UserSettings::Params::Watering::Pump::Flow& cfg);
Result validateDuration(Config::UserSettings::Params::Watering::Duration& cfg);

Result validateLine(Config::UserSettings::WateringModel& cfg);

Result validateSchedule(Config::Schedule& cfg);
// Filters schedules that point to non-existing lines
Result normalizeAndValidateSchedules(Config& cfg);

}  // namespace CrossFn

class Result {
public:
  static constexpr size_t kMaxErrorLength = 120;

  enum class Error : uint8_t {
    MissingAdapterFunction = 0,
    UnexpectedValueType,

    Duplicate,
    ValueOutOfRange,
    LengthOutOfRange,
    InvalidReference,
    OperationFailed,
    UnsupportedValue,
  };

  Result() = default;

  explicit Result(Error e) {
    set(e, "");
  }

  template<typename... Args>
  Result(Error e, const char* fmt, Args... args) {
    set(e, fmt, args...);
  }

  bool ok() const {
    return m_ok;
  }

  explicit operator bool() const {
    return ok();
  }

  Error error() const {
    return m_error;
  }

  std::string_view message() const {
    return { m_msg.data(), m_msg_len };
  }

private:
  static const char* message(Error e) {
    switch (e) {

      case Error::Duplicate:
        return "duplicate";

      case Error::MissingAdapterFunction:
        return "missing validator adapter function";

      case Error::UnexpectedValueType:
        return "unexpected value type";

      case Error::ValueOutOfRange:
        return "value out of range";

      case Error::LengthOutOfRange:
        return "length out of range";

      case Error::InvalidReference:
        return "invalid reference";

      case Error::OperationFailed:
        return "operation failed";

      case Error::UnsupportedValue:
        return "unsupported value";
    }

    return "unknown error";
  }

  template<typename... Args>
  void set(Error err, const char* fmt, Args... args) {
    m_ok    = false;
    m_error = err;

    const int written = snprintf(
      m_msg.data(),
      m_msg.size(),
      "%s%s",
      message(err), fmt[0] == '\0' ? "" : ": ");

    if (written < 0) {
      m_msg_len = 0;
      return;
    }

    m_msg_len = std::min(
      static_cast<size_t>(written),
      m_msg.size() - 1);

    const int detail = snprintf(
      m_msg.data() + m_msg_len,
      m_msg.size() - m_msg_len,
      fmt,
      args...);

    if (detail < 0)
      return;

    const size_t detailSize = static_cast<size_t>(detail);

    if (detailSize < m_msg.size() - m_msg_len) {
      m_msg_len += detailSize;
      return;
    }

    const size_t truncated =
      detailSize - (m_msg.size() - m_msg_len - 1);

    const size_t suffixLen =
      static_cast<size_t>(
        snprintf(
          nullptr,
          0,
          " (%zu)",
          truncated));

    // Keep as much of the existing message as possible.
    const size_t keep =
      m_msg.size() - 1 - suffixLen;

    m_msg[keep] = '\0';

    snprintf(
      m_msg.data() + keep,
      m_msg.size() - keep,
      " (%zu)",
      truncated);

    m_msg_len = keep + suffixLen;
  }

  Error m_error{};

  bool m_ok = true;

  std::array<char, kMaxErrorLength> m_msg = {};

  size_t m_msg_len = 0;
};

namespace Detail {

template<typename Self>
class CrossActionBase {
public:
  using ApplyFn =
    Result (*)(void*);

  constexpr explicit CrossActionBase(ApplyFn fn)
    : m_apply(fn) {}

  template<typename T>
  Result cross(T& value) const {

    if (!m_apply)
      return Result(Result::Error::MissingAdapterFunction);

    return m_apply(&value);
  }

private:
  ApplyFn m_apply = nullptr;
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
    Range = 0,
    Length,
  };

  struct RangeData {
    int32_t min, max;
  };

  struct LengthData {
    uint16_t min, max;
  };

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
      return Result(Result::Error::MissingAdapterFunction);

    return m_valueApply(*this, &value);
  }

  // JSON:
  // No JSON adapter is required in generated reflection.
  // The validator knows how its own rule maps from JSON.
  Result validate(JsonVariantConst json) const {

    switch (m_type) {

      case Type::Range:
        {
          if (!json.is<int32_t>())
            return Result(Result::Error::UnexpectedValueType);

          const int32_t value = json.as<int32_t>();
          return validateRange(value);
        }

      case Type::Length:
        {
          if (!json.is<const char*>())
            return Result(Result::Error::UnexpectedValueType);

          const char* value = json.as<const char*>();
          return validateLength(value);
        }
    }

    return {};
  }

  template<size_t N>
  Result validateRange(const std::bitset<N>& value) const {
    return validateRange(static_cast<int32_t>(value.count()));
  }

  Result validateRange(const int32_t& value) const {
    if (value >= range.min && value <= range.max)
      return {};

    return Result(
      Result::Error::ValueOutOfRange,
      "must be %d..%d, got %d",
      range.min, range.max, value);
  }

  template<typename T>
  Result validateLength(const T& value) const {
    const size_t len = valueLength(value);

    if (len >= length.min && len <= length.max)
      return {};

    return Result(
      Result::Error::LengthOutOfRange,
      "must be %u..%u, got %zu",
      length.min, length.max, len);
  }

  union {
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
  const Validation::Result result() const {
    return m_result;
  }

  std::string_view path() const {
    return m_pathBuilder.view();
  }

  template<typename Parent, typename Member>
  Reflection::VisitResult enter(const Reflection::Field<Parent, Member>& field, Member&) {
    m_pathBuilder.enter(field.name);

    return Reflection::VisitResult::Traverse;
  }

  template<typename Parent, typename Member>
  bool field(const Reflection::Field<Parent, Member>& field, Member& value) {
    m_pathBuilder.enter(field.name);

    if (!field.optional || !isDefaultValue(value)) {
      if (!validate(value, field.fieldValidator))
        return false;
    }

    m_pathBuilder.leave();
    return true;
  }

  template<typename Parent, typename Member>
  bool leave(const Reflection::Field<Parent, Member>&, Member&) {
    m_pathBuilder.leave();
    return true;
  }

  template<typename Type>
  bool schema(Type& value) {

    const auto* action =
      Reflection::Schema<Type>::crossAction;

    if (!action) return true;

    m_result = action->cross(value);
    return static_cast<bool>(m_result);
  }

private:
  template<typename T, uint8_t N>
  bool validate(Collection<T, N>& value, const Validation::FieldValidator* /*fieldValidator*/) {

    for (size_t i = 0; i < value.size(); ++i) {
      m_pathBuilder.index(i);
      if (!Reflection::visit(value[i], *this))
        return false;
      m_pathBuilder.leave();
    }

    return true;
  }

  template<typename T>
  bool validate(const T& value, const Validation::FieldValidator* fieldValidator) {

    if (!fieldValidator)
      return true;

    m_result = fieldValidator->validate(value);
    return static_cast<bool>(m_result);
  }

  template<typename T>
  bool isDefaultValue(const T& value) const {
    if constexpr (Reflection::is_collection_v<Reflection::Unqualified<T>>
                  || Reflection::is_fixedstring_v<Reflection::Unqualified<T>>)
      return value.size() == 0;
    else
      return value == T{};
  }

  Validation::Result m_result;

  Reflection::PathBuilder m_pathBuilder;
};

}  // namespace Validation
