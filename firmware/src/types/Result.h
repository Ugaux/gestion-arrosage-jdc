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

// Uses ErrorTraits<Error> to generate the default error message.
// Define a ResultDetail::ErrorTraits specialization for Error when
// using this constructor. The constructors taking an explicit message
// do not require an ErrorTraits specialization.
template<typename Error>
class [[nodiscard]] Result {
public:
  static constexpr size_t kMaxErrorLength = 100;

  Result()
    : m_ok(true) {}

  explicit Result(Error error)
    : m_ok(false), m_error(error) {
    setMessage(errToText(error));
  }

  explicit Result(Error error, std::string_view message)
    : m_ok(false), m_error(error) {
    setMessage(message);
  }

  template<typename... Args>
  explicit Result(Error error, const char* messageFormat, Args... args)
    : m_ok(false), m_error(error) {
    setMessage(messageFormat, args...);
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
  static constexpr const char* errToText(Error error) {
    return ResultDetail::ErrorTraits<Error>::toText(error);
  }

  void setMessage(std::string_view message) {
    const size_t size     = message.size();
    const size_t copySize = std::min(size, m_msg.size() - 1);

    std::memcpy(m_msg.data(), message.data(), copySize);
    m_msg[copySize] = '\0';

    m_msg_len = copySize;
  }

  template<typename... Args>
  void setMessage(const char* messageFormat, Args... args) {
    m_msg_len = 0;

    // Format detailed message

    const int written = snprintf(
      m_msg.data(),
      m_msg.size(),
      messageFormat, args...);

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

  bool m_ok = true;

  FixedString<SchemaLimits::kMaxPathLength> m_path;

  Error m_error{};

  std::array<char, kMaxErrorLength> m_msg     = {};
  size_t                            m_msg_len = 0;
};
