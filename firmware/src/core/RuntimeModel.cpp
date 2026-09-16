#include "RuntimeModel.h"

namespace Runtime {

void Schedule::configure(const Config::Schedule& config, uint8_t seasonalFactor) {
  m_config         = config;
  m_seasonalFactor = seasonalFactor;
  m_currentTime    = time(nullptr);
}

void Schedule::update(time_t now) {
  m_currentTime = now;

  if (m_started) {
    if (now < m_endTime)
      log_d("Schedule '%s' done in %llds",
            m_config.id.unparse().data(),
            static_cast<long long>(m_endTime - now));
    else {
      log_i("Schedule '%s' has finished",
            m_config.id.unparse().data());
      m_started = false;
      m_endTime = 0;
      advanceToNextOccurrence();
    }
  }

  m_due = !m_started && now >= m_occurrence;
}

void Schedule::start() {
  log_i("Schedule '%s' of %02d:%02d has started",
        m_config.id.unparse().data(),
        m_config.hour,
        m_config.minute);

  m_endTime = m_occurrence + adjustedDuration();
  m_started = true;
}

time_t Schedule::adjustedDuration() const {
  return static_cast<time_t>(m_config.duration) * 60
         * m_seasonalFactor / 100;
}

void Schedule::advanceToNextOccurrence(bool includeActive) {
  tm date;

  if (!localtime_r(&m_currentTime, &date))
    return;

  date.tm_hour = m_config.hour;
  date.tm_min  = m_config.minute;
  date.tm_sec  = 0;

  date.tm_isdst = -1;

  if (!includeActive)
    date.tm_mday++;

  // If active is included, today may be rejected because its
  // scheduled time has already passed, so we must also inspect
  // the same weekday 7 days later (so 7+1=8).
  const uint8_t daysToCheck = includeActive ? 8 : 7;

  for (uint8_t i = 0; i < daysToCheck; ++i) {
    time_t timeCandidate = mktime(&date);

    if (matchesFrequency(date)) {
      // A future occurrence.
      if (m_currentTime < timeCandidate) {
        log_d("Shedule '%s' advanced to future occurrence in %ds",
              m_config.id.unparse().data(), timeCandidate - m_currentTime);
        m_occurrence = timeCandidate;
        return;
      }

      // When active occurrences are allowed, consider
      // whether this occurrence is still active.
      if (includeActive && (m_currentTime < timeCandidate + adjustedDuration())) {
        log_d("Shedule '%s' advanced to active occurrence",
              m_config.id.unparse().data());
        m_occurrence = timeCandidate;
        return;
      }
    }

    date.tm_mday++;
  }

  // Should be unreachable with a validated config.
  m_occurrence = std::numeric_limits<time_t>::max();
}

bool Schedule::matchesFrequency(const tm& date) const {

  switch (m_config.frequency) {

    case Frequency::EveryDay:
      return true;

    case Frequency::EvenDays:
      return (date.tm_mday % 2) == 0;

    case Frequency::OddDays:
      return (date.tm_mday % 2) == 1;

    case Frequency::SpecificDays:
      return m_config.days.contains(date);
  }

  return false;
}

void Line::configure(const Config::Line& config) {
  m_config = config;
}

void Line::update(time_t now) {
  if (isManualOn() && now >= m_manualEndTime)
    setAuto();

  m_currentTime = now;
}

void Line::setManualOn(time_t until) {
  m_mode          = ControlMode::ManualOn;
  m_manualEndTime = until;
  log_i("Line '%s' set to ManualOn", m_config.name.c_str());
}

void Line::setAuto() {
  m_mode          = ControlMode::Auto;
  m_manualEndTime = 0;
  log_i("Line '%s' set to Auto", m_config.name.c_str());
}

void Line::start(ValveController& valves) {
  if (m_watering)
    return;

  for (size_t i = 0; i < m_config.valves.size(); i++)
    if (m_config.valves[i]) valves.schedulerOpen(i);

  log_i("Line '%s' started", m_config.name.c_str());
  m_watering = true;
}

void Line::stop(ValveController& valves) {
  if (!m_watering)
    return;

  for (size_t i = 0; i < m_config.valves.size(); i++)
    if (m_config.valves[i]) valves.schedulerRelease(i);

  log_i("Line '%s' stopped", m_config.name.c_str());
  m_watering = false;
}

bool Line::addSchedule(const Config::Schedule& config, float seasonalFactor) {
  if (m_scheduleCount >= m_schedules.size())
    return false;

  Schedule& schedule = m_schedules[m_scheduleCount++];
  schedule.configure(config, seasonalFactor);
  schedule.advanceToNextOccurrence(true);

  return true;
}

Schedule* Line::schedule(uint8_t index) {
  if (index >= m_scheduleCount)
    return nullptr;

  return &m_schedules[index];
}

const Schedule* Line::schedule(uint8_t index) const {
  if (index >= m_scheduleCount)
    return nullptr;

  return &m_schedules[index];
}

}  // namespace Runtime
