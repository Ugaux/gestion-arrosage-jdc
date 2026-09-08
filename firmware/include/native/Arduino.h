#pragma once

// Native stand-in for Arduino.h — only what ConfigManager/Validation/JsonDeserializer need.
#ifdef PLATFORM_NATIVE

#include <cstdio>
#include <cstdint>
#include <string>
#include <thread>
#include <chrono>

#define log_d(fmt, ...) printf("[D] " fmt "\n", ##__VA_ARGS__)
#define log_i(fmt, ...) printf("[I] " fmt "\n", ##__VA_ARGS__)
#define log_w(fmt, ...) printf("[W] " fmt "\n", ##__VA_ARGS__)
#define log_e(fmt, ...) printf("[E] " fmt "\n", ##__VA_ARGS__)

inline void delay(unsigned long ms) {
  std::this_thread::sleep_for(std::chrono::milliseconds(ms));
}

// Minimal Arduino String stand-in (only what's used: ctor, +=, clear, reserve, c_str)
class String {
public:
  String() = default;
  String(const char* s) : m_data(s ? s : "") {}
  String& operator+=(char c) {
    m_data += c;
    return *this;
  }
  void        clear() { m_data.clear(); }
  void        reserve(size_t n) { m_data.reserve(n); }
  const char* c_str() const { return m_data.c_str(); }
  size_t      length() const { return m_data.size(); }
              operator std::string() const { return m_data; }

private:
  std::string m_data;
};

#endif  // PLATFORM_NATIVE
