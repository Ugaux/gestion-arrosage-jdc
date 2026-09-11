#pragma once

#include <cstdint>
#include "types/Result.h"

namespace Deserialization {

enum class Error : uint8_t {
  InvalidJson = 0,
  Missing,
  WrongType,

  InvalidValue,
  DuplicateId,
  CapacityExceeded
};

}  // namespace Deserialization

namespace ResultDetail {

template<>
struct ErrorTraits<Deserialization::Error> {
  static constexpr const char* toText(Deserialization::Error e) {
    switch (e) {

      case Deserialization::Error::InvalidJson:
        return "invalid JSON";

      case Deserialization::Error::Missing:
        return "missing element";

      case Deserialization::Error::WrongType:
        return "wrong type";

      case Deserialization::Error::InvalidValue:
        return "invalid value";

      case Deserialization::Error::DuplicateId:
        return "duplicate ID";

      case Deserialization::Error::CapacityExceeded:
        return "capacity exceeded";
    }

    return "unknown error";
  }
};

}  // namespace ResultDetail
