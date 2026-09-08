#pragma once

#include <array>
#include <bitset>
#include <PCF8574.h>
#include "Constants.h"
#include "hardware/Valve.h"

class SystemMonitor;
class FaultManager;
class Wire;

// Scheduler is controlling a valve → Scheduler wins
// Scheduler is NOT controlling a valve → manual open/close wins
class ValveController {
public:
  static constexpr unsigned long kManualOpenTimeoutMs    = 1 * 60 * 1000;
  static constexpr unsigned long kTimeoutCheckIntervalMs = 500;

  // Limits inrush current when opening multiple valves
  static constexpr unsigned long kSwitchOnDelayMs = 300;
  // Limits water hammer when closing multiple valves
  static constexpr unsigned long kSwitchOffDelayMs = 500;

  ValveController(TwoWire &wire, SystemMonitor &monitor, FaultManager &faults);

  void begin();
  void update(unsigned long now);
  bool isBusy() const;

  // Manual opening of a valve
  void open(uint8_t valveIndex);
  // Manual closing of a valve
  void close(uint8_t valveIndex);

  // Scheduler takes control of the valve and opens it.
  void schedulerOpen(uint8_t valveIndex);
  // Scheduler releases control of the valve and closes it.
  // Manual control can be applied again afterward.
  void schedulerRelease(uint8_t valveIndex);

  bool isOpen(uint8_t valveIndex) const;

  // Cyclic testing of all relays
  void testAllValves();

private:
  bool isOpenDesired(uint8_t valveIndex) const;

  SystemMonitor &m_monitor;
  FaultManager  &m_faults;

  // https://github.com/xreef/PCF8574_library#-basic-usage
  PCF8574 m_ioExpander;

  std::array<Valve, kValveCount> m_valves;
  std::bitset<kValveCount>       m_failedValves;
  std::bitset<kValveCount>       m_desiredOpenValves;
  std::bitset<kValveCount>       m_schedulerControlledValves;

  std::array<unsigned long, kValveCount> m_manualOpenSince{};

  bool m_initialized = false;

  unsigned long m_lastChange   = 0;
  unsigned long m_currentDelay = 0;

  unsigned long m_lastTimeoutCheck = 0;
};
