#ifndef _WEBSERVER_H_
#define _WEBSERVER_H_

#include <ESPAsyncWebServer.h>

#include "watering/watering.h"

void handleRoot(AsyncWebServerRequest *request);
void handleAdd(AsyncWebServerRequest *request);
void handleEdit(AsyncWebServerRequest *request);
void handleEditSubmit(AsyncWebServerRequest *request);
void handleRemove(AsyncWebServerRequest *request);

void initWebServer();
void initWebSocket();
void cleanUpClient();
void notifyClients(String JSON_Data);
void handleWebSocketMessage(void *arg, uint8_t *data, size_t len);
void onEvent(AsyncWebSocket *server, AsyncWebSocketClient *client, AwsEventType type, void *arg, uint8_t *data, size_t len);
void htmlCreateWifi(String &message);
void htmlCreateValve(String &message);
void htmlCreateRelays(String &message);
void htmlCreateWaterings(String &message);
void updateWebSocketData(const char *op);
void updateWebSocketData(const char *op, Way *way);
void updateWebSocketData(const char *op, Relay *relay);
void updateWebSocketData(const char *op, Watering *w);

String templateProcessor(const String &var);

extern AsyncWebServer server;

#endif
