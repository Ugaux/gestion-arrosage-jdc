#include "watering.h"

#include <TimeLib.h>
#include <RCSwitch.h>

#include "schedule/schedule.h"
#include "valve/valve.h"
#include "sensors/sensors.h"
#include "preferences/preferences.h"

#define DEBUG true

int      Watering::m_searchIndex;
int      Watering::m_manualDuration;
Watering Watering::m_watering[MAX_WATERING];

Watering::Watering() : m_way(0),
                       m_hour(0), m_minute(0),
                       m_duration(0),
                       m_enable(true),
                       m_onlyIfDrySoil(false),
                       m_everyDays(false), m_evenDays(false), m_oddDays(false), m_customDays(false),
                       m_wateringDays{ "", "", "", "", "", "", "" },
                       m_autoStarted(0),
                       m_moisture(0) {
}

// Create a watering for a given way
// Example for "schedule1=07:00,15,*" def="07:00,15,*"
bool Watering::create(int index, Way *way, const char *def) {
  char        tmp[MAX_DEF];
  Watering   *w;
  const char *p;
  char       *s;

  if (DEBUG)
    Serial.printf("watering::create %s %s\n", way->getName(), def);
  strncpy(tmp, def, MAX_DEF);
  w = getByName(way->getName(), index);
  if (w) {
    w->m_index = index;

    w->m_way = way;

    // Get hours
    p = strtok_r(tmp, ":", &s);
    if (p == NULL) {
      if (DEBUG)
        Serial.printf("%s: bad format, missing ':'\n", def);
      return false;
    }
    w->m_hour = atoi(p);

    // Get minutes
    p = strtok_r(NULL, ",", &s);
    if (p == NULL) {
      if (DEBUG)
        Serial.printf("%s: bad format, missing ','\n", def);
      return false;
    }
    w->m_minute = atoi(p);

    // Get duration
    p = strtok_r(NULL, ",", &s);
    if (p == NULL) {
      if (DEBUG)
        Serial.printf("%s: bad format, missing ','\n", def);
      return false;
    }
    w->m_duration = atol(p);

    // Get humidity bypass
    p                  = strtok_r(NULL, ",", &s);
    w->m_onlyIfDrySoil = false;
    if (p != NULL && *p == '1')
      w->m_onlyIfDrySoil = true;  // watering is independant of moisture

    // Frequency watering
    p               = strtok_r(NULL, ",", &s);
    w->m_everyDays  = false;
    w->m_evenDays   = false;
    w->m_oddDays    = false;
    w->m_customDays = false;
    if (p != NULL && *p == '*')
      w->m_everyDays = true;  // Every days watering
    if (p != NULL && *p == 'e')
      w->m_evenDays = true;  // Even days watering (2,4,...)
    if (p != NULL && *p == 'o')
      w->m_oddDays = true;  // Odd days watering (1,3,...)
    if (p != NULL && *p == 'c')
      w->m_customDays = true;  // Custom days watering

    // Watering days for custom days
    p     = strtok_r(NULL, "(- )", &s);
    int i = 0;
    while (p != NULL) {
      strcpy(w->m_wateringDays[i], p);
      p = strtok_r(NULL, " -)", &s);
      i++;
    }
  }
  return true;
}

// Return the number of waterings
int Watering::getCount() {
  int n = 0;
  int i;

  for (i = 0; i < MAX_WATERING; i++) {
    if (m_watering[n].m_way != 0) {
      n++;
    }
  }
  return n;
}

// Return the first watering
Watering *Watering::getFirst() {
  m_searchIndex = 0;
  return &m_watering[m_searchIndex];
}

// Return the next watering
Watering *Watering::getNext() {
  m_searchIndex++;
  if (m_searchIndex < MAX_WATERING && m_watering[m_searchIndex].m_way != 0) {
    return &m_watering[m_searchIndex];
  }
  return 0;
}

// Return a watering giving its name
Watering *Watering::getByName(const char *def, int index) {
  if (DEBUG)
    Serial.printf("watering::getByName %s %d\n", def, index);
  for (int i = 0; i < MAX_WATERING; i++) {
    Watering *w = &m_watering[i];
    if (w->m_way == 0) {
      return 0;
    }
    if (!strcmp(w->getWayName(), def) && w->m_index == index) {
      if (DEBUG)
        Serial.printf("watering::getByName found %s %d at %d\n", w->getWayName(), w->m_index, i);
      return w;
    }
  }
  return 0;
}

