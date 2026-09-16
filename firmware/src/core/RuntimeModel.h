#pragma once

#include <limits>
#include <cstddef>
#include "config/generated/Config.h"

namespace Runtime {

struct Schedule {
public:
  void configure(const Config::Schedule& config, uint8_t seasonalFactor);
  void update(time_t now);

  void start();
  bool isDue() const { return m_due; }
  bool isStarted() const { return m_started; }

  // Returns duration adjusted with seasonal factor (in seconds)
  time_t adjustedDuration() const;

  const Config::Schedule& config() const { return m_config; }

  // @param includeActive
  // If true, an occurrence whose scheduled start has already passed but whose duration
  // has not yet elapsed may be selected. This is used when building/restoring runtime
  // state so schedules can recover after restart or when added/updated during their
  // active window.
  void advanceToNextOccurrence(bool includeActive = false);

private:
  bool matchesFrequency(const tm& date) const;

  Config::Schedule m_config;
  uint8_t          m_seasonalFactor = 100;
  time_t           m_currentTime    = 0;
  time_t           m_occurrence     = std::numeric_limits<time_t>::max();
  bool             m_due            = false;
  bool             m_started        = false;
  time_t           m_endTime        = 0;
};

class Line {
public:
  enum class ControlMode : uint8_t {
    Auto = 0,
    ManualOn
  };

  void configure(const Config::Line& config);
  void update(time_t now);

  const UUID& getId() const { return m_config.id; }
  time_t      getManualEndTime() const { return m_manualEndTime; }
  bool        isManualOn() const { return m_mode == ControlMode::ManualOn; }

  // ManualOn overrides schedules completely
  void setManualOn(time_t until);
  void setAuto();

  // Starts watering by opening all valves belonging to this line.
  // Idempotent: does nothing if watering is already active.
  void start(ValveController& valves);
  // Stops watering by closing all valves belonging to this line.
  // Idempotent: does nothing if watering is already inactive.
  void stop(ValveController& valves);

  uint8_t         scheduleCount() const { return m_scheduleCount; }
  bool            addSchedule(const Config::Schedule& config, float seasonalFactor);
  Schedule*       schedule(uint8_t index);
  const Schedule* schedule(uint8_t index) const;

private:
  using ScheduleArray = std::array<Schedule, Config::MaxSchedulePerLine>;
  ScheduleArray m_schedules;
  uint8_t       m_scheduleCount = 0;

  Config::Line m_config;
  time_t       m_currentTime   = 0;
  bool         m_watering      = false;
  uint32_t     m_manualEndTime = 0;
  ControlMode  m_mode          = ControlMode::Auto;
};

}  // namespace Runtime
