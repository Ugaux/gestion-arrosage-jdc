#ifndef _WATERING_H_
#define _WATERING_H_

#include "way/way.h"

#include <RCSwitch.h>

#define DAY_DURATION (24L * 60L * 60L)
#define DAY_WEEK     7

class Watering {
public:
  Watering();
  static int         getCount();
  static Watering   *getFirst();
  static Watering   *getNext();
  static Watering   *getWatering(int w) { return &m_watering[w]; }
  static Watering   *getByName(const char *def, int index);
  static Watering   *getFreeWatering();
  static Watering   *getFreeWatering(const char *wayName);
  static bool        create(int index, Way *way, const char *def);
  static bool        run(time_t t);
  static const char *getNextWateringTime(Watering **watering, time_t *t);
  static bool        getNextWayWateringTime(String &s);  // ADD new function
  static bool        isAnyWateringRunning();
  static bool        isAnyAutoWateringRunning(String &s);  // ADD new function
  static void        stopAllAutoWatering();
  static void        setManualDuration(int durationInSecs) { m_manualDuration = durationInSecs; }
  static int         getManualDuration() { return m_manualDuration; }
  long               getDuration() { return m_duration; };
  int                getHour() { return m_hour; };
  int                getMinute() { return m_minute; };
  int                getIndex() { return m_index; };
  Way               *getWay() { return m_way; };
  const char        *getWayName() { return m_way->getName(); };
  const char        *getHourString();
  String             getFrequency();                         // ADD get watering frequency (Every days, odd, even, custom)
  int                getClosestDay(Watering *w, int today);  // ADD get closest day for custom days
  time_t             getStartTime(time_t now);
  time_t             getStopTime(time_t now);
  char              *getWateringDay(int i) { return m_wateringDays[i]; };  // ADD get custom watering days
  bool               getEnable() { return m_enable; };                     // ADD get enable state
  void               toggleEnable() { m_enable = !m_enable; };             // ADD set enable state
  bool               onlyIfSoilDry() { return m_onlyIfDrySoil; };
  bool               everyDays() { return m_everyDays; };
  bool               evenDays() { return m_evenDays; };      // ADD get even days state
  bool               oddDays() { return m_oddDays; };        // ADD get odd days state
  bool               customDays() { return m_customDays; };  // ADD get custom days state
  bool               isCustomWateringDay(const char *dayName);
  void               autoStart();
  void               autoStop();
  bool               autoStarted();
  bool               autoStarted(time_t *start, time_t *remain);  // ADD overload to get remain time for autostart
  void               set(const char *wayName, int index);
  void               set(int hour, int minute, long duration, bool onlyIfDrySoil, const char *mode, char days[7][4]);  // UPDATE (mode -> *, e, o, c)
  void               set(const char *wayName, int index, int hour, int minute, long duration);
  void               print();
  static void        resetTimerAllumagePompe(RCSwitch &radioCmd);

private:
  static Watering m_watering[MAX_WATERING];
  static int      m_searchIndex;
  static int      m_manualDuration;
  int             m_index;
  Way            *m_way;
  int             m_hour;
  int             m_minute;
  long            m_duration;
  bool            m_enable;  // ADD enable and disable state
  bool            m_onlyIfDrySoil;
  bool            m_everyDays;                  // ADD even days
  bool            m_evenDays;                   // ADD even days
  bool            m_oddDays;                    // ADD odd days
  bool            m_customDays;                 // ADD custom days
  char            m_wateringDays[DAY_WEEK][4];  // ADD array -> scheduled watering days (string days)

  time_t  m_autoStarted;
  uint8_t m_moisture;
};

#endif
