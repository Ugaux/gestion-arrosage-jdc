#ifndef _HUMIDITY_H_
#define _HUMIDITY_H_

#define HUMIDITY_DRY 1
#define HUMIDITY_WET 2

#define HUMIDITY_AIR   2400
#define HUMIDITY_WATER 1850

int   getSoilMoisture(int *value);
float getFlow(void);

#endif
