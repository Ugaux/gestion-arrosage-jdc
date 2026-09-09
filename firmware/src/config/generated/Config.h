#pragma once

#include <bitset>
#include <array>
#include <cstdint>
#include "Constants.h"
#include "types/UUID.h"
#include "types/Collection.h"
#include "types/FixedString.h"
#include "types/Frequency.h"
#include "types/Weekdays.h"

struct Config {
  static constexpr uint8_t kMaxSchedulePerLine = 4;

  struct Zone {
    UUID id;
    FixedString<15> name = "NoName";
  };

  struct Line {
    UUID id;
    UUID zoneId;
    FixedString<15> name = "NoName";
    using ValveSet = std::bitset<kValveCount>;
    ValveSet valves = 0;
  };

  struct Schedule {
    UUID id;
    UUID lineId;
    bool enabled = false;
    FixedString<15> name;
    uint8_t hour = 7;
    uint8_t minute = 30;
    uint8_t duration = 20;  // in minutes
    bool onlyIfDrySoil = false;
    Frequency frequency = Frequency::EveryDay;
    WeekDays days = {};
  };

  struct UserSettings {

    struct Params {

      struct Wifi {
        FixedString<15> mdns = "jdc-watering";
        bool useAPMode = true;

        struct Station {
          FixedString<32> ssid = "YourWifiNetwork";
          FixedString<63> password = "12345678";
        } station;

        struct AP {
          FixedString<32> ssid = "WateringController";
          FixedString<63> password;
        } ap;
      } wifi;

      struct Watering {

        struct Duration {
          uint8_t min = 1;  // in minutes
          uint8_t max = 40;  // in minutes
          uint8_t base = 15;  // in minutes
          uint8_t step = 5;  // in minutes
        } duration;

        struct Seasonal {
          uint8_t factor = 100;  // in %
        } seasonal;

        struct Soil {

          struct Moisture {
            uint8_t threshold = 60;  // in %
          } moisture;
        } soil;

        struct Pump {

          struct Flow {
            uint8_t min = 2;  // in L/min
            uint8_t max = 80;  // in L/min
          } flow;
        } pump;
      } watering;
    } params;

    struct WateringModel {
      using ZoneCollection = Collection<Zone, kValveCount>;
      ZoneCollection zones;
      using LineCollection = Collection<Line, kValveCount>;
      LineCollection lines;
    } wateringModel;
  } userSettings;

  using ScheduleCollection = Collection<Schedule, kValveCount*kMaxSchedulePerLine>;
  ScheduleCollection schedules;
};
