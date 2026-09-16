#pragma once

#include <tuple>
#include <string_view>
#include <type_traits>
#include "Constants.h"
#include "types/UUID.h"
#include "types/Collection.h"
#include "types/FixedString.h"
#include "types/Frequency.h"
#include "types/Weekdays.h"

// forward declaration
namespace Validation {
class FieldValidator;
class CrossAction;
}

namespace Reflection {

enum class FieldType : uint8_t {
  Bool = 0,
  UInt,
  Int,
  Float,
  String,
  UUID,
  Bitset,
  Weekdays,
  Frequency,
  Collection,
  Unknown
};

template<typename T>
using Unqualified = std::remove_cv_t<std::remove_reference_t<T>>;

template<typename T>
struct is_collection : std::false_type {};
template<typename T, uint8_t N>
struct is_collection<Collection<T, N>> : std::true_type {};
template<typename T>
inline constexpr bool is_collection_v =
  is_collection<Unqualified<T>>::value;

template<typename Type>
struct is_fixedstring : std::false_type {};
template<uint16_t N>
struct is_fixedstring<FixedString<N>> : std::true_type {};
template<typename T>
inline constexpr bool is_fixedstring_v =
  is_fixedstring<Unqualified<T>>::value;

template<typename Type>
struct is_frequency : std::false_type {};
template<>
struct is_frequency<Frequency> : std::true_type {};
template<typename T>
inline constexpr bool is_frequency_v =
  is_frequency<Unqualified<T>>::value;

template<typename Type>
struct is_weekdays : std::false_type {};
template<>
struct is_weekdays<WeekDays> : std::true_type {};
template<typename T>
inline constexpr bool is_weekdays_v =
  is_weekdays<Unqualified<T>>::value;

template<typename Type>
struct is_uuid : std::false_type {};
template<>
struct is_uuid<UUID> : std::true_type {};
template<typename T>
inline constexpr bool is_uuid_v =
  is_uuid<Unqualified<T>>::value;

template<typename T>
struct is_bitset : std::false_type {};
template<size_t N>
struct is_bitset<std::bitset<N>> : std::true_type {};
template<typename T>
inline constexpr bool is_bitset_v =
  is_bitset<Unqualified<T>>::value;

template<typename Type>
constexpr FieldType fieldType() {
  using U = Unqualified<Type>;

  if constexpr (std::is_same_v<U, bool>)
    return FieldType::Bool;
  if constexpr (std::is_integral_v<U> && std::is_unsigned_v<U>)
    return FieldType::UInt;
  if constexpr (std::is_integral_v<U> && std::is_signed_v<U>)
    return FieldType::Int;
  if constexpr (std::is_floating_point_v<U>)
    return FieldType::Float;

  if constexpr (is_fixedstring_v<U>)
    return FieldType::String;
  if constexpr (is_uuid_v<U>)
    return FieldType::UUID;
  if constexpr (is_bitset_v<U>)
    return FieldType::Bitset;
  if constexpr (is_weekdays_v<U>)
    return FieldType::Weekdays;
  if constexpr (is_frequency_v<U>)
    return FieldType::Frequency;
  if constexpr (is_collection_v<U>)
    return FieldType::Collection;

  return FieldType::Unknown;
}

template<typename Type>
struct Schema {
  static constexpr bool reflected = false;
  static constexpr auto fields    = std::tuple{};
  static constexpr const Validation::CrossAction*
    crossAction = nullptr;
};

template<typename Parent, typename Member>
struct Field {
  Member Parent::*                  member;
  std::string_view                  name;
  const Validation::FieldValidator* fieldValidator = nullptr;
  std::string_view                  unit           = {};
  bool                              optional       = false;
};

template<typename Parent, typename Member>
constexpr Field<Parent, Member> makeField(
  Member Parent::*                  member,
  std::string_view                  name,
  const Validation::FieldValidator* fieldValidator,
  std::string_view                  unit     = {},
  bool                              optional = false) {
  return {
    .member         = member,
    .name           = name,
    .fieldValidator = fieldValidator,
    .unit           = unit,
    .optional       = optional,
  };
}

template<typename T>
constexpr bool isDefaultValue(const T& value) {

  using U = Unqualified<T>;

  if constexpr (
    is_collection_v<U>
    || is_fixedstring_v<U>)
    return value.size() == 0;

  else
    return value == T{};
}

template<typename Parent, typename Member>
constexpr bool isAbsent(
  const Field<Parent, Member>& field,
  const Member&                value) {

  return field.optional && isDefaultValue(value);
}

enum class VisitDecision : uint8_t {
  Skip = 0,
  Visit
};

struct NoFilter {

