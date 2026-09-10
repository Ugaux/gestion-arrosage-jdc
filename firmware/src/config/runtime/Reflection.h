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
  Int,
  UInt,
  UUID,
  Float,
  String,
  Weekdays,
  Bitset,
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
  else if constexpr (std::is_integral_v<U> && std::is_signed_v<U>)
    return FieldType::Int;
  else if constexpr (std::is_integral_v<U> && std::is_unsigned_v<U>)
    return FieldType::UInt;
  else if constexpr (std::is_floating_point_v<U>)
    return FieldType::Float;
  else if constexpr (is_uuid_v<U>)
    return FieldType::UUID;
  else if constexpr (is_weekdays_v<U>)
    return FieldType::Weekdays;
  else if constexpr (is_collection_v<U>)
    return FieldType::Collection;
  else if constexpr (is_bitset_v<U>)
    return FieldType::Bitset;
  else if constexpr (is_fixedstring_v<U>)
    return FieldType::String;
  else if constexpr (is_frequency_v<U>)
    return FieldType::Frequency;
  else
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
  Visit = 0,
  Skip
};

enum class VisitResult : uint8_t {
  Traverse = 0,
  Handled,
  Error
};

struct NoFilter {
  bool skipSchema = false;

  template<typename Parent, typename Member>
  constexpr VisitDecision operator()(
    const Field<Parent, Member>&) const {
    return VisitDecision::Visit;  // don't skip
  }
};

// Forward declaration
template<typename Visitor, typename Type>
bool visit(Type& object, Visitor& visitor);

namespace Detail {

template<typename Visitor, typename Parent, typename Member>
bool visitFieldImpl(Parent&, const Field<Parent, Member>& field, Member& value,
                    Visitor& visitor, std::true_type) {

  switch (visitor.enter(field, value)) {
    case VisitResult::Handled:
      return true;

    case VisitResult::Error:
      return false;

    case VisitResult::Traverse:
      break;
  }

  if (!visit(value, visitor))
    return false;

  return visitor.leave(field, value);
}

template< typename Visitor, typename Parent, typename Member>
bool visitFieldImpl(Parent&, const Field<Parent, Member>& field, Member& value,
                    Visitor& visitor, std::false_type) {

  return visitor.field(field, value);
}

template<typename Filter, typename Visitor, typename Parent, typename Member>
bool visitField(Parent& object, const Field<Parent, Member>& field, Visitor& visitor, const Filter& filter) {

  if (filter(field) == VisitDecision::Skip)
    return true;

  Member& value = object.*(field.member);
  return visitFieldImpl(object, field, value, visitor,
                        std::bool_constant<Schema<Member>::reflected>{});
}

}  // namespace Detail

// #### Same visitor requirement as the non filtered version:
//
// The filter should be a struct with an `operator()` returning a `Reflection::VisitDecision`.
//
// Example:
// ```cpp
// struct ExampleFilter {
//   bool skipSchema = false;
//
//   template<typename Parent, typename Member>
//   Reflection::VisitDecision operator()(
//     const Reflection::Field<Parent, Member>& field) const {
//     ...
//     return Reflection::VisitDecision::Visit;
//   }
// };
// ```
template<typename Filter, typename Visitor, typename Type>
bool visit(Type& object, Visitor& visitor, const Filter& filter) {

  if (!std::apply(
        [&](auto const&... field) {
          return (Detail::visitField(object, field, visitor, filter) && ...);
        },
        Schema<Type>::fields))
    return false;

  if (filter.skipSchema)
    return true;

  return visitor.schema(object);
}

// #### Any type passed as the Visitor argument must provide:
//
//  - `template<typename Parent, typename Member>`
//    `bool enter(const Reflection::Field<Parent, Member>&, Member&);`
//  - `template<typename Parent, typename Member>`
//    `bool field(const Reflection::Field<Parent, Member>&, Member&);`
//  - `template<typename Parent, typename Member>`
//    `bool leave(const Reflection::Field<Parent, Member>&, Member&);`
//  - `template<typename Type>`
//    `bool schema(Type& value);`
//
// Example:
// ```cpp
// struct ExampleVisitor {
//   template<typename Parent, typename Member>
//   bool enter(const Reflection::Field<Parent, Member>& field, Member& value) {...}
//   template<typename Parent, typename Member>
//   bool field(const Reflection::Field<Parent, Member>& field, Member& value) {...}
//   template<typename Parent, typename Member>
//   bool leave(const Reflection::Field<Parent, Member>& field, Member& value) {...}
//   template<typename Type>
//   bool schema(Type& value) {...}
// };
// ```
// Returning false stops the traversal. When traversal stops, the visitor
// may leave its traversal state (such as a path) at the point of failure.
template<typename Visitor, typename Type>
bool visit(Type& object, Visitor& visitor) {
  NoFilter filter;
  return visit(object, visitor, filter);
}

// A convenient helper that generates a generic C++ path like `watering.manual.duration.step`.
//
// PathBuilder should be used by a visitor alongside its corresponding visitor method,
// with the path-building method called at the beginning of the visitor method.
//
// Example:
// ```cpp
// bool ExampleVisitor::enter(const Reflection::Field<Parent, Member>& field, Member& value) {
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
