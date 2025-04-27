#ifndef _SCHEDULE_H_
#define _SCHEDULE_H_

#include "SPIFFSIniFile/SPIFFSIniFile.h"
// Supposed to be an Arduino library but latest working version was not released with a tag

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
