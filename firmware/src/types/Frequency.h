#pragma once

#include <cstdint>

enum class Frequency : uint8_t {
  EveryDay = 0,
  EvenDays,
  OddDays,
  SpecificDays
};
