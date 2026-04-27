#include <TimeLib.h>
#include <WiFi.h>

#include "rtc_module/rtc_module.h"
#include "config/config.h"
#include "cuve/cuve.h"
#include "sensors/sensors.h"
#include "watering/watering.h"
#include "schedule/schedule.h"
#include "web_server/web_server.h"
#include "oled/oled.h"
#include "hmi/hmi.h"
#include "preferences/preferences.h"

Config   config(CONFIG_FILE);
Schedule schedule(SCHEDULE_FILE);

RCSwitch radioCmd = RCSwitch();

Hmi  hmi;
Cuve cuve(radioCmd);

void setup(void) {
  Serial.begin(115200);

  if (!display.begin()) {
    Serial.println(F("OLED screen SSD1306 allocation failed"));
    while (true) delay(100);
  }

  // Initialiser le RTC
  if (!rtc.begin()) {
    Serial.println("Couldn't find RTC");
    display.displayError("Couldn't find RTC");
    while (true) delay(100);
  }
  if (rtc.lostPower()) {
    Serial.println("RTC lost power, setting time to last build time");
    display.displayError("RTC lost power");
    rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
    while (true) delay(100);
  }

  // sync esp32 clock every 300s from RTC
  setSyncProvider(syncTimeFromRTC);
  setSyncInterval(300);

  if (!SPIFFS.begin()) {
    Serial.println("SPIFFS.begin() failed");
    display.displayError("SPIFFS.begin() failed");
    while (true) delay(100);
  }

  Serial.print("\nCONFIG READ:\n");
  if (config.read() != true) {
    Serial.println("reading configuration failed");
    display.displayError("config read failed");
    while (true) delay(100);
  }
  Serial.print("\nCONFIG PRINT:\n");
  config.print();

  Serial.print("\nSCHEDULE READ:\n");
  if (schedule.read() != true) {
    Serial.println("schedule configuration failed");
    display.displayError("schedule read failed");
    while (true) delay(100);
  }
  Serial.print("\nSCHEDULE PRINT:\n");
  schedule.print();

  Serial.println("\nRESET all relays");
  Relay *relay = Relay::getFirst();
  while (relay != 0) {
    if (relay->isPresent()) {
      relay->off();
    }
    relay = Relay::getNext();
  }

  // Preferences manager -> Variable for seasonal adjustment
  prefManager.begin("sprinkler");

  hmi.setup();
  cuve.setup(true);

  pinMode(23, OUTPUT);          // sets the digital pin 23 as output
  radioCmd.enableTransmit(23);  // data de l'émetteur sur broche Digital 23
  // bien respecter l'ordre de ces 3 instructions qui suivent :
  radioCmd.setProtocol(1);       // à remplacer par votre valeur de protocol
  radioCmd.setPulseLength(346);  // à remplacer par votre valeur de PulseLength
  radioCmd.setRepeatTransmit(5);

  Serial.printf("\nCréation du point d'accès avec SSID:%s et PWD:%s ...\n", config.getSSID(), config.getPassword());
  if (not WiFi.softAP(config.getSSID(), NULL)) {
    Serial.println("wifi init failed");
    display.displayError("wifi init failed failed");
    while (true) delay(100);
  }
  Serial.print("Point d'accès créé avec succès à l'adresse IP: ");
  Serial.println(WiFi.softAPIP());

  // Websocket
  initWebSocket();
  initWebServer();

  Serial.println("INIT DONE");
}

void loop(void) {
  static time_t lastSec, lastMinute;
  time_t        currentTime = now();

  // Exécuter une tâche toutes les secondes
  if (currentTime != lastSec) {
    lastSec = currentTime;

    cuve.run();

    if (cuve.getCurrentState() == Cuve::Etat::DEFAUT) {
      //Watering::arretArrosage(radioCmd);
      display.displayError(cuve.getCurrentDefault().c_str());
      while (true) delay(100);
    }

    display.displayTimeDate();

    if (!hmi.isBusy())
      display.displayCuveState(cuve.getCurrentState());

    float flow = getFlow();
    // if (!hmi.isBusy())
    //   display.displayFlow(flow);
    if (flow > Config::getConfig()->getMaxFlow()) {
      Serial.printf("flow is too high\n");
      Watering::stopAllAutoWatering();
      Way::stopAllManualWatering();
    }

    if (Watering::isAnyWateringRunning()) {
      if (cuve.getCurrentState() == Cuve::Etat::INTERMEDIAIRE or cuve.getCurrentState() == Cuve::Etat::PLEINE)
        Watering::resetTimerAllumagePompe(radioCmd);
      display.displayMessage("ARROSAGE EN COURS");
    } else
      display.displayMessage("ARROSAGE A L'ARRET");
  }

  // Exécuter une tâche toutes les 10 secondes
  if (currentTime - lastMinute >= 10) {
    lastMinute = currentTime;

    char       timeString[50];
    struct tm *timeinfo = localtime(&currentTime);                 // Convertir en structure tm locale
    strftime(timeString, MAX_BUF, "%d/%m/%Y %H:%M:%S", timeinfo);  // Formater la chaîne de temps
    Serial.println("\n10s have passed at " + String(timeString));

    int moisture;
    getSoilMoisture(&moisture);  // just display to terminal
    if (!hmi.isBusy())
      display.displayMoisture(moisture);

    Watering::run(currentTime);
  }

  cleanUpClient();

  hmi.run();

  //delay(16);
}
