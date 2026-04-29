#include "WebServer.h"

#include <ArduinoJson.h>
#include <LittleFS.h>
#include <WiFi.h>

#include "hardware/Sensors.h"
#include "config/IniConfig.h"
#include "config/PreferencesManager.h"

#define TEST false

// Websocket
AsyncWebSocket ws("/ws");

// Webserver
AsyncWebServer server(80);

// Placeholders variables
String wifi;       // Wifi infos -> Test relays HTML page
String valve;      // Valves infos-> Test relays HTML page
String relays;     // Relays -> Test relays HTML page
String waterings;  // Waterings table -> Main HTML page

String always;     // Frequency (always, odd, even, custom) -> edit page
String even;       // Frequency (always, odd, even, custom) -> edit page
String odd;        // Frequency (always, odd, even, custom) -> edit page
String custom;     // Frequency (always, odd, even, custom) -> edit page
String isVisible;  // Hide / Show custom days -> edit page

char   way[MAX_DEF];          // Lawn.House
char   wayName[MAX_DEF];      // Way name -> add page and edit page
char   waySchedule[MAX_BUF];  // Index way -> add page  and edit page
char   adjustment[MAX_BUF];   // Display seasonal adjustment -> add and edit page
String days;                  // Display days selected (Mon, Wed...) -> add page and edit page
String startTime;             // Display start time and duration -> add page and edit page

/********** Templates processor **********/
String templateProcessor(const String &var) {
  if (var == "PLACEHOLDER_WIFI_INFO") {
    return wifi;
  }
  if (var == "PLACEHOLDER_VALVE_INFO") {
    return valve;
  } else if (var == "PLACEHOLDER_RELAYS") {
    return relays;
  } else if (var == "PLACEHOLDER_WATERINGS") {
    return waterings;
  } else if (var == "WAY_NAME") {
    return wayName;
  } else if (var == "WAY") {
    return way;
  } else if (var == "SCHEDULE") {
    return waySchedule;
  } else if (var == "ALWAYS_CHECKED") {
    return always;
  } else if (var == "EVEN_CHECKED") {
    return even;
  } else if (var == "ODD_CHECKED") {
    return odd;
  } else if (var == "CUSTOM_CHECKED") {
    return custom;
  } else if (var == "IS_VISIBLE") {
    return isVisible;
  } else if (var == "CUSTOM_DAYS") {
    return days;
  } else if (var == "START_TIME") {
    return startTime;
  } else if (var == "SEASONAL_ADJUSTMENT") {
    return adjustment;
  }
  return String();
}

/********** Handle ROOT **********/
void handleRoot(AsyncWebServerRequest *request) {
  Serial.printf("### ROOT\n");
  htmlCreateWifi(wifi);
  htmlCreateValve(valve);
  htmlCreateRelays(relays);
  htmlCreateWaterings(waterings);

  request->send(LittleFS, "/index.html", "text/html", false, templateProcessor);
}

/********** Handle ADD watering to a way **********/
void handleAdd(AsyncWebServerRequest *request) {
  if (request->hasParam("way")) {
    Watering *w = Watering::getFreeWatering(request->getParam("way")->value().c_str());
    if (w != 0) {
      snprintf(waySchedule, MAX_BUF, "%d", w->getIndex());
      strcpy(way, request->getParam("way")->value().c_str());
      snprintf(wayName, MAX_DEF, "<b>%s - %s</b>", w->getWay()->getZone()->getName(), w->getWay()->getShortName());
      snprintf(adjustment, MAX_BUF, "%d", prefManager.getInt("season_adj", 100));

      days = "";
      for (int i = 0; i < DAY_WEEK; i++) {
        days += "<div class='w3-cell'>";
        days += "<label class='custom-checkbox'>";
        days += "<input class='w3-check' name='day_";
        days += i;
        days += "'value='";
        days += i;
        days += "'type='checkbox'";
        days += "/>";
        days += "<i class='fa-solid fa-xmark-circle fa-2xl w3-text-gray unchecked'></i>";
        days += "<i class='fa-solid fa-check-circle fa-2xl w3-text-theme checked'></i>";
        days += "</label>";
        days += "</div>";

        startTime = "";
        startTime += "<div class='input-icons w3-col s8 w3-padding-right-8'>";
        startTime += "<i class=' fa-solid fa-clock icon'></i>";
        startTime += "<input name='start' id='start' class='w3-input w3-border w3-round input-field' type='text' placeholder='HH:MM' value='08:00' oninput='isTime(this)'>";
        startTime += "</div>";

        startTime += "<div class='input-icons w3-col s4'>";
        startTime += "<i class='fa-solid fa-stopwatch icon'></i>";
        startTime += "<input name='duration' id='duration' class='w3-input w3-border w3-round input-field' type='number' placeholder='1-120' value='15' oninput='isDuration(this)'>";
        startTime += "</div>";
      }
    }
  } else {
    Serial.printf("MISSING ARGUMENT(S)\n");
  }

  request->send(LittleFS, "/pages/add.html", "text/html", false, templateProcessor);
}

