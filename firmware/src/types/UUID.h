/***********************************************************************************/
/*                                                                                 */
/*   Adapted from esp32-uuid by typester https://github.com/typester/esp32-uuid/   */
/*                                                                                 */
/***********************************************************************************/

#pragma once

#include <string_view>
#include <array>
#include <cstdint>
#include <cstddef>

// ### UUID
// version 4
// variant RFC 4122
class UUID {
public:
  // 32 characters in 5 sections separated by 4 hyphens '-'
  static constexpr uint8_t StringLength = 36;
  // 32 characters in 5 sections separated by 4 hyphens '-' + null terminator
  static constexpr uint8_t StringSize = StringLength + 1;

  using String = std::array<char, StringSize>;

  // Same UUID
  bool operator==(const UUID &otherId) const;
  // Different UUID
  bool operator!=(const UUID &otherId) const;
  // Deterministic ordering, useful for std::map/std::set
  bool operator<(const UUID &otherId) const;

  static UUID generate();
  static bool parse(std::string_view in, UUID &out);

  String unparse() const;

  bool isDefault() const;

private:
  uint8_t m_bytes[16] = {};
};