// Return the first free watering
Watering *Watering::getFreeWatering() {
  if (DEBUG)
    Serial.printf("watering::getFreeWatering\n");
  for (int i = 0; i < MAX_WATERING; i++) {
    Watering *w = &m_watering[i];
    if (w->m_way == 0) {
      if (DEBUG)
        Serial.printf("watering::getFreeWatering found %d\n", i);
      return w;
    }
  }
  return 0;
}

// Return a free watering for a way
Watering *Watering::getFreeWatering(const char *wayName) {
  if (DEBUG)
    Serial.printf("watering::getFreeWatering %s\n", wayName);
  for (int i = 0; i < MAX_WATERING; i++) {
    Watering *w = &m_watering[i];
    if (w->m_way == 0) {
      return 0;
    }
    if (!strcmp(w->getWayName(), wayName)) {
      if (w->m_duration == 0) {
        if (DEBUG)
          Serial.printf("watering::getFreeWatering found %d\n", i);
        return w;
      }
    }
  }
  return 0;
}

// Run the watering
bool Watering::run(time_t t) {
  struct tm *pTime;

  if (DEBUG)
    Serial.println();
  pTime         = localtime(&t);
  int  toDayIdx = pTime->tm_mday;  // Month day 1 -> 31
  char toDayName[MAX_BUF];
  strftime(toDayName, MAX_BUF, "%a", pTime);  // Examples: "Mon", "Sun"

  for (int i = 0; i < MAX_WATERING; i++) {
    Watering *w = &m_watering[i];
    if (w->getDuration() != 0) {
      time_t stopTime   = w->getStopTime(t);
      int    adjustment = prefManager.getInt("season_adj", 100);                  // Updated for seasonal adjustement function
      time_t startTime  = stopTime - (w->getDuration() * 60 * adjustment) / 100;  // Updated for seasonal adjustement function
      if (DEBUG) {
        char buffer[MAX_BUF];
        pTime = localtime(&startTime);
        strftime(buffer, MAX_BUF, "%d/%m/%Y %H:%M:%S", pTime);
        Serial.printf("%s: next start %s\n", w->getWayName(), buffer);
        pTime = localtime(&stopTime);
        strftime(buffer, MAX_BUF, "%d/%m/%Y %H:%M:%S", pTime);
        Serial.printf("%s next stop %s\n", w->getWayName(), buffer);
      }

      bool shouldWaterToday = w->m_everyDays or (w->m_evenDays and toDayIdx % 2 == 0) or (w->m_oddDays and toDayIdx % 2 != 0) or (w->m_customDays and w->isCustomWateringDay(toDayName));

      if (w->getEnable() and shouldWaterToday and (startTime <= t and t <= stopTime)) {
        // Start watering
        if (!w->autoStarted()) {
          if (DEBUG)
            Serial.printf("In time, open relay %s\n", w->m_way->getRelay()->getName());
          w->autoStart();
        }

      } else {
        if (w->autoStarted()) w->autoStop();
      }
    }
  }
  if (isAnyWateringRunning() == false) {
    Valve::getMainValve()->close();
  } else {
    Valve::getMainValve()->open();
  }
  return true;
}

