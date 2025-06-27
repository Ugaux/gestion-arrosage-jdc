#include "watering.h"

#include <TimeLib.h>
#include <RCSwitch.h>

#include "schedule/schedule.h"
#include "valve/valve.h"
#include "humidity/humidity.h"

#define DEBUG false

int      Watering::m_searchIndex;
int      Watering::m_manualDuration;
Watering Watering::m_watering[MAX_WATERING];

Watering::Watering() : m_way(0),
                       m_hour(0), m_minute(0),
                       m_duration(0),
                       m_forceWateringWithWetSoil(false),
                       m_unJourSurDeux(false),
                       m_autoStarted(0),
                       m_moisture(0) {
}

// create a watering for a given way
// def : watering description string, "schedule1=07:00,15,*" for example:
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

    p = strtok_r(tmp, ":", &s);
    if (p == NULL) {
      if (DEBUG)
        Serial.printf("%s: bad format, missing ':'\n", def);
      return false;
    }
    w->m_hour = atoi(p);

    p = strtok_r(NULL, ",", &s);
    if (p == NULL) {
      if (DEBUG)
        Serial.printf("%s: bad format, missing ','\n", def);
      return false;
    }
    w->m_minute = atoi(p);

    p = strtok_r(NULL, ",", &s);
    if (p == NULL) {
      if (DEBUG)
        Serial.printf("%s: bad format, missing ','\n", def);
      return false;
    }
    w->m_duration = atol(p);

    p                             = strtok_r(NULL, ",", &s);
    w->m_forceWateringWithWetSoil = false;
    if (p != NULL && *p == '1') {
      // watering is independant of moisture
      w->m_forceWateringWithWetSoil = true;
    }

    p                  = strtok_r(NULL, ",", &s);
    w->m_unJourSurDeux = false;
    if (p != NULL && *p == '1') {
      // arrose 1 jour sur 2
      w->m_unJourSurDeux = true;
    }
  }
  return true;
}

// return the number of waterings
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

// return the first watering
Watering *Watering::getFirst(void) {
  m_searchIndex = 0;
  return &m_watering[m_searchIndex];
}

// return the next watering
Watering *Watering::getNext(void) {
  m_searchIndex++;
  if (m_searchIndex < MAX_WATERING && m_watering[m_searchIndex].m_way != 0) {
    return &m_watering[m_searchIndex];
  }
  return 0;
}

// return a watering giving its name
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

