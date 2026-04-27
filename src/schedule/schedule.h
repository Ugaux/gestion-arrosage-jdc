#ifndef _SCHEDULE_H_
#define _SCHEDULE_H_

#include <SPIFFSIniFile.h>
// Supposed to be an Arduino library but latest working version was not released with a tag

#include <watering/watering.h>

#define SCHEDULE_FILE "/schedule/schedule.ini"

class Schedule : public SPIFFSIniFile {
public:
  Schedule(const char *fileName);
  bool read(void);
  bool write();
  void print(void);

private:
  String      formatFrequency(Watering *w);
  const char *m_fileName;
};

extern Schedule schedule;

#endif