const char *Watering::getNextWateringTime(Watering **watering, time_t *t) {
  struct tm *pTime;
  time_t     timestamp = now();
  pTime                = localtime(&timestamp);

  int toDay = pTime->tm_wday;  // Today week day 0->6 (sunday = 0)

  time_t      nextWateringTime = 0;
  const char *nextWateringWay  = 0;
  Watering   *w                = 0;

  for (int i = 0; i < MAX_WATERING; i++) {
    w = &m_watering[i];
    if (w->getDuration() != 0 && w->getEnable()) {
      time_t startTime = w->getStartTime(timestamp);
      pTime            = localtime(&startTime);
      if (DEBUG) {
        char buffer[MAX_BUF];
        strftime(buffer, MAX_BUF, "%d/%m/%Y %H:%M:%S", pTime);
        Serial.printf("%s: next start %s\n", w->getWayName(), buffer);
      }
      int startDay     = pTime->tm_mday;  // Start day 1->31
      int startWeekDay = pTime->tm_wday;  // Start week day 0->6

      if (w->m_customDays) {
        // Get closest day
        int closestDay = w->getClosestDay(w, toDay);

        // Day offset
        if (startWeekDay > toDay) {
          startTime += DAY_DURATION * (closestDay - startWeekDay);
          if (closestDay == toDay) {
            if (startTime < timestamp) {
              startTime += DAY_DURATION * 7;
            } else {
              startTime += DAY_DURATION;
            }
          }
        }
        if (startWeekDay < toDay) {
          startTime += DAY_DURATION * (closestDay % 7 - startWeekDay);
          if (closestDay == toDay) {
            if (startTime > timestamp) {
              startTime += DAY_DURATION;
            } else {
              startTime += DAY_DURATION * 7;
            }
          }
        }
        if (startWeekDay == toDay) {
          startTime += DAY_DURATION * (closestDay - startWeekDay);
        }
      }

      if (w->m_evenDays && startDay % 2 != 0) {
        startTime = startTime + DAY_DURATION;
      }

      if (w->m_oddDays && startDay % 2 == 0) {
        startTime = startTime + DAY_DURATION;
      }

      if (nextWateringTime == 0 or startTime < nextWateringTime) {
        nextWateringTime = startTime;
        nextWateringWay  = w->getWayName();
        *watering        = &m_watering[i];
      }
    }
  }
  *t = nextWateringTime;
  if (DEBUG)
    Serial.printf("nextWateringTime = %ld\n", nextWateringTime);
  if (DEBUG)
    Serial.printf("Watering::getNextWateringTime found %s %ld\n", nextWateringWay == 0 ? "none" : nextWateringWay, *t);
  return nextWateringWay;
}

/********** Get next watering time **********/
bool Watering::getNextWayWateringTime(String &s) {
  s = "";
  struct tm *pTime;
  time_t     timestamp                 = time(NULL);
  time_t     nextWateringTime[MAX_WAY] = { 0 };

  pTime     = localtime(&timestamp);
  int toDay = pTime->tm_wday;  // Today week day 0->6 (sunday = 0)

  const char *nextWateringWay[MAX_WAY] = { 0 };
  int         n                        = 0;
  char        buffer[MAX_BUF];
  bool        status = false;
  Watering   *w      = 0;

  // Get next watering time for each way into an array
  Way *way = Way::getFirst();
  while (way != 0) {
    for (int i = 0; i < MAX_SCHEDULE; i++) {
      w = Watering::getByName(way->getName(), i);
      if (w->getDuration() != 0 && w->getEnable()) {
        time_t startTime = w->getStartTime(timestamp);
        pTime            = localtime(&startTime);
        int startDay     = pTime->tm_mday;  // Start day 1->31
        int startWeekDay = pTime->tm_wday;  // Start week day 0->6

        if (w->m_customDays) {
          // Get closest day
          int closestDay = w->getClosestDay(w, toDay);

          // Serial.printf("way[%s] today: %d\n", w->getWayName(), toDay);
          // Serial.printf("way[%s] start: %d\n", w->getWayName(), startWeekDay);
          // Serial.printf("way[%s] closest: %d\n", w->getWayName(), closestDay);

          // Day offset
          if (startWeekDay > toDay) {
            Serial.printf("1\n");
            startTime += DAY_DURATION * (closestDay - startWeekDay);
            if (closestDay == toDay) {
              Serial.printf("1bis\n");
              if (startTime < timestamp) {
                Serial.printf("1a\n");
                startTime += DAY_DURATION * 7;
              } else {
                Serial.printf("1b\n");
                startTime += DAY_DURATION;
              }
            }
          }
          if (startWeekDay < toDay) {
            Serial.printf("2\n");
            startTime += DAY_DURATION * (closestDay % 7 - startWeekDay);
            if (closestDay == toDay) {
              Serial.printf("2bis\n");
              if (startTime > timestamp) {
                Serial.printf("2a\n");
                startTime += DAY_DURATION;
              } else {
                Serial.printf("2b\n");
                startTime += DAY_DURATION * 7;
              }
            }
          }
          if (startWeekDay == toDay) {
            Serial.printf("3\n");
            startTime += DAY_DURATION * (closestDay - startWeekDay);
          }
        }

        if (w->m_evenDays && startDay % 2 != 0) {
          startTime += DAY_DURATION;
        }

        if (w->m_oddDays && startDay % 2 == 0) {
          startTime += DAY_DURATION;
        }

        if (nextWateringTime[n] == 0) {
          nextWateringTime[n] = startTime;
          nextWateringWay[n]  = w->getWayName();
        }

        if (nextWateringTime[n] > startTime) {
          nextWateringTime[n] = startTime;
          nextWateringWay[n]  = w->getWayName();
        }
      } else {
        nextWateringWay[n] = w->getWayName();
      }
    }
    n++;
    way = Way::getNext();
  }

  // Concate array -> Lawn.Studio=Sun 05 May 24 08:00,Dripline.Flowers=Sat 04 May 24 15:00
  for (int i = 0; i < MAX_WAY; i++) {
    if (nextWateringWay[i] != 0) {
      if (s.length() > 0) {
        s += ",";
      }
      s += nextWateringWay[i];
      s += "=";
      if (nextWateringTime[i] != 0) {
        pTime = localtime(&nextWateringTime[i]);
        strftime(buffer, MAX_BUF, "%a %d %b %y %H:%M", pTime);
        s += "Next run scheduled on ";
        s += buffer;
      } else {
        s += "No irrigation(s) scheduled";
      }
      status = true;
    }
  }
  return status;
}

