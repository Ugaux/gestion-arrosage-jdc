#include <WiFi.h>
#include <WiFiGeneric.h>
#include <esp_wifi.h>
#include <WiFiClient.h>
#include <WebServer.h>
#include <ESPmDNS.h>
#include <SPIFFS.h>
#include <esp_sntp.h>
#include <Wire.h>
#include <RTClib.h>
#include <TimeLib.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

#include "config/config.h"
#include "schedule/schedule.h"
#include "watering/watering.h"
#include "flow/flow.h"
#include "humidity/humidity.h"
#include "html/html.h"
#include "hmi/hmi.h"
#include "oled/oled.h"
#include "cuve/cuve.h"
#include "RTCModule/RTCModule.h"

//// Déclaration du prototype de la fonction getRtcTime()
//time_t getRtcTime();
//time_t getCurrentTime();

const char *ssid     = "JardinDuCiel";
const char *password = "";

AsyncWebServer server(80);

Config   config("/config.ini");
Schedule schedule(SCHEDULE_FILE);

const char *ntpServer = "pool.ntp.org";

Hmi  hmi;
Cuve cuve;

time_t getRtcTime();

void handleNotFound(AsyncWebServerRequest *request) {
  String message = "File Not Found\n\n";
  message += "URI: ";
  message += request->url();
  message += "\nMethod: ";
  message += request->methodToString();
  message += "\nArguments: ";
  message += request->args();
  message += "\n";
  for (uint8_t i = 0; i < request->args(); i++) {
    message += " " + request->argName(i) + ": " + request->arg(i) + "\n";
  }
  request->send(404, "text/plain", message);
}

void WiFiEvent(WiFiEvent_t event) {
  Serial.printf("[WiFi-event] event: %d\n", event);

  switch (event) {
    case ARDUINO_EVENT_WIFI_READY:
      Serial.println("WiFi interface ready");
      break;
    case ARDUINO_EVENT_WIFI_SCAN_DONE:
      Serial.println("Completed scan for access points");
      break;
    case ARDUINO_EVENT_WIFI_STA_START:
      Serial.println("WiFi client started");
      break;
    case ARDUINO_EVENT_WIFI_STA_STOP:
      Serial.println("WiFi clients stopped");
      break;
    case ARDUINO_EVENT_WIFI_STA_CONNECTED:
      Serial.println("Connected to access point");
      break;
    case ARDUINO_EVENT_WIFI_STA_DISCONNECTED:
      Serial.println("Disconnected from WiFi access point");
      break;
    case ARDUINO_EVENT_WIFI_STA_AUTHMODE_CHANGE:
      Serial.println("Authentication mode of access point has changed");
      break;
    case ARDUINO_EVENT_WIFI_STA_GOT_IP:
      Serial.print("Obtained IP address: ");
      Serial.println(WiFi.localIP());
      break;
    case ARDUINO_EVENT_WIFI_STA_LOST_IP:
      Serial.println("Lost IP address and IP address is reset to 0");
      break;
    case ARDUINO_EVENT_WPS_ER_SUCCESS:
      Serial.println("WiFi Protected Setup (WPS): succeeded in enrollee mode");
      break;
    case ARDUINO_EVENT_WPS_ER_FAILED:
      Serial.println("WiFi Protected Setup (WPS): failed in enrollee mode");
      break;
    case ARDUINO_EVENT_WPS_ER_TIMEOUT:
      Serial.println("WiFi Protected Setup (WPS): timeout in enrollee mode");
      break;
    case ARDUINO_EVENT_WPS_ER_PIN:
      Serial.println("WiFi Protected Setup (WPS): pin code in enrollee mode");
      break;
    case ARDUINO_EVENT_WIFI_AP_START:
      Serial.println("WiFi access point started");
      break;
    case ARDUINO_EVENT_WIFI_AP_STOP:
      Serial.println("WiFi access point  stopped");
      break;
    case ARDUINO_EVENT_WIFI_AP_STACONNECTED:
      Serial.println("Client connected");
      break;
    case ARDUINO_EVENT_WIFI_AP_STADISCONNECTED:
      Serial.println("Client disconnected");
      break;
    case ARDUINO_EVENT_WIFI_AP_STAIPASSIGNED:
      Serial.println("Assigned IP address to client");
      break;
    case ARDUINO_EVENT_WIFI_AP_PROBEREQRECVED:
      Serial.println("Received probe request");
      break;
    case ARDUINO_EVENT_WIFI_AP_GOT_IP6:
      Serial.println("AP IPv6 is preferred");
      break;
    case ARDUINO_EVENT_WIFI_STA_GOT_IP6:
      Serial.println("STA IPv6 is preferred");
      break;
    case ARDUINO_EVENT_ETH_GOT_IP6:
      Serial.println("Ethernet IPv6 is preferred");
      break;
    case ARDUINO_EVENT_ETH_START:
      Serial.println("Ethernet started");
      break;
    case ARDUINO_EVENT_ETH_STOP:
      Serial.println("Ethernet stopped");
      break;
    case ARDUINO_EVENT_ETH_CONNECTED:
      Serial.println("Ethernet connected");
      break;
    case ARDUINO_EVENT_ETH_DISCONNECTED:
      Serial.println("Ethernet disconnected");
      break;
    case ARDUINO_EVENT_ETH_GOT_IP:
      Serial.println("Obtained IP address");
      break;
    default:
      break;
  }
}

