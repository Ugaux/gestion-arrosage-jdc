#pragma once

#include <RTClib.h>
#include "core/FaultManager.h"

enum class TimeSyncState : uint8_t {
  // RTC is currently the time source.
  // Periodically sync RTC → ESP clock.
  OnlyRTC = 0,

  // RTC remains the time source.
  // Still periodically sync RTC → ESP clock.
  // When NTP sync completes, sync ESP clock → RTC
  // and switch to NTP_SYNCED.
  WaitingNTP,

  // ESP clock is the time source.
  // Periodically sync ESP clock → RTC.
  ActiveNTP,

  Fault,
};

class TwoWire;

class RTCModule {
public:
  static constexpr unsigned long kUpdateDelayMs  = 500;
  static constexpr unsigned long kTimeSyncDelay  = 60UL * 60 * 1000;
  static constexpr unsigned long kNTPWarnDelayMs = 30UL * 1000;

  static constexpr uint8_t  kMaxRtcSyncFailures = 3;
  static constexpr uint16_t kMinValidRtcYear    = 2026;
  static constexpr uint16_t kMaxValidRtcYear    = 2100;

  enum class SetTimeResult : uint8_t {
    Success = 0,
    NtpActive,
    AdjustRtcFailed
  };

  static time_t utcToEpoch(const tm& date);

  RTCModule(TwoWire& i2c, FaultManager& faults);

  void begin();
  void update(unsigned long now);

  void setWifiStatus(bool connected);

  // Immediately sets the ESP32 clock to the given timestamp.
  // @param timestamp UTC
  SetTimeResult setUserTime(time_t timestamp);

private:
  bool syncESPClockFromRTC(unsigned long now);
  bool syncRTCFromESPClock(unsigned long now);
  bool setRTC(time_t timestamp);
  bool syncDue(unsigned long now) const;

  void enterFault(Fault::Code code, unsigned long now);
  void transitionTo(TimeSyncState state, unsigned long now);
  void onEnterState(TimeSyncState state, unsigned long now);

  bool connectToRtc();
  // The fallback time is 2026-01-01 10:00:00
  // in the configured local timezone
  bool setDefaultESPTime() const;

  TwoWire&      m_i2c;
  FaultManager& m_faults;
  RTC_DS3231    m_rtc;

  TimeSyncState m_state          = TimeSyncState::OnlyRTC;
  unsigned long m_lastUpdateTime = 0;
  bool          m_rtcConnected   = false;
  bool          m_wifiConnected  = false;
  bool          m_forceSync      = false;
  unsigned long m_lastSync       = 0;
  unsigned long m_ntpWaitStart   = 0;
  bool          m_rtcLostPower   = false;

  uint8_t m_rtcSyncFailures = 0;
};
