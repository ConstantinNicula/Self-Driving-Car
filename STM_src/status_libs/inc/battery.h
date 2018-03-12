#ifndef _BATTERY_STATUS
#define _BATTERY_STATUS

extern float batteryLevel;

/*Init adc for battery reading.*/
void initBatteryADC(void);
float getBatteryVoltage(void);


#endif