/********** Handle EDIT **********/
void handleEdit(AsyncWebServerRequest *request) {
  Watering *w = Watering::getByName((request->arg("way")).c_str(), request->arg("schedule").toInt());
  snprintf(waySchedule, MAX_DEF, "%d", w->getIndex());
  strcpy(way, request->getParam("way")->value().c_str());
  snprintf(wayName, MAX_DEF, "<b>%s - %s</b>", w->getWay()->getZone()->getName(), w->getWay()->getShortName());
  snprintf(adjustment, MAX_BUF, "%d", prefManager.getInt("season_adj", 100));

  always    = w->everyDays() ? "checked" : "";
  even      = w->evenDays() ? "checked" : "";
  odd       = w->oddDays() ? "checked" : "";
  custom    = w->customDays() ? "checked" : "";
  isVisible = w->customDays() ? "" : "hidden";

  days = "";
  for (int i = 0; i < DAY_WEEK; i++) {
    days += "<div class='w3-cell'>";
    days += "<label class='custom-checkbox'>";
    days += "<input class='w3-check' name='day_";
    days += i;
    days += "'value='";
    days += i;
    days += "'type='checkbox'";
    for (int n = 0; n < DAY_WEEK; n++) {
      if (strcmp(w->getWateringDay(n), "Sun") == 0 && (i == 0)) days += "checked";
      if (strcmp(w->getWateringDay(n), "Mon") == 0 && (i == 1)) days += "checked";
      if (strcmp(w->getWateringDay(n), "Tue") == 0 && (i == 2)) days += "checked";
      if (strcmp(w->getWateringDay(n), "Wed") == 0 && (i == 3)) days += "checked";
      if (strcmp(w->getWateringDay(n), "Thu") == 0 && (i == 4)) days += "checked";
      if (strcmp(w->getWateringDay(n), "Fri") == 0 && (i == 5)) days += "checked";
      if (strcmp(w->getWateringDay(n), "Sat") == 0 && (i == 6)) days += "checked";
    }
    days += "/>";
    days += "<i class='fa-solid fa-xmark-circle fa-2xl w3-text-gray unchecked'></i>";
    days += "<i class='fa-solid fa-check-circle fa-2xl w3-text-theme checked'></i>";
    days += "</label>";
    days += "</div>";

    startTime = "";
    startTime += "<div class='input-icons w3-col s8 w3-padding-right-8'>";
    startTime += "<i class=' fa-solid fa-clock icon'></i>";
    startTime += "<input name='start' id='start' class='w3-input w3-border w3-round input-field' type='text' placeholder='HH:MM' value='";
    startTime += w->getHourString();
    startTime += "'oninput='isTime(this)'>";
    startTime += "</div>";

    startTime += "<div class='input-icons w3-col s4'>";
    startTime += "<i class='fa-solid fa-stopwatch icon'></i>";
    startTime += "<input name='duration' id='duration' class='w3-input w3-border w3-round input-field' type='number' placeholder='1-120' value='";
    startTime += w->getDuration();
    startTime += "'oninput='isDuration(this)'>";
    startTime += "</div>";
  }

  request->send(LittleFS, "/pages/edit.html", "text/html", false, templateProcessor);
}

