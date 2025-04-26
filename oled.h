#ifndef _OLED_H_
#define _OLED_H_

#include <Adafruit_SSD1306.h>
#include <TimeLib.h>
#include <RTClib.h>

#define OLED_RESET     4  // Reset pin # (or -1 if sharing Arduino reset pin)
#define SCREEN_ADDRESS 0x3C
#define SCREEN_WIDTH   128
#define SCREEN_HEIGHT  64

#endif

class Oled : public Adafruit_SSD1306 {
public:
  Oled();
  bool     begin(void);
  void     displayTimeDate(void);
  void     displayMoisture(int moisture);
  void     displayFlow(float flow);
  void     displayMessage(const char *msg);
  void     clearMessage(void);
  void     displayIP(void);
  void     displayNextWatering(const char *way, time_t t);
  void     displayManualWatering(const char *way);
  uint16_t getTextHeight(void) { return m_textHeight; }
  uint16_t getLinePos(int line);
  void     clearLine(int line);

private:
  uint16_t m_textHeight;
};

extern Oled display;
