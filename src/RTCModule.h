#ifndef RTCMODULE_H
#define RTCMODULE_H

#include <Wire.h>
#include <RTClib.h>


extern RTC_DS3231 rtc;

void setupRTC();
DateTime getCurrentTime();
time_t syncTimeFromRTC(); // Déclaration de la nouvelle fonction

#endif