/********** Handle EDIT SUBMIT **********/
void handleEditSubmit(AsyncWebServerRequest *request) {
  String way;
  int    schedule = 0;
  String mode     = "*";
  String start, duration;
  char   days[DAY_WEEK][4] = { "", "", "", "", "", "", "" };

  if (request->hasParam("way") && request->hasParam("schedule") && request->hasParam("mode") && request->hasParam("adjustment") && request->hasParam("start") && request->hasParam("duration")) {

    way      = request->getParam("way")->value();
    schedule = request->getParam("schedule")->value().toInt();
    mode     = request->getParam("mode")->value();
    start    = request->getParam("start")->value();
    duration = request->getParam("duration")->value();

    request->hasParam("day_0") ? strcpy(days[0], "Sun") : "";
    request->hasParam("day_1") ? strcpy(days[1], "Mon") : "";
    request->hasParam("day_2") ? strcpy(days[2], "Tue") : "";
    request->hasParam("day_3") ? strcpy(days[3], "Wed") : "";
    request->hasParam("day_4") ? strcpy(days[4], "Thu") : "";
    request->hasParam("day_5") ? strcpy(days[5], "Fri") : "";
    request->hasParam("day_6") ? strcpy(days[6], "Sat") : "";

    // Check if at least on day is checked otherwise exit
    bool isDayEmpty      = true;
    int  countCustomDays = 0;
    for (int i = 0; i < DAY_WEEK; i++) {
      if (strcmp(days[i], "") != 0) {
        isDayEmpty = false;  // Found a non-null element, array is not empty
        countCustomDays++;
      }
    }

    Watering *w = Watering::getByName(way.c_str(), schedule);
    if (w != 0) {
      prefManager.setInt("season_adj", request->getParam("adjustment")->value().toInt());
      int hour, minute;
      sscanf(start.c_str(), "%d:%d", &hour, &minute);
      // If custom days mode and days[] is empty -> Set mode to always(*)
      if (isDayEmpty && strcmp(mode.c_str(), "c") == 0) mode = "*";

      // If number of custom days = 7 this means that's ferquency is always(*)
      if (countCustomDays == 7 && strcmp(mode.c_str(), "c") == 0) mode = "*";

      // Update
      w->set(hour, minute, duration.toInt(), true, mode.c_str(), days);
    } else {
      Serial.printf("%s NOT found\n", way.c_str());
    }
  } else {
    Serial.printf("MISSING ARGUMENT(S)\n");
  }
  request->redirect("/");
}

/********** Handle REMOVE watering from a way **********/
void handleRemove(AsyncWebServerRequest *request) {
  // DEBUG Serial.printf("### REMOVE WAY\n");

  if (request->hasParam("way") && request->hasParam("schedule")) {
    // Count scheduled waterings for this way
    int count = 0;
    for (int i = 0; i < MAX_SCHEDULE; i++) {
      Watering *w = Watering::getByName(request->getParam("way")->value().c_str(), i);
      if (w->getDuration() != 0) {
        count++;
      }
    }

    Watering *w = Watering::getByName(request->getParam("way")->value().c_str(), request->getParam("schedule")->value().toInt());

    // Don't allow to suppress the last watering of a way
    if (count > 1) {
      w->set(w->getWay()->getName(), request->getParam("schedule")->value().toInt(), 0, 0, 0);
    }
  } else {
    Serial.printf("MISSING ARGUMENT(S)\n");
  }
  request->redirect("/");
}

/********** Generate HTML code -> Wifi informations **********/
void htmlCreateWifi(String &s) {
  s = "";
  s += "<div class='w3-content w3-half'>";
  s += "<p style='margin: 15px 0 7px 0 '><i class='fa-solid fa-network-wired' style='width: 20px;'></i>&nbsp;";
  s += WiFi.SSID();
  s += "</p>";
  s += "<p style='margin: 7px 0'><i class='fa-solid fa-wifi' style='width: 20px;'></i>&nbsp;";
  s += WiFi.localIP().toString();
  s += "</p>";
  s += "</div><div class='w3-content w3-half'>";
  s += "<p style='margin: 7px 0'><i class='fa-solid fa-signal' style='width: 20px;'></i>&nbsp;";
  s += WiFi.RSSI();
  s += "&nbsp;dBm</p>";
  s += "<p style='margin: 7px 0 15px 0'><i class='fa-regular fa-clock' style='width: 20px;'></i> ";
  s += "<span id='relayTime'></span>";
  s += "</p></div>";
}

