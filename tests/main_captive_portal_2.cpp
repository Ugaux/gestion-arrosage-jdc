#include <WiFi.h>
#include <ESPAsyncWebServer.h>
#include <DNSServer.h>

const byte     DNS_PORT = 53;
IPAddress      apIP(192, 168, 4, 1);
DNSServer      dnsServer;
AsyncWebServer server(80);

// Captive portal HTML content
const char *portalHTML = R"rawliteral(
<!DOCTYPE html>
<html>
  <head><title>ESP32 Captive Portal</title></head>
  <body>
    <h1>Welcome to ESP32 Portal</h1>
    <p>This is a captive portal page.</p>
  </body>
</html>
)rawliteral";

void setup() {
  Serial.begin(115200);

  // Start AP
  WiFi.mode(WIFI_AP);
  WiFi.softAP("ESP32_Portal", "");  // Open network
  WiFi.softAPConfig(apIP, apIP, IPAddress(255, 255, 255, 0));
  WiFi.setSleep(false);

  // Start DNS server to redirect all queries to ESP32
  dnsServer.start(DNS_PORT, "*", apIP);

  // Common captive portal detection paths (Android, iOS, Windows)
  server.on("/generate_204", HTTP_GET, [](AsyncWebServerRequest *request) {
    request->send(200, "text/html", portalHTML);
  });

  server.on("/gen_204", HTTP_GET, [](AsyncWebServerRequest *request) {
    request->send(200, "text/html", portalHTML);
  });

  server.on("/connecttest.txt", HTTP_GET, [](AsyncWebServerRequest *request) {
    request->send(200, "text/plain", "Microsoft Connect Test");
  });

  server.on("/hotspot-detect.html", HTTP_GET, [](AsyncWebServerRequest *request) {
    request->send(200, "text/html", portalHTML);
  });

  server.on("/ncsi.txt", HTTP_GET, [](AsyncWebServerRequest *request) {
    request->send(200, "text/plain", "Microsoft NCSI");
  });

  // Catch-all for any other path
  server.onNotFound([](AsyncWebServerRequest *request) {
    request->send(200, "text/html", portalHTML);
  });

  server.begin();
  Serial.println("Captive portal running...");
}

void loop() {
  dnsServer.processNextRequest();  // Still needed even with AsyncWebServer
}
