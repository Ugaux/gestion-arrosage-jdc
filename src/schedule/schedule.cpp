#include "schedule.h"

#include <LittleFS.h>

#include "watering/watering.h"

Schedule::Schedule(const char *fileName) : SPIFFSIniFile(fileName) {
  m_fileName = fileName;
}

// Read the schedule configuration file
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
      if (not getValue(way->getName(), name, buffer, sizeof(buffer))) {
        break;
      }
      Serial.printf("schedule::read: %s: %s\n", way->getName(), buffer);
      Watering::create(i, way, buffer);
    }
    way = Way::getNext();
  }
  return true;
}

// Write the schedule configuration file
bool Schedule::write(void) {
  if (!open()) {
    Serial.printf("%s: not found\n", m_fileName);
    return false;
  }
  close();
  File file = LittleFS.open(SCHEDULE_FILE, FILE_WRITE);
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
      if (w != 0 && w->getDuration() != 0)  // TODO: if (w != 0) sans le w->getDuration() != 0 ??
        file.printf("schedule%d=%02d:%02d,%ld,%d,%i,%s\n", i + 1, w->getHour(), w->getMinute(), w->getDuration(), w->onlyIfSoilDry(), formatFrequency(w));
    }
    way = Way::getNext();
  }
  file.close();
  return true;
}

// Print the schedule data
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

// Format frequency (always "*"", odd "o", even "e", custom "c")
String Schedule::formatFrequency(Watering *w) {
  if (w->everyDays()) return "*";
  if (w->evenDays()) return "e";
  if (w->oddDays()) return "o";

  if (w->customDays()) {
    String s = "";
    for (int i = 0; i < DAY_WEEK; i++) {
      if (strlen(w->getWateringDay(i)) != 0) {
        if (s.length() > 0) {
          s += "-";
        }
        s += w->getWateringDay(i);
      }
    }
    return "c,(" + s + ")";
  }
  return "Error";
}