/********** Generate HTML code -> Valves **********/
void htmlCreateValve(String &s) {
  int duration;
  int mainValveOpenTime;

  duration          = Watering::getManualDuration();
  mainValveOpenTime = 20;

  s = "";
  s += "<p style='margin: 15px 0 7px 0'><i class='fa-regular fa-hourglass-half' style='width: 20px;'></i>&nbsp;Manual duration&nbsp;<b>";
  s += duration;
  s += "</b>&nbsp;min</p>";
  s += "<p style='margin: 7px 0 15px 0'><i class='fa-regular fa-stopwatch' style='width: 20px;'></i>&nbsp;Main valve open time&nbsp;<b>";
  s += mainValveOpenTime;
  s += "</b>&nbsps</p>";
}

/********** Generate HTML code -> Test relays **********/
void htmlCreateRelays(String &s) {
  s            = "";
  Relay *relay = Relay::getFirst();
  int    n     = 1;
  while (relay != 0) {
    if (relay->isPresent()) {
      String state = relay->getState() == ON ? "checked" : "";
      s += "<div class='w3-col' style='width: 33%; margin: 15px 0 0 0'>";
      s += "<h6><b>Relay #";
      s += n;
      s += "</b></h6><h6><small>";
      s += relay->getName();
      s += "</small></h6>";
      s += "<label class='switch'>";
      s += "<input name='checkbox_test' type='checkbox' id='btn_";
      s += relay->getName();
      s += "' ";
      s += state;
      s += "><span class='slider'></span></label></div>";
      n++;
    }
    relay = Relay::getNext();
  }
}

