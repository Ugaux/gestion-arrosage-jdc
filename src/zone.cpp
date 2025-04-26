
#include <Arduino.h>
#include "constants.h"
#include "zone.h"

int Zone::m_searchIndex;
Zone Zone::m_zone[MAX_ZONE];

// create a zone
// def : zone description string, "garden" for example:
bool Zone::create(const char *def)
{
  Zone *z;

  Serial.printf("zone::create %s\n", def);
  z = &m_zone[m_searchIndex];
  z->m_name = def;
  m_searchIndex++;
  return true;
}

// return the number of zones
int Zone::getCount()
{
  int n;

  for (n = 0 ; n < MAX_ZONE && m_zone[n].m_name != 0 ; n++);
  return n;
}

// return the first zone
Zone *Zone::getFirst(void)
{
  m_searchIndex = 0;
  return &m_zone[m_searchIndex];
}

// return the next zone
Zone *Zone::getNext(void)
{
  m_searchIndex++;
  if (m_searchIndex < MAX_ZONE && m_zone[m_searchIndex].m_name != "") {
    return &m_zone[m_searchIndex];
  }
  return 0;
}

// print the zone
void Zone::print(void)
{
  Serial.printf("%d: %s\n", m_searchIndex, getName());
}
