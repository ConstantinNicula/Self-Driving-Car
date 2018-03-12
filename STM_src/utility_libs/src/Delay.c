#include "Delay.h"

void SysTick_Handler( void )
{
	msTicks++;
}


void delay(uint32_t delayTicks)
{
	uint32_t currentTicks = msTicks;
	while( (msTicks - currentTicks) < delayTicks)
	{
		__NOP();
	}
}

void initSystemClock( void )
{
	SysTick_Config( SystemCoreClock / 1000 );
}
