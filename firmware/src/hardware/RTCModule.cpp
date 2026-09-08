#include "RTCModule.h"

#include <esp_sntp.h>
#include <Wire.h>

time_t RTCModule::utcToEpoch(const tm& date) {
  int year  = date.tm_year + 1900;
  int month = date.tm_mon + 1;

  if (month <= 2) {
    --year;
    month += 12;
  }

  int64_t days =
    365LL * year
    + year / 4
    - year / 100
    + year / 400
    + (153 * (month - 3) + 2) / 5
    + date.tm_mday
    - 719469;

  int64_t seconds =
    days * 86400LL
    + static_cast<int64_t>(date.tm_hour) * 3600LL
    + static_cast<int64_t>(date.tm_min) * 60LL
    + static_cast<int64_t>(date.tm_sec);

  return static_cast<time_t>(seconds);
}

RTCModule::RTCModule(TwoWire& i2c, FaultManager& faults)
  : m_i2c(i2c), m_faults(faults) {}

void RTCModule::begin() {
  log_d("RTCModule is initializing...");

  // Configure Paris time zone (CET/CEST) with automatic DST.
  // RTC and ESP system clock are kept in UTC.
  // Local CET/CEST is applied only when converting/displaying time.
  // This is configured even if the physical RTC is unavailable.
  configTzTime(
    "CET-1CEST,M3.5.0/2,M10.5.0/3",
    "pool.ntp.org");

  const auto now = millis();

  // Allow the first update() call to execute immediately.
  m_lastUpdateTime = now - kUpdateDelayMs;

  if (!connectToRtc()) {
    enterFault(Fault::Code::DeviceNotFound, now);
    if (!setDefaultESPTime())
      log_w("Unable to set default fallback time!");
    return;
  }

  if (m_rtc.lostPower()) {
    // lostPower() returning true means "RTC needs synchronization/recovery."
    m_faults.set(Fault::Component::RTCModule, Fault::Code::LostPower);
    m_rtcLostPower = true;
    if (!setDefaultESPTime())
      log_w("Unable to set default fallback time!");
    // DO NOT WRITE/ADJUST THE RTC HERE.
    // adjust() clears the DS3231's OSF (lost-power) flag.
    // The RTC will be adjusted once a valid time is available,
    // via NTP or setUserTime().
    return;
  }

  if (!syncESPClockFromRTC(now)) {
    enterFault(Fault::Code::SyncFromRTCFailed, now);
    if (!setDefaultESPTime())
      log_w("Unable to set default fallback time!");
    return;
  }

  log_i("RTCModule has initialized");
}

void RTCModule::update(unsigned long now) {
  if (now - m_lastUpdateTime < kUpdateDelayMs)
    return;

  m_lastUpdateTime = now;

  switch (m_state) {
    case TimeSyncState::OnlyRTC:
      if (m_wifiConnected) {
        transitionTo(TimeSyncState::WaitingNTP, now);
        break;
      }
      if (!m_rtcLostPower && syncDue(now) && !syncESPClockFromRTC(now)) {
        if (++m_rtcSyncFailures >= kMaxRtcSyncFailures)
          enterFault(Fault::Code::SyncFromRTCFailed, now);
        else
          log_w("Sync failure: RTC → ESP");
      }
      break;

    case TimeSyncState::WaitingNTP:
      if (!m_wifiConnected) {
        transitionTo(TimeSyncState::OnlyRTC, now);
        break;
      }
      if (!m_rtcLostPower && syncDue(now) && !syncESPClockFromRTC(now)) {
        if (++m_rtcSyncFailures >= kMaxRtcSyncFailures)
          enterFault(Fault::Code::SyncFromRTCFailed, now);
        else
          log_w("Sync failure: RTC → ESP");
        break;
      }
      if (sntp_get_sync_status() == SNTP_SYNC_STATUS_COMPLETED) {
        transitionTo(TimeSyncState::ActiveNTP, now);
        break;
      }
      if (now - m_ntpWaitStart >= kNTPWarnDelayMs) {
        log_w("NTP is unavailable");
        m_ntpWaitStart = now;
      }
      break;

    case TimeSyncState::ActiveNTP:
      if (!m_wifiConnected) {
        transitionTo(TimeSyncState::OnlyRTC, now);
        break;
      }
      if (syncDue(now) && !syncRTCFromESPClock(now)) {
        if (++m_rtcSyncFailures >= kMaxRtcSyncFailures)
          enterFault(Fault::Code::SyncRTCFailed, now);
        else
          log_w("Sync failure: ESP → RTC");
      }
      break;

    case TimeSyncState::Fault:
      // Stay here until reset of ESP32.
      break;
  }
}

void RTCModule::setWifiStatus(bool connected) {
  m_wifiConnected = connected;
}

