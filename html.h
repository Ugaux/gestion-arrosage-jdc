
#ifndef _HTML_H_
#define _HTML_H_

#include <ESPAsyncWebServer.h>
#include <SPIFFS.h>

extern AsyncWebServer server;

void handleRoot(AsyncWebServerRequest *request);
void handleConfigure(AsyncWebServerRequest *request);
void handleConfigureSubmit(AsyncWebServerRequest *request);
void handleNew(AsyncWebServerRequest *request);
void handleNewSubmit(AsyncWebServerRequest *request);
void handleManual(AsyncWebServerRequest *request);
void handleManualInProgress(AsyncWebServerRequest *request);
void handleTest(AsyncWebServerRequest *request);

#endif
