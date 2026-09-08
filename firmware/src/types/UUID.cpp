#include "UUID.h"

#include <cstring>
#include <cstdio>
#include <esp_random.h>

namespace {

uint8_t unhex_char(unsigned char c) {
  if (0x30 <= c && c <= 0x39) /* 0-9 */
    return c - 0x30;
  else if (0x41 <= c && c <= 0x46) /* A-F */
    return c - 0x41 + 0xa;
  else if (0x61 <= c && c <= 0x66) /* a-f */
    return c - 0x61 + 0xa;

  return 0xff; /* invalid string */
}

bool unhex(std::string_view s, uint8_t *r) {
  if (s.size() % 2 != 0)
    return false;

  for (size_t i = 0; i < s.size(); i += 2) {
    uint8_t h = unhex_char(static_cast<unsigned char>(s[i]));
    uint8_t l = unhex_char(static_cast<unsigned char>(s[i + 1]));

    if (h == 0xff || l == 0xff)
      return false;

    r[i / 2] = (h << 4) | (l & 0xf);
  }

  return true;
}

}  // namespace

bool UUID::operator==(const UUID &other) const {
  return memcmp(m_bytes, other.m_bytes, sizeof(m_bytes)) == 0;
}

bool UUID::operator!=(const UUID &other) const {
  return !(*this == other);
}

bool UUID::operator<(const UUID &other) const {
  return memcmp(m_bytes, other.m_bytes, sizeof(m_bytes)) < 0;
}

UUID UUID::generate() {
  UUID out;

  esp_fill_random(out.m_bytes, sizeof(out.m_bytes));

  // uuid version 4
  out.m_bytes[6] = (out.m_bytes[6] & 0x0F) | 0x40;

  // uuid variant RFC 4122
  out.m_bytes[8] = (out.m_bytes[8] & 0x3F) | 0x80;

  return out;
}

bool UUID::parse(std::string_view in, UUID &out) {
  if (in.size() != StringLength)
    return false;

  auto *op = out.m_bytes;

  if (!unhex(in.substr(0, 8), op))
    return false;

  op += 4;

  if (in[8] != '-' || !unhex(in.substr(9, 4), op))
    return false;

  op += 2;

  if (in[13] != '-' || !unhex(in.substr(14, 4), op))
    return false;

  op += 2;

  if (in[18] != '-' || !unhex(in.substr(19, 4), op))
    return false;

  op += 2;

  if (in[23] != '-' || !unhex(in.substr(24, 12), op))
    return false;

  // UUID version 4.
  if ((out.m_bytes[6] & 0xF0) != 0x40)
    return false;

  // RFC 4122 variant.
  if ((out.m_bytes[8] & 0xC0) != 0x80)
    return false;

  return true;
}

UUID::String UUID::unparse() const {
  String out{};

  snprintf(out.data(), out.size(),
           "%02x%02x%02x%02x-%02x%02x-%02x%02x-"
           "%02x%02x-%02x%02x%02x%02x%02x%02x",
           m_bytes[0], m_bytes[1], m_bytes[2], m_bytes[3],
           m_bytes[4], m_bytes[5], m_bytes[6], m_bytes[7],
           m_bytes[8], m_bytes[9], m_bytes[10], m_bytes[11],
           m_bytes[12], m_bytes[13], m_bytes[14], m_bytes[15]);

  return out;
}

// Checks for 00000000-0000-0000-0000-000000000000
bool UUID::isDefault() const {
  for (int i = 0; i < 16; i++) {
    if (m_bytes[i] != 0) return false;
  }
  return true;
}
