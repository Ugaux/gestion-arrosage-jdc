#pragma once

#include <cstddef>
#include <cstdint>
#include <array>
#include "types/UUID.h"

// T must provide a public UUID member named `id`.
//
// Example:
// ```cpp
// struct Item {
//   UUID id;
// };
// ```
template<typename T, uint8_t N>
class Collection {
public:
  static constexpr uint8_t kCapacity = N;

  enum class AddResult : uint8_t {
    Ok = 0,
    Full,
    DuplicateId,
  };

  uint8_t size() const { return m_size; }

  T& operator[](uint8_t i) {
    return m_items[i];
  }

  const T& operator[](uint8_t i) const {
    return m_items[i];
  }

  const T* find(const UUID& id) const {
    for (uint8_t i = 0; i < m_size; i++) {
      if (m_items[i].id == id) return &m_items[i];
    }
    return nullptr;
  }
  T* find(const UUID& id) {
    for (uint8_t i = 0; i < m_size; i++) {
      if (m_items[i].id == id) return &m_items[i];
    }
    return nullptr;
  }
  /// Returns the index of the item with the given ID.
  /// Returns m_size if no item with the given ID was found.
  uint8_t index(const UUID& id) const {
    for (uint8_t i = 0; i < m_size; i++) {
      if (m_items[i].id == id) return i;
    }
    return m_size;
  }

  // Adds an item unless the collection is full or its ID is already present.
  AddResult add(const T& item) {
    if (m_size >= kCapacity) return AddResult::Full;

    if (find(item.id)) return AddResult::DuplicateId;

    m_items[m_size++] = item;
    return AddResult::Ok;
  }
  // Returns false if no item with ID is present.
  bool update(const T& item) {
    for (uint8_t i = 0; i < m_size; ++i) {
      if (m_items[i].id == item.id) {
        m_items[i] = item;
        return true;
      }
    }
    return false;
  }
  // Swaps with last and decrement size
  bool removeAt(uint8_t index) {
    if (index >= m_size)
      return false;

    m_items[index] = m_items[m_size - 1];
    m_size--;
    return true;
  }
  // Swaps with last and decrement size
  bool remove(const UUID& id) {
    for (uint8_t i = 0; i < m_size; i++) {
      if (m_items[i].id == id) {
        m_items[i] = m_items[m_size - 1];
        m_size--;
        return true;
      }
    }
    return false;
  }
  void clear() {
    m_size = 0;
  }

private:
  std::array<T, kCapacity> m_items{};
  uint8_t                  m_size = 0;
};
