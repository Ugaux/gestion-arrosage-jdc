#include <Arduino.h>
#include <Ticker.h>

#include "valve.h"
#include "oled.h"

Ticker timer;
Valve  Valve::m_mainValve;

// timer callback
void changeState(Valve *v) {
  if (v->getState() == VALVE_IS_OPENING) {
    v->isOpen();
  } else if (v->getState() == VALVE_IS_CLOSING) {
    v->isClosed();
  }
  display.clearMessage();
  timer.detach();
}

// create the valve
bool Valve::create(const char *def) {
  char        tmp[MAX_DEF];
  const char *p;
  char       *s;

  Serial.printf("valve::create %s\n", def);
  strncpy(tmp, def, MAX_DEF);
  p = strtok_r(tmp, ", ", &s);
  if (p == NULL) {
    Serial.printf("%s: bad value\n", def);
    return false;
  }
  m_mainValve.m_openRelay = Relay::getByName(p);

  if (m_mainValve.m_openRelay == 0) {
    Serial.printf("%s: bad value for open relay\n", def);
    return false;
  }
  p = strtok_r(NULL, ", ", &s);
  if (p == NULL) {
    return true;
  }
  m_mainValve.m_closeRelay = Relay::getByName(p);

  if (m_mainValve.m_closeRelay == 0) {
    Serial.printf("%s: bad value for close relay\n", def);
    return false;
  }
  return true;
}

Valve::Valve(void) : m_state(VALVE_UNKNOWN) {
}

// open the valve
void Valve::open(void) {
  if (m_state == VALVE_IS_CLOSED) {
    display.displayMessage("OUVERTURE VANNE");
    if (m_closeRelay) {
      // it's a motorized valve (2 relays)
      m_openRelay->on();
      m_state = VALVE_IS_OPENING;
      timer.attach(OPEN_TIME, changeState, this);
    } else {
      // it's a normal valve
      m_openRelay->on();
      m_state = VALVE_IS_OPEN;
      digitalWrite(33, LOW);
      MarcheArrosage();
    }
  }
}

// close the valve
void Valve::close(void) {
  if (m_state == VALVE_IS_OPEN || m_state == VALVE_UNKNOWN) {
    display.displayMessage("FERMETURE VANNE");
    if (m_closeRelay) {
      // it's a motorized valve (2 relays)
      m_closeRelay->on();
      m_state = VALVE_IS_CLOSING;
      timer.attach(OPEN_TIME, changeState, this);
    } else {
      // it's a normal valve
      m_openRelay->off();
      m_state = VALVE_IS_CLOSED;
      digitalWrite(33, HIGH);
      ArretArrosage();
    }
  }
}

// the motorized valve is open
void Valve::isOpen(void) {
  Serial.printf(">>>> VALVE is OPEN\n");
  m_state = VALVE_IS_OPEN;
  m_openRelay->off();
}

// the motorized valve is closed
void Valve::isClosed(void) {
  Serial.printf(">>>> VALVE is CLOSED\n");
  m_state = VALVE_IS_CLOSED;
  m_closeRelay->off();
}

// print the valve
void Valve::print(void) {
  Serial.printf("main valve %s %s\n", m_openRelay ? m_openRelay->getName() : "NONE", m_closeRelay ? m_closeRelay->getName() : "NONE");
}
