#include "oled.h"

#include <WiFi.h>

#include "RTCModule/RTCModule.h"
#include "watering/watering.h"

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
  char     title[] = "System init...";
  int16_t  x1, y1;
  uint16_t w, h;
  getTextBounds(title, 0, 0, &x1, &y1, &w, &h);
  setCursor((SCREEN_WIDTH - w) / 2, 0);
  print(title);
  getTextBounds("pt", 0, 0, &x1, &y1, &w, &m_textHeight);
  Serial.printf("Oled::begin with text height %u\n", m_textHeight);
  display();
  return true;
}

uint16_t Oled::getLinePos(int line) {
  return m_textHeight * (3 + line) + (3 * line) - 2;
}

void Oled::clearLine(int line) {
  uint8_t y = getLinePos(line);
  fillRect(0, y, SCREEN_WIDTH, getTextHeight(), SSD1306_BLACK);
}

void Oled::displayTimeDate() {
  char timeString[MAX_BUF];

  DateTime   now         = getCurrentTime();                     // Obtenir l'heure actuelle du RTC
  time_t     currentTime = now.unixtime();                       // Convertir en Unix timestamp
  struct tm *timeinfo    = localtime(&currentTime);              // Convertir en structure tm locale
  strftime(timeString, MAX_BUF, "%d/%m/%Y %H:%M:%S", timeinfo);  // Formater la chaîne de temps

  //Serial.printf("Oled::displayTimeDate %s\n", timeString);
  int16_t  x1, y1;
  uint16_t w, h;
  getTextBounds(timeString, 0, 0, &x1, &y1, &w, &h);
  fillRect(0, 0, SCREEN_WIDTH, m_textHeight, SSD1306_BLACK);  // Effacer la ligne précédente
  setCursor((SCREEN_WIDTH - w) / 2, 0);                       // Centrer le texte
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

void Oled::displayCuveState(Cuve::Etat etat) {
  uint16_t y = getLinePos(1);

  int16_t  x1, y1;
  uint16_t txt_w, h;
  fillRect(0, y, SCREEN_WIDTH, m_textHeight, SSD1306_BLACK);
  getTextBounds("CUVE ", 0, 0, &x1, &y1, &txt_w, &h);

  uint16_t cuve_fill_w = 30;
  uint16_t cursor_x    = 0;
  setCursor(cursor_x, y);
  print("CUVE ");

  drawRect(cursor_x + txt_w, y, cuve_fill_w, m_textHeight - 1, WHITE);  // Main battery body
  // fill (based on level)
  if (etat == Cuve::Etat::INTERMEDIAIRE)
    fillRect(cursor_x + txt_w + 2, y + 2, cuve_fill_w / 2 - 1, m_textHeight - 1 - 4, WHITE);  // 2nd bar
  else if (etat == Cuve::Etat::PLEINE)
    fillRect(cursor_x + txt_w + 2, y + 2, cuve_fill_w - 4, m_textHeight - 1 - 4, WHITE);  // 3rd bar

  display();
}

void Oled::displayError(const char *msg) {
  fillRect(0, m_textHeight + 3, SCREEN_WIDTH, m_textHeight, SSD1306_BLACK);  // efface létat de la cuve
  clearLine(0);
  clearLine(1);
  clearLine(2);
  clearMessage();

  uint16_t y = getLinePos(0);
  setCursor(0, y);
  print("ERROR: ");
  y = getLinePos(1);
  setCursor(0, y);
  int16_t  x1, y1;
  uint16_t w, h;
  String   s(msg);
  s = s.substring(0, 21);
  getTextBounds(s, 0, 0, &x1, &y1, &w, &h);
  fillRect(0, y, SCREEN_WIDTH, m_textHeight, SSD1306_BLACK);
  //setCursor((SCREEN_WIDTH - w) / 2, y);
  print(s);
  //print(s);
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
  String s(way);
  print(s.substring(0, 21));
  y = getLinePos(2);
  clearLine(2);
  setCursor(0, y);
  pTime = localtime(&t);
  strftime(buffer, MAX_BUF, "%d/%m/%Y %H:%M", pTime);
  print(buffer);
  display();
}

void Oled::displayManualWatering(const char *way, bool isActive) {
  char buffer[MAX_BUF];

  uint16_t y = getLinePos(0);
  clearLine(0);
  setCursor(0, y);
  print("ARROSAGE MANUEL:");
  y = getLinePos(1);
  clearLine(1);
  setCursor(0, y);
  String s(way);
  print(s.substring(0, 21));
  y = getLinePos(2);
  clearLine(2);
  setCursor(0, y);
  if (isActive)
    sprintf(buffer, "ARRETER", Watering::manualDuration());
  else
    sprintf(buffer, "ALLUMER %d MINUTES", Watering::manualDuration());
  print(buffer);
  display();
}
