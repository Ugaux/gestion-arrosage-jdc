#ifndef _ZONE_H_
#define _ZONE_H_

#include "constants.h"

class Zone {
public:
  static bool create(const char *def);
  static int  getCount();
  Zone(void) { m_name = ""; };
  static Zone *getFirst(void);
  static Zone *getNext(void);
  const char  *getName(void) { return m_name.c_str(); }
  void         print(void);
  static Zone  m_zone[MAX_ZONE];

private:
  static int m_searchIndex;
  String     m_name;
};

#endif
