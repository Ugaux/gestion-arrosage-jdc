#include "RTCModule.h"

RTC_DS3231 rtc;

DateTime getCurrentTime() {
  return rtc.now();
}

time_t syncTimeFromRTC() {
  DateTime now = rtc.now();
  return now.unixtime();
}
