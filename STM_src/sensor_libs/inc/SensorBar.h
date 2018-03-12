#ifndef _SENSORBAR_H_
#define _SENSORBAR_H_

#include "mux.h"


// global reference for each sensor.
#define NUM_SENSORS 4
extern float sensorReadings[];


/*Methods for configuring sensors and reading data.*/
void InitSensorBar(void);
void ReadSensorData(void);





#endif
