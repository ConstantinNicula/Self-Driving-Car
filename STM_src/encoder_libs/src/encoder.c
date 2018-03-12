#include "encoder.h"
#include "Delay.h"
#include "retarget.h"
#include "math.h"
static uint8_t nbTicksPerRotation = 4;
// previous direction.
uint8_t pdir = 0;
// last time the funciton was called.
uint32_t prevt;

// measured car velocity.
float velocity;
float prev_velocity;

// stall protection flag
uint8_t motor_stalling;

// PA6 D12
// PA7 D11
void initEncoder() 
{
	GPIO_InitTypeDef GPIO_InitStructure;
	TIM_TimeBaseInitTypeDef TIM_TimeBaseInitStructure;
	
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA | RCC_APB2Periph_AFIO, ENABLE);
	
	//initialize TIM3 Channel 1 -> port A, pin 8 (connected to servo data)
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_6;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN_FLOATING;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_Init(GPIOA, &GPIO_InitStructure);
	
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_7;
	GPIO_Init(GPIOA, &GPIO_InitStructure);
	
	/* TIM3 clock enable */
  RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM3, ENABLE);
	
	/*Configre timebase*/
	TIM_TimeBaseInitStructure.TIM_Prescaler = 0;
	TIM_TimeBaseInitStructure.TIM_Period = 0xFFFF;
	TIM_TimeBaseInitStructure.TIM_ClockDivision = 0;
	TIM_TimeBaseInitStructure.TIM_CounterMode = TIM_CounterMode_Up;
	TIM_TimeBaseInit(TIM3, &TIM_TimeBaseInitStructure);
	
	TIM_EncoderInterfaceConfig(TIM3, TIM_EncoderMode_TI12, TIM_ICPolarity_Rising, TIM_ICPolarity_Rising);
	TIM_Cmd (TIM3, ENABLE);
}



/*
	Configure a timer to read car forward velocity at a regular interval.
*/

void startReadingEncoder(void)
{
	TIM_TimeBaseInitTypeDef TIM_TimeBaseStruct;
	NVIC_InitTypeDef NVIC_InitStructure;
	uint16_t prescaler = (uint16_t) (SystemCoreClock / 10000) - 1;
	/*Enable clock timer.*/
	RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM2, ENABLE);

	/*Configure timer to generate a interrupt every 100 ms.*/
	TIM_TimeBaseStruct.TIM_Prescaler = prescaler; // 10kHz signal
	TIM_TimeBaseStruct.TIM_Period = encoder_read_period*10 - 1; // 10 * 100
	TIM_TimeBaseStruct.TIM_ClockDivision = 0;
	TIM_TimeBaseStruct.TIM_CounterMode = TIM_CounterMode_Up;
	
	TIM_TimeBaseInit(TIM2, &TIM_TimeBaseStruct);
	
	/* Enable the TIM2 global Interrupt */
	NVIC_InitStructure.NVIC_IRQChannel = TIM2_IRQn;
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 0x1;
	NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0;
	NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE; 
	NVIC_Init(&NVIC_InitStructure);

	
	/*TIM2 IT enable.*/
	TIM_ITConfig(TIM2, TIM_IT_Update, ENABLE);
	
	/*TIM2 enable counter.*/
	TIM_Cmd(TIM2, ENABLE);
}

/*Configure interrupt.*/
void TIM2_IRQHandler(void)
{
	// increment this counter each time car velocity drops below a certain treshold.
	static uint8_t low_velocity_counter = 0;

	// previous measurement.
	static uint32_t cnt1;
	
	if(TIM_GetITStatus(TIM2, TIM_IT_Update) != RESET)
	{
		int32_t diff, cnt2;
		uint8_t dir = TIM3->CR1 & TIM_CR1_DIR;
		//save prev velocity.
		prev_velocity = velocity;
		
		
		// check down counting.
		cnt2 = TIM3->CNT;
		
		// if the timer is downcounting
		if( TIM3->CR1 & TIM_CR1_DIR) 
		{
			// check for underflow.
			if(cnt2 <= cnt1)
				diff = cnt1 - cnt2;
			else 
				diff = (0xffff - cnt2) + cnt1;
			
			velocity =  diff * total_gear_reduction * wheel_radius * (1000./encoder_read_period /nbTicksPerRotation) * 6.28;
			
		}else {	// is upcounting.
			// check for overflow.
			if(cnt2 >= cnt1)
				diff = cnt2 - cnt1;
			else 
				diff = (0xffff - cnt1) + cnt2;
			
			velocity = -diff * total_gear_reduction * wheel_radius * (1000./encoder_read_period /nbTicksPerRotation) * 6.28;
		}

		cnt1 = cnt2;	
		
		// if low velocity increment the counter.
		if(fabs(velocity) < 0.1)
			low_velocity_counter ++;
		else 
			low_velocity_counter = 0;
		
		// if the counter reached 20 increments (3 sec of stall time), set stall flag

		if(low_velocity_counter == 15)
		{
			motor_stalling = 1;
			low_velocity_counter = 0;
		}
		
		TIM_ClearITPendingBit(TIM2, TIM_IT_Update);
	}
}	