bool Watering::isAnyWateringRunning() {
  for (int i = 0; i < MAX_WATERING; i++) {
    Watering *w = &m_watering[i];
    if (w->getDuration() != 0) {
      if (w->autoStarted() || w->getWay()->manualStarted(NULL)) {
        if (DEBUG)
          Serial.printf("Watering running: %s\n", w->getWayName());
        return true;
      }
    }
  }
  return false;
}

/********** Check if any auto watering is running and return remaining time**********/
bool Watering::isAnyAutoWateringRunning(String &s) {
  s = "";
  char t[6];
  bool status = false;
  for (int i = 0; i < MAX_WATERING; i++) {
    Watering *w = &m_watering[i];
    if (w->getDuration() != 0) {
      time_t remain;
      time_t start = w->getStartTime(time(NULL));
      if (w->autoStarted(&start, &remain)) {
        // DEBUG Serial.printf("watering::isAnyAutoWateringRunning found %s\n", w->getWayName());
        if (s.length() > 0) {
          s += ",";
        }
        s += w->getWayName();
        s += "=";
        if (remain < 1) {
          remain = 0;
        }
        snprintf(t, 6, "%02ld:%02ld", remain / 60, remain % 60);
        s += t;
        status = true;
      }
    }
  }
  return status;
}

/********** Stop all auto waterings **********/
void Watering::stopAllAutoWatering() {
  for (int i = 0; i < MAX_WATERING; i++) {
    Watering *w = &m_watering[i];
    if (w->getDuration() != 0) {
      if (w->autoStarted()) {
        w->autoStop();
      }
    }
  }
}

const char *Watering::getHourString() {
  struct tm  *pTime;
  time_t      t = now();
  static char buffer[MAX_BUF];
  time_t      startTime = getStartTime(t);
  pTime                 = localtime(&startTime);
  strftime(buffer, MAX_BUF, "%H:%M", pTime);
  return buffer;
}

/********** Get frequency (always, odd, even, custom) **********/
String Watering::getFrequency() {
  if (m_everyDays) return "Every days";
  if (m_evenDays) return "Even days";
  if (m_oddDays) return "Odd days";

  String s = "";
  if (m_customDays) {
    for (int i = 0; i < DAY_WEEK; i++) {
      if (strlen(m_wateringDays[i]) != 0) {
        if (s.length() > 0) {
          s += ", ";
        }
        s += m_wateringDays[i];
      }
    }
    return s;
  }
  return "Error";
}

