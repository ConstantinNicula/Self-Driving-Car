#ifndef _SETUP_H_
#define _SETUP_H_

#include "stm32f10x.h"

extern uint32_t SystemCoreClock;

// init core clock 
void InitSystemClock( void );
void InitTraceClock( void );
void InitBoard( void );


#endif
