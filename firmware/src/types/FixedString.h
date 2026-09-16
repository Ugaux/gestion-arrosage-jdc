#pragma once

#include <string_view>
#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>

template<uint16_t N>
class FixedString {
public:
  static constexpr uint16_t kCapacity = N;

  constexpr FixedString(std::string_view s) {
    m_length = static_cast<uint16_t>(
      std::min(s.size(), static_cast<size_t>(kCapacity)));

    for (uint16_t i = 0; i < m_length; ++i)
      m_data[i] = s[i];

    m_data[m_length] = '\0';
  }

  constexpr FixedString(const char* s = "")
    : FixedString(std::string_view{ s }) {}

  template<typename... Args>
  void format(const char* formatString, Args... args) {
    const int written = snprintf(
      m_data.data(),
      m_data.size(),
      formatString, args...);

    if (written < 0) {
      m_length  = 0;
      m_data[0] = '\0';
      return;
    }

    m_length = static_cast<uint16_t>(
      std::min(static_cast<size_t>(written),
               static_cast<size_t>(kCapacity)));

    m_data[m_length] = '\0';
  }

  constexpr std::string_view view() const {
    return { m_data.data(), m_length };
  }

  const char* c_str() const {
    return m_data.data();
  }

  uint16_t size() const {
    return m_length;
  }

private:
  std::array<char, kCapacity + 1> m_data{};

  uint16_t m_length = 0;
};
