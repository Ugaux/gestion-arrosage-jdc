#ifndef _CONFIG_H_
#define _CONFIG_H_

#include <SPIFFSIniFile.h>

#include "constants.h"

#define CONFIG_FILE "/config/config.ini"

class IniConfig : public SPIFFSIniFile {
public:
  IniConfig(const char *fileName);
  bool              read(void);
  void              print(void);
  static IniConfig *getConfig(void) { return m_config; }  // get the config instance
  char             *getSSID(void) { return m_ssid; }
  char             *getPassword(void) { return m_password; }
  uint8_t           getMoistureSensor(void) { return m_moistureSensor; }
  uint8_t           getMaxMoisture(void) { return m_maxMoisture; }
  uint8_t           getFlowSensor(void) { return m_flowSensor; }
  uint8_t           getMaxFlow(void) { return m_maxFlow; }

private:
  static IniConfig *m_config;  // config singleton (instance)
  const char       *m_fileName;
  char              m_ssid[MAX_LINE / 2];
  char              m_password[MAX_LINE / 2];
  uint8_t           m_moistureSensor;
  uint8_t           m_maxMoisture;
  uint8_t           m_flowSensor;
  uint8_t           m_maxFlow;
  bool              getModules(void);
  bool              getRelays(void);
  bool              getZones(void);
  bool              getWays(void);
};

#endif
