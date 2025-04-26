
#ifndef _CONFIG_H_
#define _CONFIG_H_

#include <SPIFFS.h>
#include <FS.h>

#include "SPIFFSIniFile.h"

#include "relay.h"
#include "valve.h"
#include "zone.h"
#include "way.h"
#include "watering.h"

class Config : public SPIFFSIniFile
{
  public:
    Config(const char *fileName);
    bool read(void);
    void print(void);
    // get the config instance
    static Config *getConfig(void) {return m_config;}
    char *getSsid(void) {
      return m_ssid;
    }
    char *getPassword(void)  {
      return m_password;
    }
    uint8_t getMoistureSensor(void) {
      return m_moistureSensor;
    }
    uint8_t getMaxMoisture(void) {
      return m_maxMoisture;
    }
    uint8_t getFlowSensor(void) {
      return m_flowSensor;
    }
    uint8_t getMaxFlow(void) {
      return m_maxFlow;
    }

  private:
    // config singleton (instance)
    static Config *m_config;
    const char *m_fileName;
    char m_ssid[MAX_LINE / 2];
    char m_password[MAX_LINE / 2];
    uint8_t m_moistureSensor;
    uint8_t m_maxMoisture;
    uint8_t m_flowSensor;
    uint8_t m_maxFlow;
    bool getModules(void);
    bool getRelays(void);
    bool getZones(void);
    bool getWays(void);
};

#endif