RTCModule::SetTimeResult RTCModule::setUserTime(time_t timestamp) {
  if (m_state == TimeSyncState::ActiveNTP)
    return SetTimeResult::NtpActive;

  if (!setRTC(timestamp))
    return SetTimeResult::AdjustRtcFailed;

  log_i("RTC updated with user time (UTC)");
  m_forceSync = true;

  return SetTimeResult::Success;
}

bool RTCModule::syncESPClockFromRTC(unsigned long now) {
  if (!m_rtcConnected)
    return false;

  DateTime rtcDate = m_rtc.now();

  if (rtcDate.year() < kMinValidRtcYear
      || rtcDate.year() > kMaxValidRtcYear)
    return false;

  tm date      = {};
  date.tm_sec  = rtcDate.second();
  date.tm_min  = rtcDate.minute();
  date.tm_hour = rtcDate.hour();
  date.tm_mday = rtcDate.day();
  date.tm_mon  = rtcDate.month() - 1;
  date.tm_year = rtcDate.year() - 1900;

  // RTC contains UTC, so don't apply the local timezone.
  time_t timestamp = utcToEpoch(date);

  timeval tv = {
    .tv_sec  = timestamp,
    .tv_usec = 0
  };

  if (settimeofday(&tv, nullptr) != 0)
    return false;

  m_forceSync = false;
  m_lastSync  = now;
  log_i("Sync update: RTC (UTC) -> ESP clock");

  m_rtcSyncFailures = 0;
  return true;
}

bool RTCModule::syncRTCFromESPClock(unsigned long now) {
  time_t currentTime = time(nullptr);

  if (!setRTC(currentTime))
    return false;

  m_forceSync = false;
  m_lastSync  = now;

  log_i("Sync update: ESP clock (UTC) -> RTC");
  m_rtcSyncFailures = 0;

  if (m_rtcLostPower) {
    m_rtcLostPower = false;
    m_faults.clear(Fault::Component::RTCModule, Fault::Code::LostPower);
    log_i("RTC recovered from lost power using NTP!");
  }

  return true;
}

bool RTCModule::setRTC(time_t timestamp) {
  if (!m_rtcConnected)
    return false;

  tm date;

  // ESP contains UTC, so don't apply the local timezone.
  if (!gmtime_r(&timestamp, &date))
    return false;

  m_rtc.adjust(DateTime(
    date.tm_year + 1900,
    date.tm_mon + 1,
    date.tm_mday,
    date.tm_hour,
    date.tm_min,
    date.tm_sec));

  // Verify that the RTC actually accepted the time.
  time_t actualTime   = m_rtc.now().unixtime();
  time_t absoluteDiff = actualTime >= timestamp
                          ? actualTime - timestamp
                          : timestamp - actualTime;

  return absoluteDiff <= 1;
}

bool RTCModule::syncDue(unsigned long now) const {
  return m_forceSync || now - m_lastSync >= kTimeSyncDelay;
}

void RTCModule::enterFault(Fault::Code code, unsigned long now) {
  if (m_state == TimeSyncState::Fault)
    return;

  m_faults.set(Fault::Component::RTCModule, code);
  transitionTo(TimeSyncState::Fault, now);
}

void RTCModule::transitionTo(TimeSyncState state, unsigned long now) {
  if (m_state == state)
    return;

  m_state = state;
  onEnterState(state, now);
}

void RTCModule::onEnterState(TimeSyncState state, unsigned long now) {
  switch (state) {

    case TimeSyncState::OnlyRTC:
      log_i("Using only RTC");
      m_rtcSyncFailures = 0;
      break;

    case TimeSyncState::WaitingNTP:
      log_i("Waiting for NTP...");
      m_ntpWaitStart = now;
      break;

    case TimeSyncState::ActiveNTP:
      log_i("NTP is now active!");
      m_rtcSyncFailures = 0;
      m_forceSync       = true;
      break;

    case TimeSyncState::Fault:
      log_i("RTCModule is in fault state!");
      break;
  }
}

bool RTCModule::connectToRtc() {
  m_rtcConnected = m_rtc.begin(&m_i2c);
  return m_rtcConnected;
}

bool RTCModule::setDefaultESPTime() const {
  tm date      = {};
  date.tm_year = 2026 - 1900;
  date.tm_mon  = 1 - 1;
  date.tm_mday = 1;
  date.tm_hour = 10;
  date.tm_min  = 0;
  date.tm_sec  = 0;

  // User edited value, apply local timezone.
  time_t timestamp = mktime(&date);

  timeval tv = {
    .tv_sec  = timestamp,
    .tv_usec = 0
  };

  if (settimeofday(&tv, nullptr) != 0)
    return false;

  log_i("ESP clock initialized from default (local time)");
  return true;
}
