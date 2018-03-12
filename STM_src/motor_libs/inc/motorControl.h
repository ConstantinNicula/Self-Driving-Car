#ifndef _MOTOR_CONTROL_
#define _MOTOR_CONTROL_

#include "stm32f10x.h"
#include "encoder.h"

static uint16_t PrescalerValue = 71;//Frequency on APB1 is 36MHz with prescaler of 1 and x2 = 72MHz otherwise

/*
	This variable is set through setDesired velocity funcion, it is used by 
	the proportional motor controller and for detecting motor stall.
*/
extern float desiredVelocity;
extern uint16_t* forward_rpm_profile; // used when setting motor rpm.
extern uint16_t* backward_rpm_profile;
extern uint8_t profile_step; // divison used when creating the rpm profile.
extern uint16_t max_rpm;


#define escPin GPIO_Pin_8// D15
#define escPort GPIOB

#define servoPin GPIO_Pin_9 //(D14)
#define servoPort GPIOB

void initEscControlPin( void );
void initServoControlPin( void );
void initMotorsTimer( void );
void setServoPWM(uint16_t pwm);
void setEscPWM(uint16_t pwm);


void setDesiredVelocity(float dvel);
void printRPMProfile(uint8_t step, uint16_t max_rpm, uint8_t direction);
#endif
