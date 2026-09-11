#pragma once

#include <iterator>
#include <cstdint>

enum class WeekDay : uint8_t {
  Monday    = 1 << 0,
  Tuesday   = 1 << 1,
  Wednesday = 1 << 2,
  Thursday  = 1 << 3,
  Friday    = 1 << 4,
  Saturday  = 1 << 5,
  Sunday    = 1 << 6
};

class WeekDays {
public:
  static constexpr uint8_t kAllDaysMask = 0x7Fu;

  WeekDays()                           = default;
  WeekDays(const WeekDays&)            = default;
  WeekDays& operator=(const WeekDays&) = default;

  explicit WeekDays(WeekDay day);

  bool operator==(const WeekDays& other) const {
    return m_mask == other.m_mask;
  }

  // Returns days that are common with `days`.
  // e.g. intersect(WeekDay::Monday | WeekDay::Friday | WeekDay::Sunday)
  WeekDays intersect(const WeekDays& days) const;

  bool contains(WeekDay day) const;
  // Returns true if all days in `days` are contained in this set.
  // e.g. contains(WeekDay::Monday | WeekDay::Friday | WeekDay::Sunday)
  bool contains(const WeekDays& days) const;
  bool contains(const tm& date) const;

  void add(WeekDay day);
  // Adds all specified days.
  // e.g. add(WeekDay::Monday | WeekDay::Friday | WeekDay::Sunday)
  void add(const WeekDays& days);

  void remove(WeekDay day);

  void set(WeekDay day);
  // Sets exactly the specified days.
  // e.g. set(WeekDay::Monday | WeekDay::Friday | WeekDay::Sunday)
  void set(const WeekDays& days);

  bool set(uint32_t mask);

  uint8_t mask() const { return m_mask; }

  bool isEmpty() const;
  void clear();

  class Iterator {
  public:
    using iterator_category = std::forward_iterator_tag;
    using value_type        = WeekDay;
    using difference_type   = std::ptrdiff_t;

    Iterator(const WeekDays& days, int bit);

    WeekDay operator*() const;

    Iterator& operator++();

    bool operator==(const Iterator& other) const;
    bool operator!=(const Iterator& other) const;

  private:
    void skipUnset();

    const WeekDays& m_days;
    int             m_bit;
  };

  Iterator begin() const;
  Iterator end() const;

private:

  uint8_t m_mask = 0;  // bitmask
};

// WeekDay | WeekDay -> WeekDays
WeekDays operator|(WeekDay lhs, WeekDay rhs);

// WeekDays | WeekDay -> WeekDays
WeekDays operator|(WeekDays lhs, WeekDay rhs);