/********** Generate HTML code -> Waterings table **********/
void htmlCreateWaterings(String &s) {
  String lastWay;
  String lastZone;
  waterings = "";
  int schedule;

  Zone *z = Zone::getFirst();
  while (z != 0) {
    lastZone = z->getName();
    waterings += "<div class='w3-card w3-border w3-round w3-margin-top-7'>";
    waterings += "<header class='w3-container w3-border-bottom w3-theme' style='border-radius: 3px 3px 0px 0px; height: 22px'>";
    waterings += "<h6 style='margin: 2px;'><b>Zone ";
    waterings += z->getName();
    waterings += "</b></h6>";
    waterings += "</header>";

    Way *way = Way::getFirst();
    while (way != 0) {
      schedule = 1;
      if (lastZone == way->getZone()->getName()) {
        lastWay = way->getName();
        waterings += "<div class='w3-container w3-border-top w3-theme-l4'>";
        waterings += "<div class='w3-bar w3-small' style='height: 49px; padding: 8px 0px;'>";
        waterings += "<p class='w3-left'><a href='/add?way=";
        waterings += way->getName();
        waterings += "'><i class='fa-solid fa-circle-plus fa-xl w3-text-theme' style='margin-left:0px; width:30px;'></i></a><b class='w3-small'>";
        waterings += way->getShortName();
        waterings += "</b></p>";

        waterings += "<button name='manual_mode' id='btn_";
        waterings += way->getName();
        waterings += "' type='btn' class='w3-right w3-btn w3-theme w3-round' style='height: 33px; width: 50px;'>";
        if (!way->manualStarted(NULL)) {
          waterings += "<i class='fa-solid fa-play' aria-hidden='true'></i>";
        } else {
          waterings += "<i class='fa-solid fa-stop' aria-hidden='true'></i>";
        }
        waterings += "</button>";

        waterings += "<div class='input-icons w3-right w3-margin-right-8' style='width: 100px;'>";
        waterings += "<i class=' fa-solid fa-stopwatch icon'></i>";
        waterings += "<input id='m_duration_";
        waterings += way->getName();
        waterings += "' class='w3-input w3-border w3-round input-field' type='number' step='5' value='";
        waterings += Watering::getManualDuration();
        waterings += "'>";
        waterings += "</div>";

        waterings += "<p id='lbl_";
        waterings += way->getName();
        waterings += "' class='w3-right w3-margin-right-8'></p>";

        waterings += "</div></div>";
        waterings += "<div class='w3-container w3-border-top'>";
        waterings += "<p class='w3-small w3-text-gray'><b><i id='lbl_next_";
        waterings += way->getName();
        waterings += "'>&nbsp;</i></b></p>";
        waterings += "</div>";

        for (int i = 0; i < MAX_WATERING; i++) {
          Watering *w = Watering::getWatering(i);
          if (w->getDuration() != 0) {
            if (lastWay == w->getWayName()) {
              waterings += "<div class='w3-container w3-border-top'>";
              waterings += "<a href='/edit?way=";
              waterings += w->getWayName();
              waterings += "&schedule=";
              waterings += w->getIndex();
              waterings += "'>";
              waterings += "<p class='w3-small w3-left' style='margin-bottom: 0px;'><i class='fa-solid fa-clock'></i><b>&nbsp;Schedule&nbsp;";
              waterings += schedule;
              waterings += "</b></p></a>";
              waterings += "<label class='switch w3-right w3-margin-top-6'>";                // Enable switch
              waterings += "<input name='checkbox_enable' type='checkbox' id='btn_enable_";  // Enable switch
              waterings += w->getWayName();                                                  // Enable switch
              waterings += "_";                                                              // Enable switch
              waterings += w->getIndex();                                                    // Enable switch
              waterings += "'";                                                              // Enable switch
              waterings += w->getEnable() ? "checked" : "";                                  // Enable switch
              waterings += ">";                                                              // Enable switch
              waterings += "<span class='slider'></slider>";                                 // Enable switch
              waterings += "</label>";                                                       // Enable switch
              waterings += "<div class='w3-row'>";
              waterings += "<div class='w3-col' style='width:80px'>";
              waterings += "<p class='w3-margin-4 w3-text-gray'><span class='w3-small'>Start time :</span></p>";
              waterings += "<p class='w3-margin-4 w3-text-gray'><span class='w3-small'>Duration :</span></p>";
              waterings += "<p class='w3-margin-4 w3-text-gray'><span class='w3-small'>Frequency :</span></p>";
              waterings += "</div>";
              waterings += "<div class='w3-rest'>";
              waterings += "<p class='w3-margin-4'>";
              waterings += w->getHourString();
              waterings += "</p>";
              waterings += "<p class='w3-margin-4'>";
              waterings += w->getDuration();
              waterings += "&nbsp;min</p>";
              waterings += "<p class='w3-margin-4'>";
              waterings += w->getFrequency();
              waterings += "</p>";
              waterings += "</div></div>";

              waterings += "</div>";
              schedule++;
              lastWay == w->getWayName();
            }
          }
        }
      }
      way = Way::getNext();
    }
    z = Zone::getNext();
    waterings += "</div>";
  }
}

