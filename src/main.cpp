#include <TimeLib.h>
#include <WiFi.h>
#include <LittleFS.h>
#include <esp_system.h>
#include "hardware/RTCModule.h"
#include "hardware/Cuve.h"
#include "hardware/Sensors.h"
#include "config/IniConfig.h"
#include "config/IniSchedule.h"
#include "config/PreferencesManager.h"
#include "ui/HMI.h"
#include "ui/OledScreen.h"
#include "ui/WebServer.h"
#include "core/Watering.h"

#define DEV_MODE true

const char *reset_reason_to_str(esp_reset_reason_t reason) {
  switch (reason) {
    case ESP_RST_UNKNOWN: return "Unknown";
    case ESP_RST_POWERON: return "Power-on";
    case ESP_RST_EXT: return "External reset";
    case ESP_RST_SW: return "Software reset";
    case ESP_RST_PANIC: return "Panic (exception)";
    case ESP_RST_INT_WDT: return "Interrupt watchdog";
    case ESP_RST_TASK_WDT: return "Task watchdog";
    case ESP_RST_WDT: return "Other watchdog";
    case ESP_RST_DEEPSLEEP: return "Wake from deep sleep";
    case ESP_RST_BROWNOUT: return "Brownout reset";
    case ESP_RST_SDIO: return "SDIO reset";
    default: return "Invalid";
  }
}

void printMemory() {
  Serial.printf("Free heap: %d\n", ESP.getFreeHeap());
  Serial.printf("Min free heap: %d\n", ESP.getMinFreeHeap());
}
void printResetReason() {
  Serial.print("Reset reason: ");
  Serial.println(reset_reason_to_str(esp_reset_reason()));
}

IniConfig   iniConfig(CONFIG_FILE);
IniSchedule iniSchedule(SCHEDULE_FILE);

RCSwitch radioCmd = RCSwitch();

Hmi  hmi;
Cuve cuve(radioCmd);

void setup(void) {
  Serial.begin(115200);
  delay(2000);  // gives time to open monitor

  printResetReason();

  if (!display.begin()) {
    Serial.println(F("OLED screen SSD1306 allocation failed"));
    while (true) delay(100);
  }

  if (!LittleFS.begin(true)) {
    Serial.println("LittleFS Mount Failed");
    display.displayError("LittleFS.begin() failed");
    while (true) delay(100);
  }

  Serial.print("\nCONFIG READ:\n");
  if (iniConfig.read() != true) {
    Serial.println("reading iniConfiguration failed");
    display.displayError("iniConfig read failed");
    while (true) delay(100);
  }
  Serial.print("\nCONFIG PRINT:\n");
  iniConfig.print();

  Serial.print("\nSCHEDULE READ:\n");
  if (iniSchedule.read() != true) {
    Serial.println("iniSchedule configuration failed");
    display.displayError("iniSchedule read failed");
    while (true) delay(100);
  }
  Serial.print("\nSCHEDULE PRINT:\n");
  iniSchedule.print();

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
    if (flow > IniConfig::getConfig()->getMaxFlow()) {
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