  template<typename Parent, typename Member>
  constexpr VisitDecision operator()(
    const Field<Parent, Member>&) const {
    return VisitDecision::Visit;  // don't skip
  }
};

// Forward declaration
template<typename Type, typename Visitor, typename Filter>
void traverse(
  Type& object, Visitor& visitor, const Filter& filter, bool skipSchema);

namespace Detail {

template<typename Parent, typename Member, typename Visitor, typename Filter>
void visitFieldImpl(
  const Field<Parent, Member>& field, Member& value,
  Visitor& visitor, const Filter& filter, std::true_type) {

  if (visitor.enter(field, value) != VisitDecision::Visit)
    return;

  traverse(value, visitor, filter, false);

  if (!visitor.result())
    return;

  visitor.leave(field, value);
}

template<typename Parent, typename Member, typename Visitor, typename Filter>
void visitFieldImpl(
  const Field<Parent, Member>& field, Member& value,
  Visitor& visitor, const Filter&, std::false_type) {

  visitor.field(field, value);
}

template<typename Parent, typename Member, typename Visitor, typename Filter>
void visitField(
  Parent& object, const Field<Parent, Member>& field,
  Visitor& visitor, const Filter& filter) {

  if (filter(field) == VisitDecision::Skip)
    return;

  Member& value = object.*(field.member);

  visitFieldImpl(
    field, value, visitor, filter,
    std::bool_constant<Schema<Member>::reflected>{});
}

}  // namespace Detail

// `traverse()` continues a traversal without finalizing the result.
// See the `visit()` overloads for the full visitor, filter,
// and `skipSchema` requirements.
template<typename Type, typename Visitor, typename Filter>
void traverse(
  Type& object, Visitor& visitor, const Filter& filter, bool skipSchema) {

  std::apply(
    [&](auto const&... field) {
      (
        [&] {
          // Check before visiting the field so that, once an error occurs, no
          // subsequent fields are processed. The return below only exits the
          // lambda for the current field; it does not stop the surrounding fold
          // expression from invoking the remaining fields.
          if (!visitor.result())
            return;

          Detail::visitField(
            object,
            field,
            visitor,
            filter);
        }(),
        ...);
    },
    Schema<Type>::fields);

  if (!visitor.result())
    return;

  if (skipSchema)
    return;

  visitor.schema(object);
}

// `traverse()` overload using the default `NoFilter`.
template<typename Type, typename Visitor>
void traverse(
  Type& object, Visitor& visitor) {

  traverse(object, visitor, NoFilter{}, false);
}

// See the `visit()` overload for the traversal explanation and visitor requirements.
//
// #### Filter
//
// The filter must provide:
//
// ```cpp
//  template<typename Parent, typename Member>
//  constexpr Reflection::VisitDecision operator()(
//   const Reflection::Field<Parent, Member>& field) const;
// ```
//
// Returning `VisitDecision::Skip` skips the field and its value.
template<typename Type, typename Visitor, typename Filter>
void visit(
  Type& object, Visitor& visitor, const Filter& filter, bool skipSchema) {

  traverse(object, visitor, filter, skipSchema);

  visitor.finalize();
}

// #### Visitor
//
// Any type passed as the Visitor argument must provide:
//
// ```cpp
// const auto& result() const;
//
// template<typename Parent, typename Member>
// Reflection::VisitDecision enter(
//   const Reflection::Field<Parent, Member>& field,
//   Member& value);
//
// template<typename Parent, typename Member>
// void field(
//   const Reflection::Field<Parent, Member>& field,
//   Member& value);
//
// template<typename Parent, typename Member>
// void leave(
//   const Reflection::Field<Parent, Member>& field,
//   Member& value);
//
// template<typename Type>
// void schema(Type& value);
//
// void finalize();
// ```
//
// `result()` must return the visitor-owned result, which must provide an
// `operator bool()` for checking whether traversal is still successful.
//
// `schema()` is called after all fields of the current object have been
// successfully traversed, unless `skipSchema` is true.
//
// `finalize()` is called at the end of the complete traversal. This allows
// the visitor to perform final processing that depends on the complete
// traversal, such as saving the final path.
//
// #### Traversal
//
// `visit()` starts a complete traversal and finalizes the result exactly once.
// It is the intended entry point for starting a new traversal.
//
// `traverse()` continues a traversal without finalizing the result.
// The filter is propagated to recursive/nested traversal.
//
// `skipSchema` applies only to the object currently being traversed. It does
// not propagate to nested objects. This allows a caller to skip the schema
// callback for a specific object while still processing schemas of its
// descendants.
//
// This distinction keeps traversal type-agnostic: the traversal engine only
// knows about reflected fields and the Visitor interface, while visitors may
// recursively traverse domain-specific types such as collections.
//
// When traversal stops with an error, the visitor may leave its traversal state
// (such as a path) at the point of failure.
template<typename Type, typename Visitor>
void visit(Type& object, Visitor& visitor) {

  visit(object, visitor, NoFilter{}, false);
}

// A convenient helper that generates a generic C++ path like `watering.manual.duration.step`.
//
// PathBuilder should be used by a visitor alongside its corresponding visitor method,
// with the path-building method called at the beginning of the visitor method.
//
// Example:
// ```cpp
// bool ExampleVisitor::enter(
//   const Reflection::Field<Parent, Member>& field,
//   Member& value) {
//   m_pathBuilder.enter(field.name);
//   ...
// };
// ```
class PathBuilder {
public:
  void enter(std::string_view fieldName) {
    if (m_depth < SchemaLimits::kMaxDepth)
      m_components[m_depth++] = { fieldName, false, 0 };
  }

