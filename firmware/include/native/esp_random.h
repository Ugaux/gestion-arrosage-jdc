#pragma once

#ifdef PLATFORM_NATIVE
#include <cstdint>
#include <cstddef>
#include <random>

inline void esp_fill_random(void* buf, size_t len) {
  static std::mt19937                rng{ std::random_device{}() };
  std::uniform_int_distribution<int> dist(0, 255);
  auto*                              p = static_cast<uint8_t*>(buf);
  for (size_t i = 0; i < len; i++) p[i] = static_cast<uint8_t>(dist(rng));
}
#endif
