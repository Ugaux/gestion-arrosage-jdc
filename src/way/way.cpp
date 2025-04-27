#include <Arduino.h>
#include <TimeLib.h>

#include "../constants.h"
#include "../way/way.h"
#include "../watering/watering.h"
#include "../config/config.h"
#include "../RTCModule/RTCModule.h"

int Way::m_searchIndex;
Way Way::m_way[MAX_WAY];

Way::Way() : m_zone(0),
             m_relay(0),
             m_manualStarted(0),
             m_manualDuration(0)

{
  memset(watering, 0, MAX_SCHEDULE * sizeof(Watering *));
};

// timer callback
void changeState(Way *w) {
  w->manualStop();
  w->getTimer()->detach();
}

// create a way in a zone
// def : way description string, "way1(mcp23017-1.0)" for example:
bool Way::create(Zone *z, const char *def) {
  char        tmp[MAX_DEF];
  Way        *w;
  const char *p;
  char       *s;

  Serial.printf("way::create %s\n", def);
  strncpy(tmp, def, MAX_DEF);
  w         = &m_way[m_searchIndex];
  w->m_zone = z;
  p         = strtok_r(tmp, "(", &s);
  if (p == NULL) {
    Serial.printf("%s: bad value\n", def);
    return false;
  }
  w->m_name = p;
  w->m_def  = z->getName();
  w->m_def += '.';
  w->m_def += p;
  p = strtok_r(NULL, ")", &s);
  if (p == NULL) {
    Serial.printf("%s: bad format, missing parenthesis\n", def);
    return false;
  }
  w->m_relay = Relay::getByName(p);
  if (w->m_relay == 0) {
    Serial.printf("%s: not found\n", p);
    return false;
  }
  Watering *wt = Watering::getFreeWatering();
  for (int i = 0; i < MAX_SCHEDULE; i++) {
    wt->set(w->m_def.c_str(), i);
    wt++;
  }
  m_searchIndex++;
  return true;
}

// return the number of ways
int Way::getCount() {
  int n;

  for (n = 0; n < MAX_WAY && m_way[n].m_name != 0; n++);
  return n;
}

// return the first way
Way *Way::getFirst(void) {
  m_searchIndex = 0;
  return &m_way[m_searchIndex];
}

// return the next way
Way *Way::getNext(void) {
  m_searchIndex++;
  if (m_searchIndex < MAX_WAY && m_way[m_searchIndex].m_name != "") {
    return &m_way[m_searchIndex];
  }
  return 0;
}

bool Way::isAnyManualWateringRunning(String &s) {
  s = "";
  char t[6];
  bool status = false;
  for (int i = 0; i < MAX_WAY; i++) {
    Way   *w = &m_way[i];
    time_t remain;
    if (w->manualStarted(&remain)) {
      Serial.printf("way::isAnyManualWateringRunning found %s\n", w->getName());
      if (s.length() > 0) {
        s += ",";
      }
      s += w->getName();
      s += "=";
      if (remain < 2) {
        remain = 0;
      }
      snprintf(t, 6, "%02ld:%02ld", remain / 60, remain % 60);
      s += t;
      status = true;
    }
  }
  return status;
}

void Way::stopAllManualWatering(void) {
  for (int i = 0; i < MAX_WAY; i++) {
    Way   *w = &m_way[i];
    time_t remain;
    if (w->manualStarted(&remain)) {
      Serial.printf("way::stopAllManual found %s\n", w->getName());
      w->manualStop();
    }
  }
}

// return a way giving its name
Way *Way::getByName(const char *def) {
  Way *way = Way::getFirst();
  while (way != 0) {
    if (!strcmp(way->getName(), def)) {
      Serial.printf("way::getByName found %s\n", def);
      return way;
    }
    way = Way::getNext();
  }
  return 0;
}

// return the name of the way
const char *Way::getName(void) {
  return m_def.c_str();
}

// print the way
void Way::print(void) {
  Serial.printf("%d: %s: %s\n", m_searchIndex, getName(), m_relay->getName());
}

void Way::manualStart(int duration) {
  m_manualStarted = now();
  char       timeString[MAX_BUF];
  struct tm *timeinfo = localtime(&m_manualStarted);
  strftime(timeString, MAX_BUF, "%d/%m/%Y %H:%M", timeinfo);
  Serial.printf("way::manualStart %s at %s for %dmin\n", getName(), timeString, duration);

  m_manualDuration = duration * 60;
  open();
  Valve::getMainValve()->open();
  m_timer.attach(duration * 60, changeState, this);
}

void Way::manualStop(void) {
  Serial.printf("way::manualStop %s\n", getName());
  m_manualStarted  = 0;
  m_manualDuration = 0;
  close();
  if (Watering::isAnyWateringRunning() == false) {
    Valve::getMainValve()->close();
  }
  m_timer.detach();
}

bool Way::manualStarted(time_t *remain) {
  if (m_manualStarted != 0) {
    Serial.printf("way::manualStarted: %s\n", getName());
    if (remain) {
      time_t elapsed = now() - m_manualStarted;
      *remain        = m_manualDuration - elapsed;
      // Serial.printf("way::manualStarted: t %ld s %ld e %ld r %ld\n", time(NULL), m_manualStarted, elapsed, *remain);
    }
    return true;
  }
  return false;
}
