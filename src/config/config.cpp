#include "config.h"

Config* Config::m_config;

Config::Config(const char* fileName) : SPIFFSIniFile(fileName) {
  m_fileName       = fileName;
  m_ssid[0]        = '\0';
  m_password[0]    = '\0';
  m_moistureSensor = 0;
  m_maxMoisture    = 0;
  m_config         = this;
}

bool Config::getModules(void) {
  char        modules[MAX_LINE];
  const char* p;
  char*       s;

  if (getValue("relays", "modules", modules, sizeof(modules)) != true) {
    Serial.printf("%s:relays:modules not found (error %d)\n", m_fileName,
                  getError());
    return false;
  }
  p = strtok_r(modules, ", ", &s);
  while (p != NULL) {
    Serial.printf("config::getModules %s\n", p);
    Module::create(p);
    p = strtok_r(NULL, ", ", &s);
  }
  return true;
}

bool Config::getRelays(void) {
  char        buffer[MAX_LINE];
  const char* p;
  char*       s;
  int         id;

  Module* mod = Module::getFirst();
  Serial.printf("config::getRelays %p\n", mod);
  while (mod != NULL && mod->isPresent()) {
    id = 0;
    Serial.printf("config::getRelays %s\n", mod->getName());
    if (getValue("relays", mod->getName(), buffer, sizeof(buffer)) != true) {
      Serial.printf("%s:relays: %s not found (error %d)\n", m_fileName,
                    mod->getName(), getError());
    } else {
      Serial.printf("%s:relays: %s\n", m_fileName, buffer);
    }
    p = strtok_r(buffer, ", ", &s);
    while (p != NULL) {
      Serial.printf("config::getRelays %s %s\n", mod->getName(), p);
      if (Relay::create(mod, id, p) != true) {
        Serial.printf("%s: error creating relay\n", p);
        return false;
      }
      p = strtok_r(NULL, ", ", &s);
      id++;
    }
    p   = strtok_r(NULL, ", ", &s);
    mod = Module::getNext();
  }
  return true;
}

bool Config::getZones(void) {
  char        zones[MAX_LINE];
  const char* p;
  char*       s;

  Serial.printf("config::getZones\n");
  if (getValue("zones", "zones", zones, sizeof(zones)) != true) {
    Serial.printf("%s:zones:zones not found (error %d)\n", m_fileName,
                  getError());
    return false;
  }
  p = strtok_r(zones, ", ", &s);
  while (p != NULL) {
    Serial.printf("config::getZones %s\n", p);
    Zone::create(p);
    p = strtok_r(NULL, ", ", &s);
  }
  return true;
}

bool Config::getWays(void) {
  char        buffer[MAX_LINE];
  const char* p;
  char*       s;
  int         id;

  Zone* z = Zone::getFirst();
  Serial.printf("config::getWays %p\n", z);
  while (z != NULL && strlen(z->getName()) != 0) {
    id = 0;
    Serial.printf("config::getWays %s\n", z->getName());
    if (getValue(z->getName(), "ways", buffer, sizeof(buffer)) != true) {
      Serial.printf("%s:ways: %s not found (error %d)\n", m_fileName,
                    "ways", getError());
    } else {
      Serial.printf("%s:ways: %s\n", m_fileName, buffer);
    }
    p = strtok_r(buffer, ", ", &s);
    while (p != NULL) {
      Serial.printf("config::getWays %s %s\n", z->getName(), p);
      if (Way::create(z, p) != true) {
        Serial.printf("%s: error creating way\n", p);
        return false;
      }
      p = strtok_r(NULL, ", ", &s);
      id++;
    }
    p = strtok_r(NULL, ", ", &s);
    z = Zone::getNext();
  }
  return true;
}

bool Config::read(void) {
  char        buffer[MAX_LINE];
  const char* p;
  char*       s;

  if (!open()) {
    Serial.printf("%s: not found\n", m_fileName);
    return false;
  }
  if (getValue("WIFI", "access-point", buffer, sizeof(buffer)) != true) {
    Serial.printf("%s:WIFI:access-point not found (error %d)\n", m_fileName,
                  getError());
    return false;
  }
  p = strtok_r(buffer, ":", &s);
  if (p == NULL) {
    Serial.printf("%s: bad format, missing ':'\n", buffer);
    return false;
  }
  strcpy(m_ssid, p);
  p = strtok_r(NULL, ":", &s);
  if (p == NULL) {
    Serial.printf("%s: missing password\n", buffer);
    return false;
  }
  strcpy(m_password, p);
  if (getModules() != true) {
    return false;
  }
  if (getRelays() != true) {
    return false;
  }
  if (getZones() != true) {
    return false;
  }
  if (getWays() != true) {
    return false;
  }
  int duration;
  if (getValue("manual", "duration", buffer, sizeof(buffer), duration) != true) {
    Serial.printf("%s:manual:duration not found (error %d)\n", m_fileName,
                  getError());
    duration = 5;
  }
  Watering::manualDuration(duration);

  if (getValue("moisture", "sensor", buffer, sizeof(buffer)) == true) {
    m_moistureSensor = atoi(buffer);
    if (getValue("moisture", "max", buffer, sizeof(buffer)) == true) {
      m_maxMoisture = atoi(buffer);
    }
  }
  if (getValue("flow", "sensor", buffer, sizeof(buffer)) == true) {
    m_flowSensor = atoi(buffer);
    if (getValue("flow", "max", buffer, sizeof(buffer)) == true) {
      m_maxFlow = atoi(buffer);
    }
  }

  Serial.printf("getting main valve\n");
  if (getValue("valve", "main", buffer, sizeof(buffer)) != true) {
    Serial.printf("%s:valve:main not found (error %d)\n", m_fileName,
                  getError());
    return false;
  }
  if (Valve::create(buffer) != true) {
    Serial.printf("%s: error creating valve\n", buffer);
    return false;
  }
  return true;
}

void Config::print(void) {
  Serial.printf("ssid: %s\n", m_ssid);
  Serial.printf("password: %s\n", m_password);
  Serial.printf("modules: %d/%d\n", Module::getCount(), MAX_MODULE);
  Module* module = Module::getFirst();
  while (module != 0 && module->isPresent()) {
    module->print();
    module = Module::getNext();
  }
  Serial.printf("relays: %d/%d\n", Relay::getCount(), MAX_WAY);
  Relay* relay = Relay::getFirst();
  while (relay != 0 && relay->isPresent()) {
    relay->print();
    relay = Relay::getNext();
  }
  Valve::getMainValve()->print();
  Serial.printf("zones: %d/%d\n", Zone::getCount(), MAX_ZONE);
  Zone* zone = Zone::getFirst();
  while (zone != 0) {
    zone->print();
    zone = Zone::getNext();
  }
  Serial.printf("ways: %d/%d\n", Way::getCount(), MAX_WAY);
  Way* way = Way::getFirst();
  while (way != 0) {
    way->print();
    way = Way::getNext();
  }
}
