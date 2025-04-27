#ifndef _WAY_H_
#define _WAY_H_

#include <Ticker.h>
#include <TimeLib.h>

#include "../zone/zone.h"
#include "../relay/relay.h"

class Watering;

class Way {
public:
  static bool create(Zone *z, const char *def);
  static int  getCount();
  Way();
  static Way *getFirst(void);
  static Way *getNext(void);
  static Way *getByName(const char *def);
  static bool isAnyManualWateringRunning(String &s);
  static void stopAllManualWatering(void);
  const char *getName(void);
  Relay      *getRelay(void) { return m_relay; }
  void        print(void);
  void        open(void) { m_relay->on(); }
  void        close(void) { m_relay->off(); }
  void        manualStart(int duration);
  void        manualStop(void);
  bool        manualStarted(time_t *remain);
  Ticker     *getTimer(void) { return &m_timer; }

private:
  static Way m_way[MAX_WAY];
  static int m_searchIndex;
  Zone      *m_zone;
  Relay     *m_relay;
  Watering  *watering[MAX_SCHEDULE];
  String     m_name;
  String     m_def;
  time_t     m_manualStarted;
  time_t     m_manualDuration;
  Ticker     m_timer;
};

#endif