void WiFiGotIP(WiFiEvent_t event, WiFiEventInfo_t info) {
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(IPAddress(info.got_ip.ip_info.ip.addr));
}

void setup(void) {
  Serial.begin(115200);

  // Initialiser le RTC
  setupRTC();

  // Initialiser la synchronisation de l'heure
  setSyncProvider(syncTimeFromRTC);  // Utilise la nouvelle fonction pour synchroniser l'heure

  //  // Initialiser la communication I2C
  //  Wire.begin(21, 22); // SDA sur GPIO21, SCL sur GPIO22

  cuve.setup();

  if (!display.begin()) {
    Serial.println(F("SSD1306 allocation failed"));
    for (;;);  // Don't proceed, loop forever
  }

  hmi.begin();

  if (!SPIFFS.begin()) {
    Serial.println("SPIFFS.begin() failed");
  }
  if (config.read() != true) {
    Serial.println("configuration failed");
    while (1)
      sleep(1);
  }
  config.print();
  if (schedule.read() != true) {
    Serial.println("schedule configuration failed");
    while (1)
      sleep(1);
  }
  schedule.print();
  //  WiFi.onEvent(WiFiStationConnected, SYSTEM_EVENT_STA_CONNECTED);
  //  WiFi.onEvent(WiFiGotIP, SYSTEM_EVENT_STA_GOT_IP);
  //  WiFi.onEvent(WiFiStationDisconnected, SYSTEM_EVENT_STA_DISCONNECTED);
  //  WiFi.mode(WIFI_STA);
  //  WiFi.disconnect();
  //  ssid = config.getSsid();
  //  password = config.getPassword();
  //  WiFi.begin(ssid, password);
  //  Serial.println("");
  //  Serial.print("Connecting to ");
  //  Serial.println(ssid);

  // delete old config

  /*
    WiFi.disconnect(true);

    delay(1000);

    // Examples of different ways to register wifi events
    WiFi.onEvent(WiFiEvent);
    WiFi.onEvent(WiFiGotIP, WiFiEvent_t::ARDUINO_EVENT_WIFI_STA_GOT_IP);
    WiFiEventId_t eventID = WiFi.onEvent([](WiFiEvent_t event, WiFiEventInfo_t info) {
     Serial.print("WiFi lost connection. Reason: ");
     Serial.println(info.wifi_sta_disconnected.reason);
    }, WiFiEvent_t::ARDUINO_EVENT_WIFI_STA_DISCONNECTED);

    // Remove WiFi event
    Serial.print("WiFi Event ID: ");
    Serial.println(eventID);
    // WiFi.removeEvent(eventID);

    WiFi.begin(ssid, password);

    Serial.println();
    Serial.println();
    Serial.println("Wait for WiFi... ");
  */

  Serial.println("\n");

  Serial.println("Creation du point d'acces...");
  WiFi.softAP(ssid, password);

  Serial.print("Adresse IP: ");
  Serial.println(WiFi.softAPIP());

  //  File file = SPIFFS.open("/index.html");
  //  if (!file || file.isDirectory()) {
  //    Serial.println("− failed to open file for reading");
  //    return;
  //  }
  //  Serial.println("− read from file:");
  //  while (file.available()) {
  //    Serial.write(file.read());
  //  }
  //  file.close();

  server.on("/", handleRoot);

  server.on("/configure", handleConfigure);

  server.on("/configure_submit", handleConfigureSubmit);

  server.on("/new", handleNew);

  server.on("/new_submit", handleNewSubmit);

  server.on("/manual", handleManual);

  server.on("/manual_in_progress", handleManualInProgress);

  server.on("/maint.html", HTTP_GET, [](AsyncWebServerRequest *request) {
    request->send(SPIFFS, "/maint.html", "text/html", false);
  });

  server.on("/test_relays", HTTP_GET, [](AsyncWebServerRequest *request) {
    Serial.println("Relays test");
    handleTest(request);
  });

  server.on("/on", HTTP_GET, [](AsyncWebServerRequest *request) {
    if (request->hasParam("relay")) {
      String name = request->arg("relay");
      Serial.printf("ON relay=%s\n", name.c_str());
      Relay *relay = Relay::getByName(name.c_str());
      if (relay == 0) {
        Serial.printf("%s: not found\n", name.c_str());
      } else {
        relay->on();
      }
    }
    handleTest(request);
  });

  server.on("/off", HTTP_GET, [](AsyncWebServerRequest *request) {
    if (request->hasParam("relay")) {
      String name = request->arg("relay");
      Serial.printf("ON relay=%s\n", name.c_str());
      Relay *relay = Relay::getByName(name.c_str());
      if (relay == 0) {
        Serial.printf("%s: not found\n", name.c_str());
      } else {
        relay->off();
      }
    }
    handleTest(request);
  });

  server.on("/config_ini", HTTP_GET, [](AsyncWebServerRequest *request) {
    request->send(SPIFFS, "/config.ini", "text/plain", false);
  });

  server.on("/schedule_ini", HTTP_GET, [](AsyncWebServerRequest *request) {
    request->send(SPIFFS, "/schedule.ini", "text/plain", false);
  });

  server.onNotFound(handleNotFound);

  server.begin();
  //  esp_wifi_set_ps(WIFI_PS_MIN_MODEM);
  Serial.println("HTTP server started");

  //  configTzTime("CET-1CEST-2,M3.5.0/02:00:00,M10.5.0/03:00:00", ntpServer);

  Serial.println("RESET all relays");
  Relay *relay = Relay::getFirst();
  while (relay != 0) {
    if (relay->isPresent()) {
      relay->off();
    }
    relay = Relay::getNext();
  }
  flowInit();
}

