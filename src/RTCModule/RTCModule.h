#ifndef _RTCMODULE_H_
#define _RTCMODULE_H_

#include <Wire.h>
#include <RTClib.h>

extern RTC_DS3231 rtc;

void     setupRTC();
DateTime getCurrentTime();
time_t   syncTimeFromRTC();  // Déclaration de la nouvelle fonction

#endif
