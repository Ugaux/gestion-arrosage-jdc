#include <Arduino.h>
#include <WiFi.h>
#include "RTCModule.h"

#include "constants.h"
#include "watering.h"
#include "oled.h"

// Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);
Oled display;

Oled::Oled() : Adafruit_SSD1306(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET) {
}

bool Oled::begin(void) {
  if (!Adafruit_SSD1306::begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS)) {
    return false;
  }
  clearDisplay();
  setTextColor(SSD1306_WHITE);
  setCursor(0, 0);
  char     title[] = "MON IRRIGATION";
  int16_t  x1, y1;
  uint16_t w, h;
  getTextBounds(title, 0, 0, &x1, &y1, &w, &h);
  setCursor((SCREEN_WIDTH - w) / 2, 0);
  print(title);
  getTextBounds("pt", 0, 0, &x1, &y1, &w, &m_textHeight);
  Serial.printf("Oled::begin %u\n", m_textHeight);
  display();
  return true;
}

uint16_t Oled::getLinePos(int line) {
  return m_textHeight * (3 + line) + (3 * line);
}

void Oled::clearLine(int line) {
  uint8_t y = getLinePos(line);
  fillRect(0, y, SCREEN_WIDTH, getTextHeight(), SSD1306_BLACK);
}

void Oled::displayTimeDate() {
  char timeString[MAX_BUF];

  DateTime   now         = getCurrentTime();                  // Obtenir l'heure actuelle du RTC
  time_t     currentTime = now.unixtime();                    // Convertir en Unix timestamp
  struct tm *timeinfo    = localtime(&currentTime);           // Convertir en structure tm locale
  strftime(timeString, MAX_BUF, "%d/%m/%Y %H:%M", timeinfo);  // Formater la chaîne de temps

  Serial.printf("Oled::displayTimeDate %s\n", timeString);
  int16_t  x1, y1;
  uint16_t w, h;
  getTextBounds(timeString, 0, 0, &x1, &y1, &w, &h);
  fillRect(0, 0, SCREEN_WIDTH, 16, SSD1306_BLACK);  // Effacer la ligne précédente
  setCursor((SCREEN_WIDTH - w) / 2, 0);             // Centrer le texte
  print(timeString);
  display();
}

void Oled::displayMoisture(int moisture) {
  uint16_t y = getLinePos(0);
  fillRect(0, y, 50, m_textHeight, SSD1306_BLACK);
  clearLine(0);
  setCursor(0, y);
  printf("HUMIDITE:%d%%", moisture);
  display();
}

void Oled::displayFlow(float flow) {
  uint16_t y = getLinePos(1);
  fillRect(0, y, 50, m_textHeight, SSD1306_BLACK);
  setCursor(0, y);
  printf("DEBIT:%.2fL", flow);
  display();
}

void Oled::displayMessage(const char *msg) {
  uint16_t y = SCREEN_HEIGHT - m_textHeight;
  int16_t  x1, y1;
  uint16_t w, h;
  getTextBounds(msg, 0, 0, &x1, &y1, &w, &h);
  fillRect(0, y, SCREEN_WIDTH, m_textHeight, SSD1306_BLACK);
  setCursor((SCREEN_WIDTH - w) / 2, y);
  print(msg);
  display();
}

void Oled::clearMessage(void) {
  uint16_t y = SCREEN_HEIGHT - m_textHeight;
  fillRect(0, y, SCREEN_WIDTH, m_textHeight, SSD1306_BLACK);
}

void Oled::displayIP(void) {
  uint16_t y = getLinePos(0);
  clearLine(0);
  setCursor(0, y);
  print("IP: ");
  print(WiFi.softAPIP());
  display();
}

void Oled::displayNextWatering(const char *way, time_t t) {
  struct tm *pTime;
  char       buffer[MAX_BUF];

  uint16_t y = getLinePos(0);
  clearLine(0);
  setCursor(0, y);
  print("PROCHAIN ARROSAGE:");
  y = getLinePos(1);
  clearLine(1);
  setCursor(0, y);
  print(way);
  y = getLinePos(2);
  clearLine(2);
  setCursor(0, y);
  pTime = localtime(&t);
  strftime(buffer, MAX_BUF, "%d/%m/%Y %H:%M", pTime);
  print(buffer);
  display();
}

void Oled::displayManualWatering(const char *way) {
  char buffer[MAX_BUF];

  uint16_t y = getLinePos(0);
  clearLine(0);
  setCursor(0, y);
  print("ARROSAGE MANUEL:");
  y = getLinePos(1);
  clearLine(1);
  setCursor(0, y);
  print(way);
  y = getLinePos(2);
  clearLine(2);
  setCursor(0, y);
  sprintf(buffer, "%d minutes", Watering::manualDuration());
  print(buffer);
  display();
}