// return the first free watering
Watering *Watering::getFreeWatering(void) {
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

// return a free watering for a way
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

// run the watering
bool Watering::run(time_t t) {
  struct tm *pTime;

  if (DEBUG)
    Serial.println();
  pTime     = localtime(&t);
  int toDay = pTime->tm_mday;
  for (int i = 0; i < MAX_WATERING; i++) {
    Watering *w = &m_watering[i];
    if (w->getDuration() != 0) {
      char   buffer[MAX_BUF];
      time_t startTime = w->getStartTime(t);
      pTime            = localtime(&startTime);
      strftime(buffer, MAX_BUF, "%d/%m/%Y %H:%M:%S", pTime);
      if (DEBUG)
        Serial.printf("%s: next start %s\n", w->getWayName(), buffer);
      time_t stopTime = w->getStopTime(t);
      pTime           = localtime(&stopTime);
      strftime(buffer, MAX_BUF, "%d/%m/%Y %H:%M:%S", pTime);
      if (DEBUG)
        Serial.printf("%s next stop %s\n", w->getWayName(), buffer);
      if (t < startTime) {
        time_t d = startTime - t;
        int    h = d / 3600;
        d %= 3600;
        int m = d / 60;
        d %= 60;
        int s = d;
        if (DEBUG) {
          if (toDay != pTime->tm_mday) {
            Serial.printf("%s: after time, schedule in %02d:%02d:%02d\n", w->getWayName(), h, m, s);
            //          w->autoStop();
          } else {
            Serial.printf("%s: before time, schedule in %02d:%02d:%02d\n", w->getWayName(), h, m, s);
          }
        }
      }
      if (toDay == pTime->tm_mday && t >= startTime && t <= stopTime) {
        if (DEBUG)
          Serial.printf("In time, open relay %s\n", w->m_way->getRelay()->getName());
        w->autoStart();
      }
      if (toDay == pTime->tm_mday && t > stopTime) {
        w->autoStop();
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

const char *Watering::getNextWateringTime(time_t *t) {
  struct tm  *pTime;
  time_t      timestamp        = now();
  time_t      nextWateringTime = 0;
  const char *nextWateringWay  = 0;
  Watering   *w                = 0;

  pTime = localtime(&timestamp);
  for (int i = 0; i < MAX_WATERING; i++) {
    w = &m_watering[i];
    if (w->getDuration() != 0) {
      char   buffer[MAX_BUF];
      time_t startTime = w->getStartTime(timestamp);
      pTime            = localtime(&startTime);
      strftime(buffer, MAX_BUF, "%d/%m/%Y %H:%M:%S", pTime);
      if (DEBUG)
        Serial.printf("%s: next start %s\n", w->getWayName(), buffer);
      if (nextWateringTime == 0) {
        nextWateringTime = startTime;
        nextWateringWay  = w->getWayName();
      }
      if (nextWateringTime > startTime) {
        nextWateringTime = startTime;
        nextWateringWay  = w->getWayName();
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

void Watering::stopAllAutoWatering(void) {
  for (int i = 0; i < MAX_WATERING; i++) {
    Watering *w = &m_watering[i];
    if (w->getDuration() != 0) {
      if (w->autoStarted()) {
        w->autoStop();
      }
    }
  }
}

const char *Watering::getHourString(void) {
  struct tm  *pTime;
  time_t      t = now();
  static char buffer[MAX_BUF];
  time_t      startTime = getStartTime(t);
  pTime                 = localtime(&startTime);
  strftime(buffer, MAX_BUF, "%H:%M", pTime);
  return buffer;
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
  if (at + (m_duration * 60) < now) {
    if (m_unJourSurDeux == true)
      at += DAY_DURATION * 2;
    else
      at += DAY_DURATION;
  }
  //  Serial.printf("Start time: %ld\n", at);
  return at;
}

// return the watering's stop time
time_t Watering::getStopTime(time_t now) {
  struct tm *pTime;

  pTime          = localtime(&now);
  pTime->tm_hour = m_hour + (m_duration / 60);
  pTime->tm_min  = m_minute + (m_duration % 60);
  pTime->tm_sec  = 0;
  time_t at      = mktime(pTime);
  if (at + 10 < now) {
    if (m_unJourSurDeux == true)
      at += DAY_DURATION * 2;
    else
      at += DAY_DURATION;
  }
  //  Serial.printf("Stop time: %ld\n", at);
  return at;
}

// print the watering
void Watering::print(void) {
  Serial.printf("%d %s %02d:%02d %ld minutes (%s, %s)\n", m_searchIndex, m_way->getName(), m_hour, m_minute, m_duration, m_forceWateringWithWetSoil == true ? "force if wet" : "skip if wet", m_unJourSurDeux == true ? "1 jour sur 2" : "tous les jours");
}

// Exemple de fonction autoStart avec des journaux de débogage
void Watering::autoStart() {
  int moisture;

  if (DEBUG)
    Serial.printf("Watering::autoStart %s: %ld\n\n", getWayName(), m_duration);
  m_autoStarted = now();
  m_moisture    = getSoilMoisture(&moisture);
  if (m_moisture == HUMIDITY_DRY) {
    if (DEBUG)
      Serial.printf("Watering::autoStart: DRY soil (moisture %x%%)\n", moisture);
    m_way->open();
  } else if (m_moisture == HUMIDITY_WET) {
    if (m_forceWateringWithWetSoil) {
      m_way->open();
      if (DEBUG)
        Serial.printf("Watering::autoStart: forced with WET soil (moisture %x%%)\n", moisture);
    } else {
      if (DEBUG)
        Serial.printf("Watering::autoStart: skipped with WET soil (moisture %x%%)\n", moisture);
    }
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

bool Watering::autoStarted(void) {
  if (m_autoStarted != 0) {
    if (DEBUG)
      Serial.printf("watering::autoStarted: %s\n", getWayName());
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

void Watering::set(int hour, int minute, long duration, bool forceWateringWithWetSoil, bool unJourSurDeux) {
  if (DEBUG)
    Serial.printf("Watering::set %s[%d] %dÂ %d %ld %d %d\n", getWayName(), m_index, hour, minute, duration, forceWateringWithWetSoil, unJourSurDeux);
  m_hour                     = hour;
  m_minute                   = minute;
  m_duration                 = duration;
  m_unJourSurDeux            = unJourSurDeux;
  m_forceWateringWithWetSoil = forceWateringWithWetSoil;
  schedule.write();
}

void Watering::set(const char *wayName, int index, int hour, int minute, long duration, bool forceWateringWithWetSoil, bool unJourSurDeux) {
  if (DEBUG)
    Serial.printf("Watering::set %s[%d] %dÂ %d %ld %d %d\n", wayName, index, hour, minute, duration, forceWateringWithWetSoil, unJourSurDeux);
  Way *way                   = Way::getByName(wayName);
  m_way                      = way;
  m_index                    = index;
  m_hour                     = hour;
  m_minute                   = minute;
  m_duration                 = duration;
  m_unJourSurDeux            = unJourSurDeux;
  m_forceWateringWithWetSoil = forceWateringWithWetSoil;
  schedule.write();
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
