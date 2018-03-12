#ifndef _ENCODER_H_
#define _ENCODER_H_
#include "stm32f10x.h"

#define total_gear_reduction (1/10.46) 
#define wheel_radius 42.5   // in mm


// read every 100 ms
#define encoder_read_period 150 

// car velocity
extern float velocity ;
extern float prev_velocity;

// stall check 
extern uint8_t motor_stalling;

void initEncoder(void);
void startReadingEncoder(void);

#endif

