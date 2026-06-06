#ifndef _PREF_MANAGER_H_
#define _PREF_MANAGER_H_

#include <Preferences.h>

class PreferencesManager {
public:
  PreferencesManager();

  void begin(const char *name);
  int  getInt(const char *key, int defaultValue);
  void setInt(const char *key, int value);

private:
  Preferences preferences;
};

extern PreferencesManager prefManager;

#endif
