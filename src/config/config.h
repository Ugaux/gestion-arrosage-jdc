#ifndef _CONFIG_H_
#define _CONFIG_H_

#include <SPIFFS.h>
#include <FS.h>

#include "../SPIFFSIniFile/SPIFFSIniFile.h"
// Supposed to be an Arduino library but latest working version was not released with a tag

#include "../relay/relay.h"
#include "../valve/valve.h"
#include "../zone/zone.h"
#include "../way/way.h"
#include "../watering/watering.h"

class Config : public SPIFFSIniFile {
public:
  Config(const char *fileName);
  bool           read(void);
  void           print(void);
  static Config *getConfig(void) { return m_config; }  // get the config instance
  char          *getSsid(void) { return m_ssid; }
  char          *getPassword(void) { return m_password; }
  uint8_t        getMoistureSensor(void) { return m_moistureSensor; }
  uint8_t        getMaxMoisture(void) { return m_maxMoisture; }
  uint8_t        getFlowSensor(void) { return m_flowSensor; }
  uint8_t        getMaxFlow(void) { return m_maxFlow; }

private:
  static Config *m_config;  // config singleton (instance)
  const char    *m_fileName;
  char           m_ssid[MAX_LINE / 2];
  char           m_password[MAX_LINE / 2];
  uint8_t        m_moistureSensor;
  uint8_t        m_maxMoisture;
  uint8_t        m_flowSensor;
  uint8_t        m_maxFlow;
  bool           getModules(void);
  bool           getRelays(void);
  bool           getZones(void);
  bool           getWays(void);
};

#endif
