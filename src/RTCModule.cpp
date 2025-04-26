#include "RTCModule.h"

RTC_DS3231 rtc;


void setupRTC() {
  if (!rtc.begin()) {
    Serial.println("Couldn't find RTC");
    while (1);
  }

  if (rtc.lostPower()) {
    Serial.println("RTC lost power, setting the time!");
    rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
  }
}

DateTime getCurrentTime() {
  return rtc.now();
}

time_t syncTimeFromRTC() {
    DateTime now = rtc.now();
    return now.unixtime();
}
