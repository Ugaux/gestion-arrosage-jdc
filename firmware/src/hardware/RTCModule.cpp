#include "RTCModule.h"

RTC_DS3231 rtc;

time_t syncTimeFromRTC() {
  DateTime now = rtc.now();
  return now.unixtime();
}
