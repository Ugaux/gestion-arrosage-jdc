
#include <Arduino.h>
#include "module.h"

int Module::m_searchIndex;
Module Module::m_module[MAX_MODULE];
const char *moduleName[MAX_MODULE_TYPE] = {"NOTYPE", "gpio", "mcp23008", "mcp23017"};

Module::Module() :
  m_type(NOTYPE),
  m_id(0),
  m_addr(0),
  m_present(false)
{
}

bool Module::begin()
{
  return create("gpio");
}

// return the type of a module
uint8_t Module::getType(const char *name)
{
  for (int t = 0 ; t < MAX_MODULE_TYPE ; t++) {
    if (!strcasecmp(moduleName[t], name)) {
      return t;
    }
  }
  return NOTYPE;
}

// create a module (GPIO, MCP23008 or MCP23017)
// def : module description string (mcp23017-1(0x20) for example)
bool Module::create(const char *def)
{
  char tmp[MAX_DEF];
  Module *mod;
  const char *p;
  char *s;

  Serial.printf("module::create %s\n", def);
  strncpy(tmp, def, MAX_DEF);
  p = strtok_r(tmp, "-", &s);
  if (p == NULL) {
    Serial.printf("%s: bad format, missing '-'\n", def);
    return false;
  }
  mod = &m_module[m_searchIndex];
  mod->m_index = m_searchIndex;
  mod->m_type = getType(p);
  if (mod->m_type == NOTYPE) {
    Serial.printf("%s: bad value\n", def);
    return false;
  }
  p = strtok_r(NULL, "(", &s);
  if (p == NULL) {
    Serial.printf("%s: bad format, missing '('\n", def);
    return false;
  }
  mod->m_id = atoi(p);
  p = strtok_r(NULL, ")", &s);
  if (p != NULL) {
    if (*p == '0' && *(p + 1) == 'x') {
      unsigned addr;
      sscanf(p + 2, "%x", &addr);
      mod->m_addr = (uint8_t)addr;
    }
    else {
      mod->m_addr = atoi(p);
    }
  }
  switch (mod->m_type) {
    case IO:
      mod->m_present = true;
      break;
    case MCP23008:
      mod->m_mcp23008.begin_I2C(mod->m_addr);
      mod->m_present = mod->check();
      break;
    case MCP23017:
      mod->m_mcp23017.begin_I2C(mod->m_addr);
      mod->m_present = mod->check();
      break;
  }
  Serial.printf("module::create OK %s %d addr=%d\n", moduleName[mod->m_type], mod->m_id, mod->m_addr);
  m_searchIndex++;
  return true;
}

// return the number of modules
int Module::getCount()
{
  int n;

  for (n = 0 ; n < MAX_MODULE && m_module[n].isPresent() == true ; n++);
  return n;
}

// return the first module
Module *Module::getFirst(void)
{
  m_searchIndex = 0;
  return &m_module[m_searchIndex];
}

// return the next module
Module *Module::getNext(void)
{
  m_searchIndex++;
  if (m_searchIndex < MAX_MODULE) {
    return &m_module[m_searchIndex];
  }
  return 0;
}

// return the nth module
Module *Module::getModule(int n)
{
  if (n < MAX_MODULE) {
    return &m_module[n];
  }
  return 0;
}

// return a module giving its name
Module *Module::getByName(const char *def)
{
  char tmp[MAX_DEF];
  int type, id;
  const char *p;
  char *s;

  Serial.printf("module::getByName %s\n", def);
  strncpy(tmp, def, MAX_DEF);
  p = strtok_r(tmp, "-", &s);
  if (p == NULL) {
    Serial.printf("%s: bad format, missing '-'\n", def);
    return 0;
  }
  type = getType(p);
  if (type == NOTYPE) {
    Serial.printf("%s: not found\n", def);
  }
  p = strtok_r(NULL, ".", &s);
  if (p == NULL) {
    Serial.printf("%s: bad format, missing '.'\n", def);
    return 0;
  }
  id = atoi(p);
  Module *module = getFirst();
  while (module != 0) {
    if (module->getType() == type && module->getId() == id) {
      Serial.printf("module::getByName found %s %d\n", moduleName[type], id);
      return module;
    }
    module = getNext();
  }
  return 0;
}

// check if an I2C module is connected
bool Module::check(void)
{
  Wire.beginTransmission(m_addr);
  byte error = Wire.endTransmission();
  if (error == 0)  {
    Serial.printf("I2C device found at 0x%02x\n", m_addr);
    return true;
  }
  else if (error == 4) {
    Serial.printf("I2C error at 0x%02x\n", m_addr);
  }
  Serial.printf("I2C device not found at 0x%02x\n", m_addr);
  return false;
}

// return the module's name
const char *Module::getName(void)
{
  static char name[MAX_DEF];
  snprintf(name, MAX_DEF, "%s-%d", moduleName[m_type], m_id);
  return name;
}

// print the module
void Module::print(void)
{
  if (!isPresent()) {
    Serial.printf("Not present\n");
    return;
  }
  Serial.printf("%d: %s, addr=%x\n", m_searchIndex, getName(), getAddr());
}

// set the module's GPIO mode
void Module::setMode(uint8_t pin, uint8_t mode)
{
  Serial.printf("%s: pin%d=OUTPUT\n", getName(), pin);
  switch (m_type) {
    case IO:
      pinMode(pin, mode);
      break;
    case MCP23008:
      m_mcp23008.pinMode(pin, mode);
      break;
    case MCP23017:
      m_mcp23017.pinMode(pin, mode);
      break;
  }
}

// write HIGH or LOW to the module's GPIO
void Module::write(uint8_t pin, uint8_t value)
{
  switch (m_type) {
    case IO:
      digitalWrite(pin, value);
      break;
    case MCP23008:
      m_mcp23008.digitalWrite(pin, value);
      break;
    case MCP23017:
      m_mcp23017.digitalWrite(pin, value);
      break;
  }
}
