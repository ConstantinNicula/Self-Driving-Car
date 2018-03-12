#ifndef _CONTROL_H__
#define _CONTROL_H__
#include "stm32f10x.h"

/*
	Should be updated at regular interval.
*/
extern float velocity;
extern float basePWM;

//gain parameter used for controller.
extern float gain;
/*
	Compute heading angle.
*/
float computeNewHeading(void);
void getInitialReadings(void);
float getHeading(float* sensorMeasurements);
uint16_t getPWM_from_angle(float angle);

#endif
