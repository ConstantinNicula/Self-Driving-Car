#include "stm32f10x.h"

#ifndef _DELAY_
#define _DELAY_

extern volatile uint32_t  msTicks;

void SysTick_Handler( void );
void delay(uint32_t delayTicks);
void initSystemClock( void );

#endif
