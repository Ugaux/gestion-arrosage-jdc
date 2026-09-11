#pragma once

#include <cstdint>
#include "types/Result.h"

namespace Validation {

enum class Error : uint8_t {
  MissingAdapterFunction = 0,
  UnexpectedValueType,

  DuplicateValue,
  CountOutOfRange,
  ValueOutOfRange,
  LengthOutOfRange,
  InvalidReference,
  OperationFailed,
  UnsupportedValue,
};

}  // namespace Validation

namespace ResultDetail {

template<>
struct ErrorTraits<Validation::Error> {
  static constexpr const char* toText(Validation::Error e) {
    switch (e) {

      case Validation::Error::MissingAdapterFunction:
        return "missing validator adapter function";

      case Validation::Error::UnexpectedValueType:
        return "unexpected value type";

      case Validation::Error::DuplicateValue:
        return "duplicate value";

      case Validation::Error::CountOutOfRange:
        return "count out of range";

      case Validation::Error::ValueOutOfRange:
        return "value out of range";

      case Validation::Error::LengthOutOfRange:
        return "length out of range";

      case Validation::Error::InvalidReference:
        return "invalid reference";

      case Validation::Error::OperationFailed:
        return "operation failed";

      case Validation::Error::UnsupportedValue:
        return "unsupported value";
    }

    return "unknown error";
  }
};

}  // namespace ResultDetail
