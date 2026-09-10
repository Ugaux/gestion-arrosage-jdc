#pragma once

#include <cstddef>
#include <cstdint>
#include <array>
#include <string_view>
#include "Constants.h"
#include "types/FixedString.h"

namespace ResultDetail {

template<typename E>
struct ErrorTraits;

}  // namespace ResultDetail

template<typename Error>
class Result {
public:
  static constexpr size_t kMaxErrorLength = 100;

  Result() = default;

  explicit Result(Error e)
    : m_error(e), m_ok(false) {
    setMessage(errToText(e));
  }

  template<typename... Args>
  Result(Error e, const char* fmt, Args... args)
    : m_error(e), m_ok(false) {
    setMessage(fmt, args...);
  }

  Result withPath(std::string_view path) const {
    Result result = *this;
    result.m_path = path;
    return result;
  }

  bool ok() const { return m_ok; }

  explicit operator bool() const { return ok(); }

  Error error() const { return m_error; }

  std::string_view message() const {
    return { m_msg.data(), m_msg_len };
  }

  std::string_view path() const {
    return m_path.view();
  }

private:
  static const char* errToText(Error e) {
    return ResultDetail::ErrorTraits<Error>::toText(e);
  }

  template<typename... Args>
  void setMessage(const char* fmt, Args... args) {
    m_msg_len = 0;

    // Format detailed message

    const int written = snprintf(
      m_msg.data(),
      m_msg.size(),
      fmt, args...);

    if (written < 0)
      return;

    const size_t size =
      static_cast<size_t>(written);

    // Fits without truncation.

    if (size < m_msg.size()) {
      m_msg_len = size;
      return;
    }

    // Message was truncated

    // Need to leave room for "(N)".
    // Find a stable value for N because the number of digits
    // in N affects how much room the suffix requires.

    size_t keep = m_msg.size() - 1;

    for (;;) {
      const size_t truncated = size - keep;

      const size_t suffixLen =
        static_cast<size_t>(
          snprintf(nullptr, 0, "(+%zu)", truncated));

      const size_t newKeep =
        m_msg.size() - 1 - suffixLen;

      if (newKeep == keep)
        break;

      keep = newKeep;
    }

    const size_t truncated = size - keep;

    snprintf(
      m_msg.data() + keep,
      m_msg.size() - keep,
      "(+%zu)",
      truncated);

    m_msg_len = keep
                + static_cast< size_t>(snprintf(
                  nullptr, 0, "(+%zu)", truncated));
  }

  FixedString<SchemaLimits::kMaxPathLength> m_path;

  Error m_error{};

  bool m_ok = true;

  std::array<char, kMaxErrorLength> m_msg = {};

  size_t m_msg_len = 0;
};
