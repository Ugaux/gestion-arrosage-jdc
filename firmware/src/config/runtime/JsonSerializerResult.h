#pragma once

#include <cstdint>
#include "types/Result.h"

namespace Serialization {

enum class Error : uint8_t {
  // ArduinoJson ran out of buffer space
  CapacityExceeded = 0,
  // an in-memory value doesn't map to any known JSON representation
  InvalidValue
};

}  // namespace Serialization

template<>
struct ResultDetail::ErrorTraits<Serialization::Error> {
  static constexpr const char* toText(Serialization::Error e) {
    switch (e) {

      case Serialization::Error::CapacityExceeded:
        return "capacity exceeded";

      case Serialization::Error::InvalidValue:
        return "invalid value";
    }

    return "unknown error";
  }
};
