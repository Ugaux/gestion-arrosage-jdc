#pragma once

#include <cstdint>

enum class Frequency : uint8_t {
  EveryDay = 0,
  EvenDays,
  OddDays,
  SpecificDays
};

bool toChar(Frequency freq, unsigned char &c);

bool fromChar(unsigned char c, Frequency &freq);
