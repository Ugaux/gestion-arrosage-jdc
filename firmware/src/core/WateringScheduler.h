#pragma once

#include <array>
#include <mutex>
#include <functional>
#include <cstdint>
#include "Constants.h"
#include "core/RuntimeModel.h"
#include "config/ConfigManager.h"
#include "hardware/controllers/ValveController.h"

struct AppContext;

// Handles:
// - manual on override
// - midnight crossing
// - overlapping schedules
class WateringScheduler {
public:
  WateringScheduler(AppContext& app);

  std::function<void(bool)> onWateringChanged;

  void begin();
  void update(time_t now_wall);

  // @param duration in seconds
  void setManualOn(const UUID& lineId, uint8_t duration);
  void setAuto(const UUID& lineId);

  bool reloadRuntime();

private:
  bool buildRuntimeSchedules(Runtime::Line&                     runtimeLine,
                             ConfigManager::ScheduleCollection& schedulesConfig,
                             ConfigManager::UserSettings&       userSettings);

  bool           isSoilMoist() const;
  Runtime::Line* findLine(const UUID& id);

  AppContext& m_app;

  ConfigManager::UserSettings m_userSettings;

  using RuntimeLineArray = std::array<Runtime::Line, kValveCount>;
  RuntimeLineArray m_lines;
  uint8_t          m_lineCount = 0;

  std::mutex m_mutex;
  time_t     m_lastUpdateSecond = 0;
};
