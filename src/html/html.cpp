#include <Arduino.h>

#include "../html/html.h"
#include "../watering/watering.h"

String formWayName;
int    formSchedule;
String formTime;
String formDuration;
String formAlways;
String formOptions;

char head[] = R"(<!DOCTYPE html><html>
<head><meta name="viewport" content="width=device-width, initial-scale=1">
</head>
<meta charset="utf-8">
)";

char buttonCss[] = R"(<style>html { font-family: Helvetica; display: inline-block; margin: 0px auto; text-align: center;}
.button { background-color: #4CAF50; border: none; color: white; padding: 16px 40px; text-decoration: none; font-size: 15px; margin: 2px; cursor: pointer;}
.button2 {background-color: #555555;}</style>
)";

void addConfigButton(String &html, const char *way, int schedule) {
  html += "<p><button id='config_";
  html += way;
  html += "'";
  html += "onclick=\"window.location.href='/configure?way=";
  html += way;
  html += "&schedule=";
  html += schedule;
  html += "'\">";
  html += "CONFIGURE";
  html += "</button>\n";
  html += "</p>\n";
}

void addStartButton(String &html, const char *way, const char *title) {
  html += "<p><button id='btn_";
  html += way;
  html += "'";
  html += "onclick=manual('";
  html += way;
  html += "')>";
  html += title;
  html += "</button>\n";
  html += "<label style=\"font-size: 10px ;\" id='lbl_";
  html += way;
  html += "'";
  html += "</p>\n";
}

void addNewButton(String &html) {
  html += "<p><button id='new'";
  html += "onclick=\"window.location.href='/new'\">";
  html += "AJOUTER";
  html += "</button>\n";
  html += "</p>\n";
}

String wateringsTable;

void displayParams(AsyncWebServerRequest *request) {
  int paramsNr = request->params();
  Serial.println(paramsNr);
  for (int i = 0; i < paramsNr; i++) {
    const AsyncWebParameter *p = request->getParam(i);
    Serial.printf("%s=%s\n", p->name().c_str(), p->value().c_str());
  }
}

String templateProcessor(const String &var) {
  if (var == "PLACEHOLDER_MANUAL_DURATION") {
    return String(Watering::manualDuration());
  } else if (var == "PLACEHOLDER_WATERINGS") {
    return wateringsTable;
  } else if (var == "WAY_NAME") {
    return formWayName;
  } else if (var == "WAY_SCHEDULE") {
    return String(formSchedule);
  } else if (var == "WAY_TIME") {
    return formTime;
  } else if (var == "WAY_DURATION") {
    return formDuration;
  } else if (var == "WAY_ALWAYS") {
    return formAlways;
  } else if (var == "OPTIONS") {
    return formOptions;
  }
  return "";
}

void handleRoot(AsyncWebServerRequest *request) {
  Serial.printf("### ROOT\n");
  String manualoperation;
  String manualWatering;
  int    manualDuration;
  if (request->hasParam("op")) {
    manualoperation = request->arg("op");
    manualWatering  = request->arg("way");
    Watering *w     = Watering::getByName(manualWatering.c_str(), 0);
    if (manualoperation == "start") {
      manualDuration = request->arg("duration").toInt();
      Serial.printf("manual %s %d\n", manualWatering.c_str(), manualDuration);
      w->getWay()->manualStart(manualDuration);
    } else {
      w->getWay()->manualStop();
    }
  }
  String      lastWay;
  const char *trStyle[] = { "<tr>\n", "<tr style=\"background-color:#ecffb3;\">\n" };
  int         trIndex   = 0;
  wateringsTable        = "";
  int schedule          = 0;
  for (int i = 0; i < MAX_WATERING; i++) {
    Watering *w = Watering::getWatering(i);
    if (w->getDuration() != 0) {
      if (lastWay != w->getWayName()) {
        trIndex = trIndex == 0 ? 1 : 0;
      }
      wateringsTable += trStyle[trIndex];
      wateringsTable += "<td>";
      wateringsTable += w->getWayName();
      wateringsTable += "</td>\n";
      wateringsTable += "<td>";
      wateringsTable += w->getHourString();
      wateringsTable += "</td>\n";
      wateringsTable += "<td>";
      wateringsTable += w->getDuration();
      wateringsTable += "</td>\n";
      wateringsTable += "<td>";
      wateringsTable += w->always() ? "OUI" : "NON";
      wateringsTable += "</td>\n";
      wateringsTable += "<td>";
      if (lastWay != w->getWayName()) {
        schedule = 0;
      }
      addConfigButton(wateringsTable, w->getWayName(), schedule);
      wateringsTable += "</td>\n";
      wateringsTable += "<td style=\"border-left: 1px solid #BDB76B;\">";
      if (lastWay != w->getWayName()) {
        if (!w->getWay()->manualStarted(NULL)) {
          Serial.printf("%s: automatic mode\n", w->getWayName());
          addStartButton(wateringsTable, w->getWayName(), "DEMARRER");
        } else {
          Serial.printf("%s: manual mode\n", w->getWayName());
          addStartButton(wateringsTable, w->getWayName(), "ARRETER");
        }
        wateringsTable += "</td>\n";
        lastWay = w->getWayName();
      } else {
        wateringsTable += "&nbsp";
        wateringsTable += "</td>\n";
      }
      schedule++;
      wateringsTable += "</tr>\n";
    }
  }
  wateringsTable += trStyle[trIndex];
  wateringsTable += "<td>";
  addNewButton(wateringsTable);
  wateringsTable += "</td>&nbsp<td>&nbsp</td><td>&nbsp</td><td>&nbsp</td><td>&nbsp</td><td style=\"border-left: 1px solid #BDB76B;\"></td>";
  wateringsTable += "</tr>\n";
  wateringsTable += "</table>";
  request->send(SPIFFS, "/index.html", "text/html", false, templateProcessor);
}

void handleConfigure(AsyncWebServerRequest *request) {
  formWayName  = request->arg("way");
  formSchedule = request->arg("schedule").toInt();
  Serial.printf("### CONFIGURE %s %d\n", formWayName.c_str(), formSchedule);
  Watering *w  = Watering::getByName(formWayName.c_str(), formSchedule);
  formTime     = w->getHourString();
  formDuration = w->getDuration();
  formAlways   = w->always() ? "checked" : "";
  request->send(SPIFFS, "/configure.html", "text/html", false, templateProcessor);
}

void handleConfigureSubmit(AsyncWebServerRequest *request) {
  String way;
  int    schedule = 0;
  String time;
  String duration;
  String always = "off";

  if (request->hasParam("way") && request->hasParam("schedule") && request->hasParam("time") && request->hasParam("duration")) {
    way      = request->getParam("way")->value();
    schedule = request->getParam("schedule")->value().toInt();
    time     = request->getParam("time")->value();
    duration = request->getParam("duration")->value();
    if (request->hasParam("always")) {
      always = request->getParam("always")->value();
    }
    Serial.printf("### CONFIGURE SUBMIT %s %d\n", way.c_str(), schedule);
    Serial.printf("way=%s\n", way.c_str());
    Serial.printf("time=%s\n", time.c_str());
    Serial.printf("duration=%s\n", duration.c_str());
    Serial.printf("always=%s\n", always.c_str());
    Watering *w = Watering::getByName(way.c_str(), schedule);
    if (w != 0) {
      int hour, minute;
      sscanf(time.c_str(), "%d:%d", &hour, &minute);
      w->set(hour, minute, duration.toInt(), always == "on" ? true : false);
    } else {
      Serial.printf("%s NOT found\n", way.c_str());
    }
  } else {
    Serial.printf("MISSING ARGUMENT(S)\n");
  }
  request->redirect("/");
}

void handleNew(AsyncWebServerRequest *request) {
  Serial.printf("### NEW\n");
  Way *way    = Way::getFirst();
  formOptions = "";
  while (way != 0) {
    Serial.printf("way=%s\n", way->getName());
    if (way->getName() == 0) {
      break;
    }
    formOptions += "<option value=\"";
    formOptions += way->getName();
    formOptions += "\">";
    formOptions += way->getName();
    formOptions += "</option>";
    way = Way::getNext();
  }
  request->send(SPIFFS, "/new.html", "text/html", false, templateProcessor);
}

void handleNewSubmit(AsyncWebServerRequest *request) {
  String way;
  int    schedule = 0;
  String time;
  String duration;
  String always = "off";

  displayParams(request);
  if (request->hasParam("way") && request->hasParam("time") && request->hasParam("duration")) {
    way      = request->getParam("way")->value();
    time     = request->getParam("time")->value();
    duration = request->getParam("duration")->value();
    if (request->hasParam("always")) {
      always = request->getParam("always")->value();
    }
    Serial.printf("### NEW SUBMIT %s %d\n", way.c_str(), schedule);
    Serial.printf("way=%s\n", way.c_str());
    Serial.printf("time=%s\n", time.c_str());
    Serial.printf("duration=%s\n", duration.c_str());
    Serial.printf("always=%s\n", always.c_str());
    Watering *w = Watering::getFreeWatering(way.c_str());
    if (w != 0) {
      int hour, minute;
      sscanf(time.c_str(), "%d:%d", &hour, &minute);
      w->set(hour, minute, duration.toInt(), always == "on" ? true : false);
    } else {
      Serial.printf("FREE WATERING NOT found\n");
    }
  } else {
    Serial.printf("MISSING ARGUMENT(S)\n");
  }
  request->redirect("/");
}

void handleTest(AsyncWebServerRequest *request) {
  Serial.printf("### TEST\n");
  String message = head;
  message += "<body><h1>RELAIS</h1>\n";
  message += buttonCss;
  Relay *relay = Relay::getFirst();
  while (relay != 0) {
    if (relay->isPresent()) {
      String state = relay->getState() == OFF ? "OFF" : "ON";
      message += "<p>";
      message += relay->getName();
      message += " : " + state + "</p>";
      if (relay->getState() == OFF) {
        message += "<p><a href=\"/on?relay=";
        message += relay->getName();
        message += "\"><button class=\"button\">ON</button></a></p>";
      } else {
        message += "<p><a href=\"/off?relay=";
        message += relay->getName();
        message += "\"><button class=\"button button2\">OFF</button></a></p>";
      }
    }
    relay = Relay::getNext();
  }
  message += "</body></html>";
  request->send(200, "text/html", message);
}

void handleManual(AsyncWebServerRequest *request) {
  handleRoot(request);
}

void handleManualInProgress(AsyncWebServerRequest *request) {
  String s;
  if (Way::isAnyManualWateringRunning(s)) {
    Serial.printf("handleManualInProgress found %s\n", s.c_str());
    request->send(200, "text/plain", s);
  } else {
    request->send(200, "text/plain", "none");
  }
}
