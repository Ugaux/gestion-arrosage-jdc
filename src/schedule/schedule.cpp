#include "schedule.h"

#include "watering/watering.h"

Schedule::Schedule(const char *fileName) : SPIFFSIniFile(fileName) {
  m_fileName = fileName;
}

// read the schedule configuration file
bool Schedule::read(void) {
  if (!open()) {
    Serial.printf("%s: not found\n", m_fileName);
    return false;
  }
  Way *way = Way::getFirst();
  while (way != 0) {
    Serial.printf("schedule::read: %s\n", way->getName());
    for (int i = 0; i < MAX_SCHEDULE; i++) {
      char name[MAX_DEF];
      char buffer[MAX_LINE];
      snprintf(name, MAX_DEF, "schedule%d", i + 1);
      if (getValue(way->getName(), name, buffer, sizeof(buffer)) != true) {
        break;
      }
      Serial.printf("schedule::read: %s: %s\n", way->getName(), buffer);
      Watering::create(i, way, buffer);
    }
    way = Way::getNext();
  }
  return true;
}

// write the schedule configuration file
bool Schedule::write(void) {
  if (!open()) {
    Serial.printf("%s: not found\n", m_fileName);
    return false;
  }
  close();
  File file = SPIFFS.open(SCHEDULE_FILE, FILE_WRITE);
  if (!file) {
    Serial.println("ERROR opening the file for writing");
    return false;
  }
  Way *way = Way::getFirst();
  while (way != 0 && way->getName() != 0) {
    Serial.printf("Schedule::write: %s\n", way->getName());
    file.printf("\n[%s]\n", way->getName());
    for (int i = 0; i < MAX_SCHEDULE; i++) {
      Watering *w = Watering::getByName(way->getName(), i);
      if (w != 0 && w->getDuration() != 0)
        file.printf("schedule%d=%02d:%02d,%ld,%d,%d\n", i + 1, w->getHour(), w->getMinute(), w->getDuration(), w->always(), w->unJourSurDeux());
    }
    way = Way::getNext();
  }
  file.close();
  return true;
}

// print the schedule data
void Schedule::print(void) {
  Serial.printf("watering: %d/%d\n", Watering::getCount(), MAX_WATERING);
  Watering *w = Watering::getFirst();
  while (w != 0) {
    if (w->getWay() != 0) {
      w->print();
    }
    w = Watering::getNext();
  }
}
