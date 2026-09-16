#include "WateringScheduler.h"

#include <time.h>
#include "core/SystemMonitor.h"
#include "core/FaultManager.h"
#include "core/AppContext.h"
#include "hardware/controllers/SensorController.h"
#include "hardware/controllers/PumpController.h"
#include "types/Frequency.h"
#include "types/WeekDays.h"

WateringScheduler::WateringScheduler(AppContext& app)
  : m_app(app) {}

void WateringScheduler::begin() {
  log_d("Scheduler is initializing...");

  if (!onWateringChanged) {
    log_e("onWateringChanged callback is not configured!");
    while (true) delay(1000);
  }

  if (!reloadRuntime()) {
    m_app.faults.set(
      Fault::Component::Scheduler,
      Fault::Code::RuntimeBuildFailed);
    return;
  }

  log_i("Scheduler has initialized");
}

void WateringScheduler::update(time_t now_wall) {
  // Checks schedule once for each wall-clock second.
  if (now_wall == m_lastUpdateSecond)
    return;

  m_lastUpdateSecond = now_wall;

  if (!m_app.monitor.canSchedule())
    return;

  std::lock_guard lock(m_mutex);

  bool shouldRunPump = false;

  for (uint8_t i = 0; i < m_lineCount; i++) {
    Runtime::Line& line = m_lines[i];

    bool shouldRunLine = false;

    line.update(now_wall);

    if (line.isManualOn())
      shouldRunLine = true;

    else {
      for (uint8_t j = 0; j < line.scheduleCount(); j++) {
        Runtime::Schedule* schedule = line.schedule(j);

        if (!schedule)
          continue;

        if (!schedule->config().enabled)
          continue;

        // Schedule is enabled, so now update its state
        schedule->update(now_wall);

        if (schedule->isStarted()) {
          shouldRunLine = true;
          continue;
        }

        // Waiting for next schedule run

        if (!schedule->isDue())
          continue;

        // Schedule is due

        if (schedule->config().onlyIfDrySoil && isSoilMoist()) {
          log_i("Schedule '%s' of %02d:%02d skipped because soil is moist",
                schedule->config().id.unparse().data(),
                schedule->config().hour,
                schedule->config().minute);
          schedule->advanceToNextOccurrence();
          continue;
        }

        // Soil moisture is under threshold or schedule bypasses
        // the check, so it is time to start the schedule!

        shouldRunLine = true;
        schedule->start();
      }
    }

    if (shouldRunLine) {
      line.start(m_app.valves);
      shouldRunPump = true;
    } else
      line.stop(m_app.valves);
  }

  if (shouldRunPump)
    m_app.pumps.requestOn(PumpController::Type::Watering);
  else
    m_app.pumps.requestOff(PumpController::Type::Watering);

  onWateringChanged(shouldRunPump);
}

void WateringScheduler::setManualOn(const UUID& lineId, uint8_t duration) {
  std::lock_guard lock(m_mutex);

  Runtime::Line* line = findLine(lineId);
  if (!line) return;

  line->setManualOn(time(nullptr) + duration);
}

void WateringScheduler::setAuto(const UUID& lineId) {
  std::lock_guard lock(m_mutex);

  Runtime::Line* line = findLine(lineId);
  if (!line) return;

  line->setAuto();
}

bool WateringScheduler::reloadRuntime() {
  if (!m_app.monitor.canSchedule())
    return false;

  // Takes consistent snapshots
  auto userSettings    = m_app.config.getUserSettings();
  auto schedulesConfig = m_app.config.getSchedules();

  RuntimeLineArray tempRuntimesLines;
  uint8_t          tempRuntimeLineCount = 0;

  for (uint8_t i = 0; i < userSettings.wateringModel.lines.getCount(); i++) {

    if (tempRuntimeLineCount >= tempRuntimesLines.size())
      return false;

    Runtime::Line& runtimeLine =
      tempRuntimesLines[tempRuntimeLineCount++];

    if (!userSettings.wateringModel.lines[i])
      return false;

    runtimeLine.configure(*userSettings.wateringModel.lines[i]);

    if (!buildRuntimeSchedules(runtimeLine, schedulesConfig, userSettings))
      return false;
  }

  std::lock_guard lock(m_mutex);

  m_userSettings = userSettings;

  m_lines     = tempRuntimesLines;
  m_lineCount = tempRuntimeLineCount;

  return true;
}

bool WateringScheduler::buildRuntimeSchedules(Runtime::Line&                     runtimeLine,
                                              ConfigManager::ScheduleCollection& schedulesConfig,
                                              ConfigManager::UserSettings&       userSettings) {
  for (uint8_t i = 0; i < schedulesConfig.getCount(); i++) {

    if (!schedulesConfig[i])
      return false;

    const Config::Schedule& scheduleConfig = *schedulesConfig[i];

    if (scheduleConfig.lineId != runtimeLine.getId())
      continue;

    if (!runtimeLine.addSchedule(
          scheduleConfig,
          m_userSettings.params.watering.seasonal.factor))
      return false;
  }

  return true;
}

bool WateringScheduler::isSoilMoist() const {
  return m_app.sensors.getSoilMoisture()
         > m_userSettings.params.watering.soil.moisture.threshold;
}

Runtime::Line* WateringScheduler::findLine(const UUID& id) {
  for (uint8_t i = 0; i < m_lineCount; i++) {
    if (m_lines[i].getId() == id) return &m_lines[i];
  }
  return nullptr;
}
