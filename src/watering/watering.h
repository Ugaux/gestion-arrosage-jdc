#ifndef _WATERING_H_
#define _WATERING_H_

#include "way/way.h"

#define DAY_DURATION (24L * 60L * 60L)

class Watering {
public:
  Watering();

  static int         getCount();
  static Watering   *getFirst(void);
  static Watering   *getNext(void);
  static Watering   *getWatering(int w) { return &m_watering[w]; }
  static Watering   *getByName(const char *def, int index);
  static Watering   *getFreeWatering(void);
  static Watering   *getFreeWatering(const char *wayName);
  static bool        create(int index, Way *way, const char *def);
  static bool        run(time_t t);
  static const char *getNextWateringTime(time_t *t);
  static bool        isAnyWateringRunning(void);
  static void        stopAllAutoWatering(void);
  static void        manualDuration(int duration) { m_manualDuration = duration; }
  static int         manualDuration() { return m_manualDuration; }
  long               getDuration(void) { return m_duration; };
  int                getHour(void) { return m_hour; };
  int                getMinute(void) { return m_minute; };
  Way               *getWay(void) { return m_way; };
  const char        *getWayName(void) { return m_way->getName(); };
  const char        *getHourString(void);
  time_t             getStartTime(time_t now);
  time_t             getStopTime(time_t now);
  bool               always(void) { return m_always; };
  void               print(void);
  void               autoStart(void);
  void               autoStop(void);
  bool               autoStarted(void);
  void               set(const char *wayName, int index);
  void               set(int hour, int minute, long duration, bool always);
  void               set(const char *wayName, int index, int hour, int minute, long duration, bool always);

private:
  static Watering m_watering[MAX_WATERING];
  static int      m_searchIndex;
  static int      m_manualDuration;
  int             m_index;
  Way            *m_way;
  int             m_hour;
  int             m_minute;
  long            m_duration;
  bool            m_always;
  time_t          m_autoStarted;
  uint8_t         m_moisture;
};

#endif
