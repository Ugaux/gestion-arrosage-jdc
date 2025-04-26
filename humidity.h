
#ifndef _HUMIDITY_H_
#define _HUMIDITY_H_

/*
#define HUMIDITY_AIR  700 // air humidity
#define HUMIDITY_WATER 290 // water humidity
*/
// Humidity sensor 
#define HUMIDITY_AIR    2400
#define HUMIDITY_WATER  1850

//#define HUMIDITY_AIR        3300
//#define HUMIDITY_WATER      1300

#define HUMIDITY_DRY        1
#define HUMIDITY_WET        2

int getSoilMoisture(int *value);

#endif
