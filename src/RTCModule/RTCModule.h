#ifndef _RTCMODULE_H_
#define _RTCMODULE_H_

#include <RTClib.h>

void     setupRTC();
DateTime getCurrentTime();
time_t   syncTimeFromRTC();  // Déclaration de la nouvelle fonction

extern RTC_DS3231 rtc;

#endif
