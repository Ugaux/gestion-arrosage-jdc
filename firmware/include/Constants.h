#pragma once

#include <cstdint>

inline constexpr uint8_t kValveCount = 8;

namespace SchemaLimits {

// longest "a.b[2].c." path ever build
inline constexpr size_t kMaxPathLength = 64;
// deepest struct nesting in the schema
inline constexpr size_t kMaxDepth = 8;

inline constexpr size_t kMaxErrorMessageLength = 96;

}  // namespace SchemaLimits
