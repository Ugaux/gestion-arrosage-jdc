#ifndef _SCHEDULE_H_
#define _SCHEDULE_H_

#include <SPIFFS.h>
#include <FS.h>

#include "SPIFFSIniFile.h"

#include "relay.h"
#include "zone.h"
#include "way.h"

#define SCHEDULE_FILE "/schedule.ini"

class Schedule : public SPIFFSIniFile {
public:
  Schedule(const char *fileName);
  bool read(void);
  bool write();
  void print(void);

private:
  const char *m_fileName;
};

extern Schedule schedule;

#endif
