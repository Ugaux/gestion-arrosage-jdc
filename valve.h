#ifndef _VALVE_H_
#define _VALVE_H_

#include "relay.h"

#define OPEN_TIME 20

enum valveState { VALVE_UNKNOWN,
                  VALVE_IS_CLOSED,
                  VALVE_IS_OPENING,
                  VALVE_IS_OPEN,
                  VALVE_IS_CLOSING };
extern const char *valveStateName[];

class Valve {
public:
  Valve();
  static bool   create(const char *def);
  static Valve *getMainValve(void) { return &m_mainValve; }
  void          open(void);
  void          close(void);
  void          isOpen(void);
  void          isClosed(void);
  void          print(void);
  valveState    getState(void) { return m_state; }

private:
  static Valve m_mainValve;
  Relay       *m_openRelay;
  Relay       *m_closeRelay;
  valveState   m_state;
};

#endif