/********** DATE & SENSORS & MANUAL TIMERS & WATERING STATUS - Send updates to websocket website **********/
void updateWebSocketData(const char *op) {

  StaticJsonDocument<200> doc;
  String                  data;

  // Update DATE TIME on website
  if (strcmp(op, "date_time") == 0) {
    // DEBUG Serial.print("*** DATE_TIME\n");
    struct tm  *pTime;
    time_t      now = time(NULL);
    static char date[MAX_BUF];
    static char time[MAX_BUF];
    pTime = localtime(&now);
    strftime(date, MAX_BUF, "%a %d %b %y", pTime);  // Thu 24 Apr 2021
    strftime(time, MAX_BUF, "%H:%M", pTime);        // 12:30

    doc["update"] = "date_time";
    doc["date"]   = date;
    doc["time"]   = time;
    serializeJson(doc, data);
    notifyClients(data);
  }

  // Update SENSORS values on website
  else if (strcmp(op, "sensors") == 0) {
    // DEBUG Serial.print("*** SENSORS\n");
    int   moisture;
    float flow;
    char  buf[MAX_BUF];
    getSoilMoisture(&moisture);
    flow = getFlow();
    snprintf(buf, MAX_BUF, "%.2f", flow);  // display -> X.XX
    doc["update"] = "sensors";
    IniConfig::getConfig()->getMoistureSensor() == 0 ? doc["moisture"] = "No sensor" : doc["moisture"] = moisture;
    IniConfig::getConfig()->getFlowSensor() == 0 ? doc["flow"] = "No sensor" : doc["flow"] = buf;
    doc["adjustment"] = prefManager.getInt("season_adj", 100);
    serializeJson(doc, data);
    notifyClients(data);

  }

  // Update TIMERS values on website on manual mode
  else if (strcmp(op, "manual_in_progress") == 0) {
    // DEBUG Serial.print("*** MANUAL_MODE\n");
    String s;
    if (Way::isAnyManualWateringRunning(s)) {
      // DEBUG Serial.printf("manual in progress: %s\n", s.c_str());
      doc["update"] = "manual_in_progress";
      doc["ways"]   = s.c_str();
      serializeJson(doc, data);
      notifyClients(data);
    }
  }

  // Update WATERINGS STATUS
  // Next irrigation -> way, start time, duration
  // Watering in progress -> way, remaining time
  // Manual mode -> way, remaining time
  // No scheduled watering -> no data to display
  else if (strcmp(op, "watering_status") == 0) {
    // Watering in progress - Manual mode
    String    s;
    time_t    t;
    Watering *w;

    if (Way::isAnyManualWateringRunning(s)) {
      doc["update"] = "watering_status";
      doc["status"] = "Manual watering";
      doc["infos"]  = s.c_str();
      serializeJson(doc, data);
      notifyClients(data);
    }

    // Watering in progress - Auto mode
    else if (Watering::isAnyAutoWateringRunning(s)) {
      doc["update"] = "watering_status";
      doc["status"] = "Watering in progress";
      doc["infos"]  = s.c_str();
      serializeJson(doc, data);
      notifyClients(data);
    }

    // Next watering
    else if (Watering::getNextWateringTime(&w, &t) && !Watering::isAnyWateringRunning()) {
      struct tm *pTime;
      char       duration[MAX_BUF];

      if (Watering::getNextWateringTime(&w, &t)) {
        char buffer[MAX_BUF];
        pTime = localtime(&t);
        strftime(buffer, MAX_BUF, "%a %d %b %y %H:%M", pTime);    // Thu 24 Apr 2021 12:30
        snprintf(duration, MAX_BUF, "%d min", w->getDuration());  // 10 min.
        doc["update"] = "watering_status";
        doc["status"] = "Next irrigation";
        doc["line_1"] = w->getWay()->getShortName();
        doc["line_2"] = buffer;
        doc["line_3"] = duration;
        serializeJson(doc, data);
        notifyClients(data);
      }
    }

    // No scheduled watering
    else {
      doc["update"] = "watering_status";
      doc["status"] = "No scheduled irrigation(s)";
      doc["line_1"] = "";
      doc["line_2"] = "Add irrigation(s)";
      doc["line_3"] = "";
      serializeJson(doc, data);
      notifyClients(data);
    }
  }

  // Update NEXT WATERING for each way on website
  else if (strcmp(op, "next_watering") == 0) {
    String s;
    if (Watering::getNextWayWateringTime(s)) {
      // DEBUG Serial.printf("next way watering start tim: %s\n", s.c_str());
      doc["update"] = "next_watering";
      doc["infos"]  = s.c_str();
      serializeJson(doc, data);
      notifyClients(data);
    }
  }
}

/********** TESY RELAYS - Send updates to websocket website **********/
void updateWebSocketData(const char *op, Relay *relay) {

  StaticJsonDocument<200> doc;
  String                  data;

  if (strcmp(op, "test_relay") == 0) {
    // DEBUG Serial.print("*** TEST_RELAY\n");
    if (relay != 0) {
      if (relay->isPresent()) {
        doc["update"] = "test_relay";
        doc["relay"]  = relay->getName();
        doc["state"]  = relay->getState();
        data          = "";
        serializeJson(doc, data);
        notifyClients(data);
      }
    }
  }
}