bool sntp_sync;

void loop(void) {

  static time_t lastSec;
  static time_t lastMinute;

  time_t currentTime = getRtcTime();

  // Exécuter une tâche toutes les secondes
  if (currentTime != lastSec) {
    lastSec = currentTime;
    //    Serial.println("One second has passed.");
    //    displayTimeDate();
    cuve.run();

    float flow = getFlow();
    if (!hmi.isBusy()) {
      display.displayFlow(flow);
    }
    if (flow > Config::getConfig()->getMaxFlow()) {
      Serial.printf("flow is tooOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOO high\n");
      Watering::stopAllAutoWatering();
      Way::stopAllManualWatering();
    }
  }

  // Exécuter une tâche toutes les 60 secondes
  if (currentTime - lastMinute >= 10) {
    lastMinute = currentTime;
    Serial.println("60s have passed.");
    // Ajouter ici les tâches à exécuter toutes les 5 secondes
    int moisture;
    getSoilMoisture(&moisture);  // just display to terminal
    if (!hmi.isBusy()) {
      display.displayMoisture(moisture);
    }
    display.displayTimeDate();
    //    Watering::run(timestamp);
    Watering::run(currentTime);
  }
  hmi.run();
}

time_t getRtcTime() {
  DateTime     now = rtc.now();
  tmElements_t tm;
  tm.Second = now.second();
  tm.Minute = now.minute();
  tm.Hour   = now.hour();
  tm.Day    = now.day();
  tm.Month  = now.month();
  tm.Year   = CalendarYrToTm(now.year());
  return makeTime(tm);
}

void fonction() {
  // Exemple d'utilisation de displayNextWatering
  time_t t = getRtcTime();  // t = heure actuelle
  Serial.println(t);
  //  display.displayNextWatering(way, t);
}

void displayTimeDate() {
  char timeString[MAX_BUF];

  DateTime   now         = getCurrentTime();                  // Obtenir l'heure actuelle du RTC
  time_t     currentTime = now.unixtime();                    // Convertir en Unix timestamp
  struct tm *timeinfo    = localtime(&currentTime);           // Convertir en structure tm locale
  strftime(timeString, MAX_BUF, "%d/%m/%Y %H:%M", timeinfo);  // Formater la chaîne de temps

  Serial.printf("Oled::displayTimeDate %s\n", timeString);
  int16_t  x1, y1;
  uint16_t w, h;
  display.getTextBounds(timeString, 0, 0, &x1, &y1, &w, &h);
  display.fillRect(0, 0, SCREEN_WIDTH, 16, SSD1306_BLACK);  // Effacer la ligne précédente
  display.setCursor((SCREEN_WIDTH - w) / 2, 0);             // Centrer le texte
  display.print(timeString);
  display.display();
}
