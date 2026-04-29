#ifndef _RTC_MODULE_H_
#define _RTC_MODULE_H_

#include <RTClib.h>

time_t syncTimeFromRTC();  // Déclaration de la nouvelle fonction

extern RTC_DS3231 rtc;

#endif