/********** MANUAL MODE - Send updates to websocket website **********/
void updateWebSocketData(const char *op, Way *way) {

  StaticJsonDocument<200> doc;
  String                  data;

  if (strcmp(op, "manual_mode") == 0) {
    // DEBUG Serial.printf("*** MANUAL MODE\n");
    if (way != 0) {
      doc["update"]    = "manual_mode";
      doc["way"]       = way->getName();
      doc["state"]     = way->manualStarted(NULL) ? "start" : "stop";
      doc["operation"] = !way->manualStarted(NULL) ? "<i class='fa-solid fa-play' aria-hidden='true'></i>" : "<i class='fa-solid fa-stop' aria-hidden='true'></i>";
      data             = "";
      serializeJson(doc, data);
      notifyClients(data);
    }
  }
}

/********** Enable and disable buttons - Send updates to websocket website **********/
void updateWebSocketData(const char *op, Watering *w) {

  StaticJsonDocument<200> doc;
  String                  data;

  if (strcmp(op, "enable_button") == 0) {
    // DEBUG Serial.printf("*** ENABLE BUTTON\n");
    if (way != 0) {
      doc["update"] = "enable_button";
      doc["way"]    = w->getWayName();
      doc["index"]  = w->getIndex();
      doc["state"]  = w->getEnable();
      data          = "";
      serializeJson(doc, data);
      notifyClients(data);
    }
  }
}

/********** Handle websocket requests from ESP32 **********/
void handleWebSocketMessage(void *arg, uint8_t *data, size_t len) {
  AwsFrameInfo *info = (AwsFrameInfo *)arg;
  if (info->final && info->index == 0 && info->len == len && info->opcode == WS_TEXT) {
    data[len] = 0;

    StaticJsonDocument<200> doc;

    DeserializationError error = deserializeJson(doc, data);
    if (error) {
      Serial.print(F("deserializeJson() failed: "));
      Serial.println(error.f_str());
      return;
    }

    const char *update = doc["update"];

    // Test relays
    if (strcmp(update, "test_relay") == 0) {
      Relay *relay = Relay::getByName(doc["relay"]);
      if (relay != 0) {
        if (relay->getState() == ON) {
          relay->off();
        } else {
          relay->on();
        }
        updateWebSocketData(update, relay);
      }
    }

    // Enable buttons
    else if (strcmp(update, "enable_button") == 0) {
      Watering *w = Watering::getByName(doc["way"], atoi(doc["index"]));
      w->toggleEnable();
      // DEBUG Serial.printf("m_enable: %d", w->getEnable());
      updateWebSocketData(update, w);
    }

    // Date & time
    else if (strcmp(update, "date_time") == 0) {
      updateWebSocketData(update);
    }

    // Humidity an flow sensor's values
    else if (strcmp(update, "sensors") == 0) {
      updateWebSocketData(update);
    }

    // Manual mode START & STOP
    else if (strcmp(update, "manual_mode") == 0) {
      Watering *w = Watering::getByName(doc["way"], 0);
      if (strcmp(doc["operation"], "start") == 0) {
        w->getWay()->manualStart(doc["duration"]);
      } else {
        w->getWay()->manualStop();
      }
      updateWebSocketData(update, w->getWay());
      updateWebSocketData("test_relay", w->getWay()->getRelay());  // Update test relays buttons
    }

    // Manual in progress -> Get remain time
    else if (strcmp(update, "manual_in_progress") == 0) {
      updateWebSocketData(update);
    }

    // Watering status (Next, in progress, manual, no scheduled)
    else if (strcmp(update, "watering_status") == 0) {
      updateWebSocketData(update);
    }

    // Next watering time for each way
    else if (strcmp(update, "next_watering") == 0) {
      updateWebSocketData(update);
    }
  }
}

/********** Events function **********/
void onEvent(AsyncWebSocket *server, AsyncWebSocketClient *client, AwsEventType type,
             void *arg, uint8_t *data, size_t len) {
  switch (type) {
    case WS_EVT_CONNECT:
      Serial.printf("WebSocket client #%u connected from %s\n", client->id(), client->remoteIP().toString().c_str());
      break;
    case WS_EVT_DISCONNECT:
      Serial.printf("WebSocket client #%u disconnected\n", client->id());
      break;
    case WS_EVT_DATA:
      handleWebSocketMessage(arg, data, len);
      break;
    case WS_EVT_PONG:
    case WS_EVT_ERROR:
      break;
  }
}