  void index(size_t idx) {
    if (m_depth < SchemaLimits::kMaxDepth)
      m_components[m_depth++] = { {}, true, idx };
  }

  void leave() {
    if (m_depth > 0) --m_depth;
  }

  // Only ever called once, in finalize() - safe to do real work here.
  std::string_view view() {
    m_pathLen = 0;
    for (uint8_t i = 0; i < m_depth; ++i) {
      const auto& c = m_components[i];
      if (c.isIndex) {
        appendChar('[');
        appendUInt(c.index);
        appendChar(']');
      } else {
        if (i > 0) appendChar('.');
        appendStr(c.name);
      }
    }
    return { m_path.data(), m_pathLen };
  }

private:
  struct Component {
    std::string_view name;
    bool             isIndex = false;
    size_t           index   = 0;
  };

  void appendChar(char c) {
    if (m_pathLen < m_path.size() - 1) m_path[m_pathLen++] = c;
  }

  void appendStr(std::string_view s) {
    size_t n = std::min(s.size(), m_path.size() - 1 - m_pathLen);
    memcpy(m_path.data() + m_pathLen, s.data(), n);
    m_pathLen += n;
  }

  void appendUInt(size_t v) {
    char buf[20];
    int  n = 0;
    if (v == 0) buf[n++] = '0';
    while (v > 0 && n < (int)sizeof(buf)) {
      buf[n++] = char('0' + v % 10);
      v /= 10;
    }
    while (n > 0) appendChar(buf[--n]);
  }

  std::array<char, SchemaLimits::kMaxPathLength> m_path    = {};
  size_t                                         m_pathLen = 0;

  std::array<Component, SchemaLimits::kMaxDepth> m_components = {};
  uint8_t                                        m_depth      = 0;
};

}  // namespace Reflection
