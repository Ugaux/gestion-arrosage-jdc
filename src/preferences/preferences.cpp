#include "preferences.h"

PreferencesManager prefManager;

/********** Constructor **********/
PreferencesManager::PreferencesManager() {}

/********** Init namespace **********/
void PreferencesManager::begin(const char *name) {
  preferences.begin(name, false);
}

/********** Get key value **********/
int PreferencesManager::getInt(const char *key, int defaultValue) {
  return preferences.getInt(key, defaultValue);
}

/********** Set key value **********/
void PreferencesManager::setInt(const char *key, int value) {
  preferences.putInt(key, value);
}
