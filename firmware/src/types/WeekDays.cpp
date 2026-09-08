#include "WeekDays.h"

WeekDays::WeekDays(WeekDay day)
  : m_mask(static_cast<uint8_t>(day)) {
}

WeekDays WeekDays::intersect(const WeekDays& days) const {
  WeekDays result;
  result.m_mask = m_mask & days.m_mask;
  return result;
}

bool WeekDays::contains(WeekDay day) const {
  return (m_mask & static_cast<uint8_t>(day)) != 0;
}
bool WeekDays::contains(const WeekDays& days) const {
  return (m_mask & days.m_mask) == days.m_mask;
}
bool WeekDays::contains(const tm& date) const {
  // Converts tm_wday (Sun=0) to the bit numbering (Mon=0)
  uint8_t mask = (date.tm_wday + 6) % 7;

  WeekDay day = static_cast<WeekDay>(1u << mask);
  return contains(day);
}

void WeekDays::add(WeekDay day) {
  m_mask |= static_cast<uint8_t>(day);
}
void WeekDays::add(const WeekDays& days) {
  m_mask |= days.m_mask;
}

void WeekDays::remove(WeekDay day) {
  m_mask &= ~static_cast<uint8_t>(day);
}

void WeekDays::set(WeekDay day) {
  m_mask = static_cast<uint8_t>(day);
}
void WeekDays::set(const WeekDays& days) {
  m_mask = days.m_mask;
}

void WeekDays::clear() {
  m_mask = 0;
}

bool WeekDays::isEmpty() const {
  return m_mask == 0;
}

bool WeekDays::setMask(uint32_t mask) {
  if (mask & ~kAllDaysMask) {
    return false;
  }

  m_mask = static_cast<uint8_t>(mask);
  return true;
}

WeekDays::Iterator::Iterator(const WeekDays& days, int bit)
  : m_days(days), m_bit(bit) {
  skipUnset();
}

void WeekDays::Iterator::skipUnset() {
  while (m_bit < 7 && (m_days.m_mask & (1 << m_bit)) == 0)
    ++m_bit;
}

WeekDay WeekDays::Iterator::operator*() const {
  return static_cast<WeekDay>(1 << m_bit);
}

WeekDays::Iterator& WeekDays::Iterator::operator++() {
  ++m_bit;
  skipUnset();
  return *this;
}

bool WeekDays::Iterator::operator==(const Iterator& other) const {
  return &m_days == &other.m_days && m_bit == other.m_bit;
}

bool WeekDays::Iterator::operator!=(const Iterator& other) const {
  return !(*this == other);
}

WeekDays::Iterator WeekDays::begin() const {
  return Iterator(*this, 0);
}

WeekDays::Iterator WeekDays::end() const {
  return Iterator(*this, 7);
}

WeekDays operator|(WeekDay lhs, WeekDay rhs) {
  WeekDays result(lhs);
  result.add(rhs);
  return result;
}

WeekDays operator|(WeekDays lhs, WeekDay rhs) {
  lhs.add(rhs);
  return lhs;
}