/********** Get closest day for custom days **********/
int Watering::getClosestDay(Watering *w, int today) {
  int days[7] = { -1, -1, -1, -1, -1, -1, -1 };

  // Convert string array to int array
  for (int n = 0; n < DAY_WEEK; n++) {
    if (strcmp(w->m_wateringDays[n], "Sun") == 0 && strlen(w->m_wateringDays[n]) != 0) days[n] = 0;
    if (strcmp(w->m_wateringDays[n], "Mon") == 0 && strlen(w->m_wateringDays[n]) != 0) days[n] = 1;
    if (strcmp(w->m_wateringDays[n], "Tue") == 0 && strlen(w->m_wateringDays[n]) != 0) days[n] = 2;
    if (strcmp(w->m_wateringDays[n], "Wed") == 0 && strlen(w->m_wateringDays[n]) != 0) days[n] = 3;
    if (strcmp(w->m_wateringDays[n], "Thu") == 0 && strlen(w->m_wateringDays[n]) != 0) days[n] = 4;
    if (strcmp(w->m_wateringDays[n], "Fri") == 0 && strlen(w->m_wateringDays[n]) != 0) days[n] = 5;
    if (strcmp(w->m_wateringDays[n], "Sat") == 0 && strlen(w->m_wateringDays[n]) != 0) days[n] = 6;
  }

  // Find the closest day from today
  int minDifference = INT_MAX;
  int closestDay;
  for (int n = 0; n < DAY_WEEK; n++) {
    if (days[n] != -1) {
      if (days[n] < today) days[n] += 7;
      int difference = abs(today - days[n]);
      if (difference < minDifference) {
        minDifference = difference;
        closestDay    = days[n];
      }
    }
  }

  return closestDay;
}

// return the watering's start time
time_t Watering::getStartTime(time_t now) {
  struct tm *pTime;

  pTime = localtime(&now);
  //  Serial.printf("Current hour: %d, Current minute: %d\n", pTime->tm_hour, pTime->tm_min);
  pTime->tm_hour = m_hour;
  pTime->tm_min  = m_minute;
  pTime->tm_sec  = 0;
  time_t at      = mktime(pTime);
  // SANS LE + (m_duration * 60)
  if (at + (m_duration * 60) < now) {
    at += DAY_DURATION;
  }
  //  Serial.printf("Start time: %ld\n", at);
  return at;
}

// Return the watering's stop time
time_t Watering::getStopTime(time_t now) {
  struct tm *pTime;

  pTime                            = localtime(&now);
  int adjustment                   = prefManager.getInt("season_adj", 100);     // Updated for seasonal adjustement function
  int adjusted_duration_total_secs = (m_duration * 60 * adjustment) / 100;      // Convert m_duration from mins to secs
  int adjusted_duration_hours      = adjusted_duration_total_secs / 3600;       // get m_duration -> hour
  int adjusted_duration_mins       = (adjusted_duration_total_secs / 60) % 60;  // get m_duration -> mins
  int adjusted_duration_secs       = adjusted_duration_total_secs % 60;         // get m_duration -> secs

  pTime->tm_hour = m_hour + adjusted_duration_hours;
  pTime->tm_min  = m_minute + adjusted_duration_mins;
  pTime->tm_sec  = adjusted_duration_secs;

  time_t at = mktime(pTime);
  // SANS LE + 10
  if (at + 10 < now) {
    at += DAY_DURATION;
  }
  return at;
}

// Helper function to check if a specific day is a custom watering day
bool Watering::isCustomWateringDay(const char *dayName) {
  for (int n = 0; n < DAY_WEEK; n++) {
    if (strcmp(dayName, m_wateringDays[n]) == 0) {
      return true;
    }
  }

  return false;
}

// Exemple de fonction autoStart avec des journaux de débogage
void Watering::autoStart() {
  int moisture;

  if (DEBUG)
    Serial.printf("Watering::autoStart %s: %ld\n\n", getWayName(), m_duration);
  m_autoStarted = now();

  if (m_onlyIfDrySoil) {
    m_moisture = getSoilMoisture(&moisture);
    if (m_moisture == HUMIDITY_DRY) {
      if (DEBUG)
        Serial.printf("Watering::autoStart way opened with DRY soil (moisture %x%%)\n", moisture);
      m_way->open();
    } else if (m_moisture == HUMIDITY_WET) {
      if (DEBUG)
        Serial.printf("Watering::autoStart way opening skipped because of WET soil (moisture %x%%)\n", moisture);
    }
  } else {
    m_way->open();
    if (DEBUG)
      Serial.printf("Watering::autoStart way opened even with WET soil (moisture %x%%)\n", moisture);
  }
}

