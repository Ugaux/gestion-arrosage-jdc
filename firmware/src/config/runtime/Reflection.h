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

enum class VisitDecision : uint8_t {
  Traverse = 0,
  Handled,
  Error
};

template<typename Result>
struct [[nodiscard]] VisitResult {
  VisitDecision decision;
  Result        result;

  static VisitResult traverse() {
    return { VisitDecision::Traverse, {} };
  }

  static VisitResult handled() {
    return { VisitDecision::Handled, {} };
  }

  static VisitResult error(Result result) {
    return { VisitDecision::Error, std::move(result) };
  }
};

enum class FilterDecision : uint8_t {
  Visit = 0,
  Skip
};

struct NoFilter {

  template<typename Parent, typename Member>
  constexpr FilterDecision operator()(
    const Field<Parent, Member>&) const {
    return FilterDecision::Visit;  // don't skip
  }
};

// Forward declaration
template<typename Type, typename Visitor, typename Filter>
typename Visitor::Result traverse(
  Type& object, Visitor& visitor, const Filter& filter, bool skipSchema);

namespace Detail {

template<typename Parent, typename Member, typename Visitor, typename Filter>
typename Visitor::Result visitFieldImpl(
  const Field<Parent, Member>& field, Member& value,
  Visitor& visitor, const Filter& filter, std::true_type) {

  using Result = typename Visitor::Result;

  const VisitResult<Result> visitResult =
    visitor.enter(field, value);

  switch (visitResult.decision) {
    case VisitDecision::Handled:
      return visitor.leave(field, value);

    case VisitDecision::Error:
      return visitResult.result;

    case VisitDecision::Traverse:
      break;
  }

  if (Result result = traverse(value, visitor, filter, false); !result)
    return result;

  return visitor.leave(field, value);
}

template<typename Parent, typename Member, typename Visitor, typename Filter>
typename Visitor::Result visitFieldImpl(
  const Field<Parent, Member>& field, Member& value,
  Visitor& visitor, const Filter&, std::false_type) {

  return visitor.field(field, value);
}

template<typename Parent, typename Member, typename Visitor, typename Filter>
typename Visitor::Result visitField(
  Parent& object, const Field<Parent, Member>& field,
  Visitor& visitor, const Filter& filter) {

  if (filter(field) == FilterDecision::Skip)
    return {};

  Member& value = object.*(field.member);

  return visitFieldImpl(
    field, value, visitor, filter,
    std::bool_constant<Schema<Member>::reflected>{});
}

}  // namespace Detail

// `traverse()` continues a traversal without finalizing the result.
// See the `visit()` overloads for the full visitor and filter requirements.
//
// `skipSchema` applies only to the object currently being traversed. It does
// not propagate to nested objects. This allows a caller to skip the schema
// callback for a specific object while still processing schemas of its descendants.
template<typename Type, typename Visitor, typename Filter>
typename Visitor::Result traverse(
  Type& object, Visitor& visitor, const Filter& filter, bool skipSchema) {

  typename Visitor::Result result{};

  std::apply(
    [&](auto const&... field) {
      (
        [&] {
          if (!result)
            return;

          result = Detail::visitField(
            object,
            field,
            visitor,
            filter);
        }(),
        ...);
    },
    Schema<Type>::fields);

  if (!result)
    return result;

  if (skipSchema)
    return {};

  return visitor.schema(object);
}

// `traverse()` overload using the default `NoFilter`.
template<typename Type, typename Visitor>
typename Visitor::Result traverse(
  Type& object, Visitor& visitor) {

  return traverse(object, visitor, NoFilter{}, false);
}

// See the `visit()` overload for the traversal explanation and visitor requirements.
//
// #### Filter
//
// The filter must provide:
//
// ```cpp
//  template<typename Parent, typename Member>
//  constexpr Reflection::FilterDecision operator()(
//   const Reflection::Field<Parent, Member>& field) const;
// ```
//
// Returning `FilterDecision::Skip` skips the field and its value.
template<typename Type, typename Visitor, typename Filter>
typename Visitor::Result visit(
  Type& object, Visitor& visitor, const Filter& filter, bool skipSchema) {

  typename Visitor::Result result =
    traverse(object, visitor, filter, skipSchema);
  return visitor.finalize(result);
}

// #### Traversal
//
// `visit()` starts a complete traversal and finalizes the result exactly once.
// It is the intended entry point for starting a new traversal.
//
// `traverse()` continues a traversal without finalizing the result.
// The filter is propagated to recursive/nested traversal.
//
// `schema()` is called after all fields of the current object have been
// successfully traversed, unless `skipSchema` is true.
//
// `skipSchema` applies only to the object currently being traversed. It does
// not propagate to nested objects. This allows a caller to skip the schema
// callback for a specific object while still processing schemas of its
// descendants.
//
// `finalize()` is called at the end of the complete traversal. This allows
// the visitor to perform final processing that depends on the complete
// traversal, such as saving the final path.
//
// This distinction keeps traversal type-agnostic: the traversal engine only
// knows about reflected fields and the Visitor interface, while visitors may
// recursively traverse domain-specific types such as collections.
//
// When traversal stops with an error, the visitor may leave its traversal state
// (such as a path) at the point of failure.
//
// #### Visitor
//
// Any type passed as the Visitor argument must provide:
//
// ```cpp
// template<typename Parent, typename Member>
// VisitResult enter(
//   const Reflection::Field<Parent, Member>& field,
//   Member& value);
//
// template<typename Parent, typename Member>
// Result field(
//   const Reflection::Field<Parent, Member>& field,
//   Member& value);
//
// template<typename Parent, typename Member>
// Result leave(
//   const Reflection::Field<Parent, Member>& field,
//   Member& value);
//
// template<typename Type>
// Result schema(Type& value);
//
// Result finalize(Result result);
// ```
template<typename Type, typename Visitor>
typename Visitor::Result visit(Type& object, Visitor& visitor) {

  return visit(object, visitor, NoFilter{}, false);
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
    if (m_depth >= SchemaLimits::kMaxDepth)
      return;

    m_pathLenAtDepth[m_depth++] = m_pathLen;

    append(
      "%s%.*s",
      m_depth > 1 ? "." : "",
      static_cast<int>(fieldName.size()),
      fieldName.data());
  }

  void index(size_t index) {
    if (m_depth >= SchemaLimits::kMaxDepth)
      return;

    m_pathLenAtDepth[m_depth++] = m_pathLen;

    append("[%zu]", index);
  }

  void leave() {
    if (m_depth == 0)
      return;

    m_depth--;
    m_pathLen = m_pathLenAtDepth[m_depth];
  }

  std::string_view view() const {
    return { m_path.data(), m_pathLen };
  }

private:
  template<typename... Args>
  void append(const char* format, Args... args) {
    if (m_pathLen >= SchemaLimits::kMaxPathLength - 1)
      return;

    size_t written = snprintf(
      m_path.data() + m_pathLen,
      SchemaLimits::kMaxPathLength - m_pathLen,
      format,
      args...);

    if (written > 0)
      m_pathLen = std::min(
        m_pathLen + written,
        SchemaLimits::kMaxPathLength - 1);  // clamp: snprintf can report more than it wrote
  }

  std::array<char, SchemaLimits::kMaxPathLength> m_path    = {};
  size_t                                         m_pathLen = 0;

  size_t m_pathLenAtDepth[SchemaLimits::kMaxDepth] = {};
  size_t m_depth                                   = 0;
};

}  // namespace Reflection
