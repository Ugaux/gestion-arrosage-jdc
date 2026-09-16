#pragma once

#include <cstdint>

inline constexpr uint8_t kValveCount = 8;

namespace Pumps {

namespace Watering {
inline constexpr char kName[] = "watering";
inline constexpr char kOnCommand[] =
  "101000000110101010110100";
}  // namespace Watering

namespace WaterTank {
inline constexpr char kName[] = "water tank";
inline constexpr char kOnCommand[] =
  "101000000110101010110010";
}  // namespace WaterTank

}  // namespace Pumps

namespace SchemaLimits {

// longest "a.b[2].c." path ever build
inline constexpr size_t kMaxPathLength = 64;
// deepest struct nesting in the schema
inline constexpr size_t kMaxDepth = 8;

inline constexpr size_t kMaxErrorMessageLength = 96;

}  // namespace SchemaLimits