void Watering::autoStop() {
  if (DEBUG)
    Serial.printf("Watering::autoStop %s\n\n", getWayName());
  m_autoStarted = 0;
  if (!m_way->manualStarted(NULL)) {
    if (DEBUG)
      Serial.printf("close relay %s\n", m_way->getRelay()->getName());
    m_way->close();
  } else {
    if (DEBUG)
      Serial.printf("Relay %s remains open (manual start detected)\n", m_way->getRelay()->getName());
  }
}

bool Watering::autoStarted() {
  if (m_autoStarted != 0) {
    if (DEBUG)
      Serial.printf("watering::autoStarted: %s\n", getWayName());
    return true;
  }
  return false;
}

bool Watering::autoStarted(time_t *start, time_t *remain) {
  if (m_autoStarted != 0) {
    if (DEBUG)
      Serial.printf("watering::autoStarted: %s\n", getWayName());
    if (remain) {
      time_t elapsed    = now() - (*start - DAY_DURATION);
      int    adjustment = prefManager.getInt("season_adj", 100);             // Updated for seasonal adjustement function
      *remain           = ((m_duration * 60 * adjustment) / 100) - elapsed;  // Updated for seasonal adjustement function
    }
    return true;
  }
  return false;
}

void Watering::set(const char *wayName, int index) {
  if (DEBUG)
    Serial.printf("Watering::set %s[%d]\n", wayName, index);
  Way *way = Way::getByName(wayName);
  m_way    = way;
  m_index  = index;
}

//Set watering : hour, minute, duration, frequency, custom days
void Watering::set(int hour, int minute, long duration, bool onlyIfDrySoil, const char *mode, char days[7][4]) {  // UPDATE (mode -> *, e, o, c)
  if (DEBUG)
    Serial.printf("Watering::set %s[%d] %d:%d %ld %s (only if dry:%d)\n", getWayName(), m_index, hour, minute, duration, mode, onlyIfDrySoil);
  m_hour          = hour;
  m_minute        = minute;
  m_duration      = duration;
  m_onlyIfDrySoil = onlyIfDrySoil;

  m_everyDays  = !strcmp(mode, "*") ? true : false;
  m_evenDays   = !strcmp(mode, "e") ? true : false;
  m_oddDays    = !strcmp(mode, "o") ? true : false;
  m_customDays = !strcmp(mode, "c") ? true : false;

  if (!strcmp(mode, "c")) {
    for (int i = 0; i < DAY_WEEK; i++) {
      strcpy(m_wateringDays[i], days[i]);
    }
  } else {
    for (int i = 0; i < DAY_WEEK; i++) {
      strcpy(m_wateringDays[i], "");
    }
  }

  schedule.write();
}

void Watering::set(const char *wayName, int index, int hour, int minute, long duration) {
  if (DEBUG)
    Serial.printf("Watering::set %s[%d] %d:%d %ld\n", wayName, index, hour, minute, duration);
  Way *way        = Way::getByName(wayName);
  m_way           = way;
  m_index         = index;
  m_hour          = hour;
  m_minute        = minute;
  m_duration      = duration;
  m_onlyIfDrySoil = false;
  m_everyDays     = true;
  m_evenDays      = false;
  m_oddDays       = false;
  m_customDays    = false;
  for (int i = 0; i < DAY_WEEK; i++) {
    strcpy(m_wateringDays[i], "");
  }

  schedule.write();
}

// Print the watering
void Watering::print() {
  Serial.printf("index:%d searchIndex:%d way:%s at %02d:%02d for %ld minutes, %s, (%s)\n", m_index, m_searchIndex, m_way->getName(), m_hour, m_minute, m_duration, m_onlyIfDrySoil == true ? "Only if soil is dry" : "Even if soil is wet", getFrequency());
}

void Watering::resetTimerAllumagePompe(RCSwitch &radioCmd) {
  if (DEBUG)
    Serial.println("resetTimerAllumagePompe");
  unsigned long sendTime = millis();
  while (millis() - sendTime < 10) {
    radioCmd.send("101000000110101010110100");  // = BOUTON "C" télécommande : Arrosage au jet
    if (DEBUG)
      delay(1);
  }
}
