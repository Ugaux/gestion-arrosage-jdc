#ifndef _SCHEDULE_H_
#define _SCHEDULE_H_

#include <SPIFFSIniFile.h>
// Supposed to be an Arduino library but latest working version was not released with a tag

#include "core/Watering.h"

#define SCHEDULE_FILE "/config/schedule.ini"
#define MAX_SCHEDULE  4  // max waterings number in a day for each way

class IniSchedule : public SPIFFSIniFile {
public:
  IniSchedule(const char *fileName);
  bool read(void);
  bool write();
  void print(void);

private:
  String      formatFrequency(Watering *w);
  const char *m_fileName;
};

extern IniSchedule iniSchedule;

#endif
