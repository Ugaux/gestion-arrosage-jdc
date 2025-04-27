#ifndef _RELAY_H_
#define _RELAY_H_

#include "IOExtenser/IOExtenser.h"
#include "constants.h"

enum relayState { ON,
                  OFF };

class Relay {
public:
  static bool   create(Module *mod, int id, const char *def);
  static int    getCount();
  static Relay *getByName(const char *name);
  static Relay *getFirst(void);
  static Relay *getNext(void);
  static Relay *getRelay(int n);
  Relay();
  bool        isPresent(void);
  bool        isLatch(void);
  void        print(const char *message = NULL);
  const char *getName(void);
  void        on(void);
  void        off(void);
  relayState  getState(void) { return m_state; }

private:
  static Relay m_relay[MAX_WAY];
  static int   m_searchIndex;
  Module      *m_module;
  moduleType   m_access;
  uint8_t      m_index;
  uint8_t      m_id;
  int8_t       m_onPin;
  int8_t       m_offPin;
  int8_t       m_level;
  relayState   m_state;
};

#endif
