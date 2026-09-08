#include "HttpRouter.h"

#include <ESPAsyncWebServer.h>

void HttpRouter::begin(AsyncWebServer &server)
{
  log_d("HttpRouter is initializing...");

  registerStaticRoutes(server);
  registerApiRoutes(server);
  registerCaptivePortalRoutes(server);

  log_i("HttpRouter has initialized");
}

void HttpRouter::registerCaptivePortalRoutes(AsyncWebServer &server)
{
  // /generate_204, /ncsi.txt, etc.

  // Captive-portal detection URLs
  server.on("/generate_204", HTTP_GET,
            [this](AsyncWebServerRequest *request)
            {
              handleRoot(request);
            });
  server.on("/hotspot-detect.html", HTTP_GET,
            [this](AsyncWebServerRequest *request)
            {
              handleRoot(request);
            });
  server.on("/connecttest.txt", HTTP_GET,
            [this](AsyncWebServerRequest *request)
            {
              handleRoot(request);
            });
  server.on("/ncsi.txt", HTTP_GET,
            [this](AsyncWebServerRequest *request)
            {
              handleRoot(request);
            });
}
