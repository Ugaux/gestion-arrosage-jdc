// webServer.send(200, "text/html", "<h1>Tapez www.jardinduciel.local dans un navigateur pour accéder à l'interface de gestion de l'arrosage du Jardin du Ciel.</h1>");

#include <WiFi.h>
#include <WebServer.h>
#include <DNSServer.h>
#include <Preferences.h>

#define AP_SSID "ESP32_Captive"
#define AP_PASS ""

const byte  DNS_PORT = 53;
DNSServer   dnsServer;
WebServer   server(80);
Preferences preferences;

IPAddress apIP(192, 168, 4, 1);

String currentClientMAC;

// Convert MAC bytes to readable string
String macToString(const uint8_t* mac) {
  char macStr[18];
  sprintf(macStr, "%02X:%02X:%02X:%02X:%02X:%02X",
          mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
  return String(macStr);
}

// Handle new device connection (get MAC)
void WiFiEvent(WiFiEvent_t event, WiFiEventInfo_t info) {
  if (event == SYSTEM_EVENT_AP_STACONNECTED) {
    currentClientMAC = macToString(info.sta_connected.mac);
    Serial.println("Device connected: " + currentClientMAC);
  }
}

// Check if device is remembered
bool isRememberedDevice(String mac) {
  preferences.begin("known-devices", true);
  int count = preferences.getUInt("count", 0);
  for (int i = 0; i < count; i++) {
    const char* key    = ("mac" + String(i)).c_str();
    String      stored = preferences.getString(key, "");
    if (stored == mac) {
      preferences.end();
      return true;
    }
  }
  preferences.end();
  return false;
}

// Save new MAC to list
void rememberDevice(String mac) {
  preferences.begin("known-devices", false);
  int count = preferences.getUInt("count", 0);
  preferences.putString("mac" + String(count), mac);
  preferences.putUInt("count", count + 1);
  preferences.end();
  Serial.println("Remembered new device: " + mac);
}

// Captive portal root
void handleRoot() {
  if (isRememberedDevice(currentClientMAC)) {
    server.send(200, "text/plain", "Welcome back! You're a known device.");
  } else {
    server.send(200, "text/html",
                "<h2>Captive Portal</h2><p>You're not recognized. <a href=\"/remember\">Click here to be remembered</a></p>");
  }
}

// Endpoint to remember a device
void handleRemember() {
  rememberDevice(currentClientMAC);
  server.send(200, "text/html", "<h2>Device remembered!</h2><p>Next time, you won't see this page.</p>");
}

// Redirect all unknown routes
void handleRedirect() {
  server.sendHeader("Location", "http://192.168.4.1/", true);
  server.send(302, "text/plain", "");
}

void setup() {
  Serial.begin(115200);
  WiFi.mode(WIFI_AP);
  WiFi.softAP(AP_SSID, AP_PASS);
  WiFi.onEvent(WiFiEvent);
  WiFi.softAPConfig(apIP, apIP, IPAddress(255, 255, 255, 0));

  dnsServer.start(DNS_PORT, "*", apIP);

  server.on("/", handleRoot);
  server.on("/remember", handleRemember);
  server.onNotFound(handleRedirect);
  server.begin();

  Serial.println("Captive portal started. Connect to SSID: " + String(AP_SSID));
}

void loop() {
  dnsServer.processNextRequest();
  server.handleClient();
}
