#include <Arduino.h>

#include "relay.h"
#include "cuve.h"

int   Relay::m_searchIndex;
Relay Relay::m_relay[MAX_WAY];

Relay::Relay() : m_module(0),
                 m_index(0),
                 m_id(0),
                 m_access(IO),
                 m_onPin(-1), m_offPin(-1),
                 m_level(HIGH) {
}

// create a relay
// module: the module instance (GPIO, MCP23008 or MCP23017)
// id: the ID of the relay
// def : relay description string, for example:
//   gpio-1.0, NORMAL, GPIO, LOW-LEVEL(4)
//   mcp23017-1.0, NORMAL, I2C, HIGH-LEVEL(0)
bool Relay::create(Module *mod, int id, const char *def) {
  Relay      *relay;
  char        tmp[MAX_DEF], tmp2[MAX_DEF];
  const char *p, *prefix;
  char       *s;

  relay = Relay::getRelay(m_searchIndex);
  Serial.printf("relay::create %d %s\n", m_searchIndex, def);
  if (relay == 0) {
    Serial.printf("relay %d: too much items !!!\n", m_searchIndex);
    return false;
  }
  relay->m_module = mod;
  relay->m_index  = m_searchIndex;
  relay->m_id     = id;
  strncpy(tmp, def, MAX_DEF);
  prefix = strtok_r(tmp, "(", &s);
  if (prefix == NULL) {
    Module *mod = Module::getByName(prefix);
    if (mod == 0) {
      Serial.printf("%s: bad value\n", def);
    }
    return false;
  }
  if (!strcmp(prefix, "GPIO-H")) {
    relay->m_access = IO;
    relay->m_level  = HIGH;
  } else if (!strcmp(prefix, "GPIO-L")) {
    relay->m_access = IO;
    relay->m_level  = LOW;
  } else if (!strcmp(prefix, "I2C-H")) {
    relay->m_access = MCP23008;
    relay->m_level  = HIGH;
  } else if (!strcmp(prefix, "I2C-L")) {
    relay->m_access = MCP23008;
    relay->m_level  = LOW;
  } else {
    Serial.printf("%s: bad value\n", def);
    return false;
  }
  p = strtok_r(NULL, ")", &s);
  if (p == NULL) {
    Serial.printf("%s: bad format, missing parenthesis\n", def);
    return false;
  }
  strncpy(tmp2, p, MAX_DEF);
  if (strchr(p, '+')) {
    // ie GPIO-L(4+5) : create latch relay
    p               = strtok_r(tmp2, "+", &s);
    relay->m_onPin  = atoi(p);
    p               = strtok_r(NULL, "+", &s);
    relay->m_offPin = atoi(p);
    relay->m_module->setMode(relay->m_onPin, OUTPUT);
    relay->m_module->write(relay->m_onPin, relay->m_level == HIGH ? LOW : HIGH);
    relay->m_module->setMode(relay->m_offPin, OUTPUT);
    relay->m_module->write(relay->m_offPin, relay->m_level == HIGH ? LOW : HIGH);
  } else if (strchr(p, '-')) {
    // ie I2C-L(0-7) : create relay group
    char key[MAX_DEF];
    p        = strtok_r(tmp2, "-", &s);
    int from = atoi(p);
    p        = strtok_r(NULL, "-", &s);
    int to   = atoi(p);
    for (int pin = from; pin <= to; pin++) {
      snprintf(key, MAX_DEF, "%s(%d)", prefix, pin);
      Relay::create(mod, pin, key);
    }
    return true;
  } else {
    relay->m_onPin = atoi(p);
    relay->m_module->setMode(relay->m_onPin, OUTPUT);
    relay->m_module->write(relay->m_onPin, relay->m_level == HIGH ? LOW : HIGH);
  }
  relay->print("relay::create ");
  m_searchIndex++;
  return true;
}

// return the number of relays
int Relay::getCount() {
  int n;

  for (n = 0; n < MAX_WAY && m_relay[n].isPresent() == true; n++);
  return n;
}

// return a relay giving its name
Relay *Relay::getByName(const char *def) {
  Relay *relay = Relay::getFirst();
  while (relay != 0) {
    if (!strcmp(relay->getName(), def)) {
      Serial.printf("relay::getByName found %s\n", def);
      return relay;
    }
    relay = Relay::getNext();
  }
  return 0;
}

// return the first relays
Relay *Relay::getFirst(void) {
  m_searchIndex = 0;
  return &m_relay[m_searchIndex];
}

// return the next relays
Relay *Relay::getNext(void) {
  m_searchIndex++;
  if (m_searchIndex < MAX_WAY) {
    return &m_relay[m_searchIndex];
  }
  return 0;
}

// return the nth relays
Relay *Relay::getRelay(int n) {
  if (n < MAX_WAY) {
    return &m_relay[n];
  }
  return 0;
}

// return relay's presence
bool Relay::isPresent(void) {
  return (m_onPin != -1 || m_offPin != -1);
}

// return true if the relay is a latch relay
bool Relay::isLatch(void) {
  if (!isPresent()) return false;
  return m_offPin != -1;
}

// print the relay
void Relay::print(const char *message) {
  if (message) {
    Serial.printf(message);
  }
  if (!isPresent()) {
    Serial.printf("Not present\n");
    return;
  }
  if (isLatch()) {
    Serial.printf("%d: %s, LATCH, %s, %s(%d,%d)\n", m_index, getName(), m_access == IO ? "GPIO" : "I2C", m_level == HIGH ? "HIGH-LEVEL" : "LOW-LEVEL", m_onPin, m_offPin);
  } else {
    Serial.printf("%d: %s, NORMAL, %s, %s(%d)\n", m_index, getName(), m_access == IO ? "GPIO" : "I2C", m_level == HIGH ? "HIGH-LEVEL" : "LOW-LEVEL", m_onPin);
  }
}

// return the name of the relay
const char *Relay::getName(void) {
  static char name[MAX_DEF];
  if (m_module) {
    snprintf(name, MAX_DEF, "%s.%d", m_module->getName(), m_id);
    return name;
  }
  return "NONE";
}

// turn ON the relay
void Relay::on(void) {
  if (!isPresent()) {
    return;
  }
  if (!m_module->isConnected()) {
    Serial.printf("%s not connected !!!\n", m_module->getName());
    //return;
  }
  if (isLatch()) {
    Serial.printf("%s: pin%d=%d\n", getName(), m_onPin, m_level);
    m_module->write(m_onPin, m_level);
    delay(50);
    Serial.printf("%s: pin%d=%d\n", getName(), m_onPin, !m_level);
    m_module->write(m_onPin, !m_level);
  } else {
    Serial.printf("%s: pin%d=%d\n", getName(), m_onPin, m_level);
    m_module->write(m_onPin, m_level);
  }
  m_state = ON;
}

// turn OFF the relay
void Relay::off(void) {
  if (!isPresent()) {
    return;
  }
  if (!m_module->isConnected()) {
    Serial.printf("%s not connected !!!\n", m_module->getName());
    //return;
  }
  if (isLatch()) {
    Serial.printf("%s: pin%d=%d\n", getName(), m_offPin, m_level);
    m_module->write(m_offPin, m_level);
    delay(50);
    Serial.printf("%s: pin%d=%d\n", getName(), m_offPin, !m_level);
    m_module->write(m_offPin, !m_level);
  } else {
    Serial.printf("%s: pin%d=%d\n", getName(), m_onPin, !m_level);
    m_module->write(m_onPin, !m_level);
  }
  m_state = OFF;
}
