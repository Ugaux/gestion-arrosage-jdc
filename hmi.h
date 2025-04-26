
#ifndef _HMI_H_
#define _HMI_H_

#define FUNCTION_BUTTON   35
#define MANUAL_BUTTON     34

#include <RTClib.h>
#include <TimeLib.h>


enum hmiState {IDLE, DISPLAY_IP, DISPLAY_NEXTWATERING, DISPLAY_MANUAL};

void fonction();

class Way;

class Hmi
{
  public:
    Hmi();
    void begin();
    void displayDefaults(void);
    void displayIPAndNextWatering(void);
    void displayManual(void);
    void run(void);
    bool isBusy(void);
    uint8_t getButton(void);
    uint8_t m_pressed;

  private:
    int m_state;
    Way *m_manualWatering;
    unsigned long m_time;
};

extern Hmi hmi;

#endif
