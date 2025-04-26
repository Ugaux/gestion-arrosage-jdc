
#ifndef _MODULE_H_
#define _MODULE_H_

#include <Adafruit_MCP23X08.h>
#include <Adafruit_MCP23X17.h>
#include "constants.h"

#define MAX_MODULE_TYPE 4
#define MAX_MODULE      8

enum moduleType {NOTYPE, IO, MCP23008, MCP23017};

class Module
{
  public:
    static bool begin(void);
    static uint8_t getType(const char *name);
    static bool create(const char *def);
    static int getCount();
    static Module *getFirst(void);
    static Module *getNext(void);
    static Module *getModule(int n);
    static Module *getByName(const char *name);
    Module(void);
    bool isPresent(void) {
      return m_type != NOTYPE;
    }
    bool check(void);
    const char *getName(void);
    uint8_t getType(void) {
      return m_type;
    }
    uint8_t getId(void) {
      return m_id;
    }
    uint8_t getAddr(void) {
      return m_addr;
    }
    bool isConnected(void)  {
      return m_present;
    }
    void print(void);
    void setMode(uint8_t pin, uint8_t mode);
    void write(uint8_t pin, uint8_t value);

  private:
    static Module m_module[MAX_MODULE];
    static int m_searchIndex;
    uint8_t m_type;
    uint8_t m_index;
    uint8_t m_id;
    uint8_t m_addr;
    uint8_t m_present;
    Adafruit_MCP23X08 m_mcp23008;
    Adafruit_MCP23X17 m_mcp23017;
};

#endif
