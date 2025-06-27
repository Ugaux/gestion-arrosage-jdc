#include <TimeLib.h>
#include <WiFi.h>

#include "RTCModule/RTCModule.h"
#include "config/config.h"
#include "cuve/cuve.h"
#include "humidity/humidity.h"
#include "flow/flow.h"
#include "watering/watering.h"
#include "schedule/schedule.h"
#include "html/html.h"
#include "oled/oled.h"
#include "hmi/hmi.h"

const char *ssid     = "JardinDuCiel";
const char *password = "";

AsyncWebServer server(80);

Config   config("/config.ini");
Schedule schedule(SCHEDULE_FILE);

const char *ntpServer = "pool.ntp.org";

RCSwitch radioCmd = RCSwitch();

Hmi  hmi;
Cuve cuve(radioCmd);

time_t lastSec;
time_t lastMinute;

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

  // Initialiser la synchronisation de l'heure
  setSyncProvider(syncTimeFromRTC);

  if (!SPIFFS.begin()) {
    Serial.println("SPIFFS.begin() failed");
    display.displayError("SPIFFS.begin() failed");
    while (true) delay(100);
  }
  if (config.read() != true) {
    Serial.println("reading configuration failed");
    display.displayError("config read failed");
    while (true) delay(100);
  }
  config.print();
  if (schedule.read() != true) {
    Serial.println("schedule configuration failed");
    display.displayError("schedule read failed");
    while (true) delay(100);
  }
  schedule.print();

  hmi.setup();
  cuve.setup(true);

  pinMode(23, OUTPUT);          // sets the digital pin 23 as output
  radioCmd.enableTransmit(23);  // data de l'émetteur sur broche Digital 23
  // bien respecter l'ordre de ces 3 instructions qui suivent :
  radioCmd.setProtocol(1);       // à remplacer par votre valeur de protocol
  radioCmd.setPulseLength(346);  // à remplacer par votre valeur de PulseLength
  radioCmd.setRepeatTransmit(5);

  Serial.println("\n");

  Serial.println("Creation du point d'acces...");
  WiFi.softAP(ssid, password);

  Serial.print("Adresse IP: ");
  Serial.println(WiFi.softAPIP());

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
  server.onNotFound([](AsyncWebServerRequest *request) {
    request->send(200, "text/html", "/maint.html");
  });

  server.begin();

  Serial.println("HTTP server started");

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

void loop(void) {
  time_t currentTime = getRtcTime();

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

  hmi.run();

  delay(16);
}