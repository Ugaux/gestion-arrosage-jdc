#pragma once

#include "config/ConfigManager.h"
#include "core/FaultManager.h"
#include "core/SystemMonitor.h"
#include "hardware/controllers/ValveController.h"
#include "hardware/controllers/SensorController.h"
#include "hardware/controllers/PumpController.h"

struct AppContext {
  ConfigManager& config;
  SystemMonitor& monitor;
  FaultManager&  faults;

  ValveController&  valves;
  SensorController& sensors;
  PumpController&   pumps;
};