/********** Web socket notification - Test relays **********/
void notifyClients(String JSON_Data) {
  ws.textAll(JSON_Data);
}

void handleApiState(AsyncWebServerRequest *request) {
  StaticJsonDocument<770> doc;

  doc["wifi"] = "OnlyJesusSaves";

  JsonArray relayArr = doc.createNestedArray("relays");
  Relay    *relay    = Relay::getFirst();
  int       n        = 1;
  while (relay != 0) {
    if (relay->isPresent()) {
      String     state     = relay->getState() == ON ? "ON" : "OFF";
      JsonObject relay_obj = relayArr.createNestedObject();
      relay_obj["id"]      = n;
      relay_obj["name"]    = relay->getName();
      relay_obj["state"]   = state;
      n++;
    }
    relay = Relay::getNext();
  }

  String response;
  serializeJson(doc, response);
  request->send(200, "application/json", response);

  AsyncWebServerResponse *res = request->beginResponse(200, "application/json", response);
  res->addHeader("Access-Control-Allow-Origin", "*");
  res->addHeader("Access-Control-Allow-Methods", "GET, POST, PUT, DELETE, OPTIONS");
  res->addHeader("Access-Control-Allow-Headers", "Content-Type, Authorization");

  request->send(res);
}

/********** Websocket initialization - Test relays **********/
void initWebServer() {
  server.on("/api/state", HTTP_GET, handleApiState);

  // Main HTML page
  server.on("/", handleRoot);

  // Edit Schedules page
  server.on("/add", handleAdd);
  server.on("/edit", handleEdit);
  server.on("/edit_submit", handleEditSubmit);
  server.on("/remove", handleRemove);

  // Files CSS
  server.on("/w3.css", HTTP_GET, [](AsyncWebServerRequest *request) {
    request->send(LittleFS, "/css/w3.css", "text/css");
  });
  server.on("/w3-theme.css", HTTP_GET, [](AsyncWebServerRequest *request) {
    request->send(LittleFS, "/css/w3-theme.css", "text/css");
  });
  server.on("/style.css", HTTP_GET, [](AsyncWebServerRequest *request) {
    request->send(LittleFS, "/css/style.css", "text/css");
  });
  server.on("/edit.css", HTTP_GET, [](AsyncWebServerRequest *request) {
    request->send(LittleFS, "/css/edit.css", "text/css");
  });
  server.on("/fa-all.min.css", HTTP_GET, [](AsyncWebServerRequest *request) {
    request->send(LittleFS, "/fa/all.min.css", "text/css");
  });

  // Files WOFF2
  server.on("/webfonts/fa-regular-400.woff2", HTTP_GET, [](AsyncWebServerRequest *request) {
    request->send(LittleFS, "/fa/regular-400.woff2", "font/woff2");
  });
  server.on("/webfonts/fa-solid-900.woff2", HTTP_GET, [](AsyncWebServerRequest *request) {
    request->send(LittleFS, "/fa/solid-900.woff2", "font/woff2");
  });
  server.on("/webfonts/fa-v4compatibility.woff2", HTTP_GET, [](AsyncWebServerRequest *request) {
    request->send(LittleFS, "/fa/v4compatibility.woff2", "font/woff2");
  });

  // Files JS
  server.on("/scripts.js", HTTP_GET, [](AsyncWebServerRequest *request) {
    request->send(LittleFS, "/js/scripts.js", "text/javascript");
  });
  server.on("/edit.js", HTTP_GET, [](AsyncWebServerRequest *request) {
    request->send(LittleFS, "/js/edit.js", "text/javascript");
  });

  // Captive portal
  //server.onNotFound([](AsyncWebServerRequest *request) {
  //  request->send(200, "text/html", "/");
  //});

  // Start AsyncWebServer
  server.begin();
}

/********** Websocket initialization - Test relays **********/
void initWebSocket() {
  ws.onEvent(onEvent);
  server.addHandler(&ws);
}

/********** Websocket delete inactive clients - Test relays **********/
void cleanUpClient() {
  ws.cleanupClients();
}
