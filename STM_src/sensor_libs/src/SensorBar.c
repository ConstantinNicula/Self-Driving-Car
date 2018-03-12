#include "SensorBar.h"
#include "retarget.h"
#include "delay.h"
/*Buffers*/

float sensorReadings[NUM_SENSORS];
volatile uint32_t t = 0;

#define conversion_factor 343./2*1e-6

uint8_t mux_map[4] = {
	7, // stanga 
	6, // stanga fata
	2,// dreapta fata
	3 // dreapta
};

uint16_t gpio_trig_map[4] = {
	GPIO_Pin_0, 
	GPIO_Pin_1,
	GPIO_Pin_2,
	GPIO_Pin_3
};

uint8_t sensor_order[4] = 
{
	0, // left back
	2, // right front
	1, // left front 
	3 // right back
};

uint8_t currently_active_sensor = 0;
volatile uint8_t sensor_read_done = 0;
volatile uint8_t sensor_timed_out = 0;

uint32_t pulse_duration;


/* Configure timer needed to send 10us pulse to sensors*/
void initTIM1(void)
{
	TIM_TimeBaseInitTypeDef  TIM_TimeBaseStructure;
	NVIC_InitTypeDef NVIC_InitStructure;

	/* TIM2 clock enable */
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_TIM1, ENABLE);

	/* Enable the TIM2 global Interrupt */
	NVIC_InitStructure.NVIC_IRQChannel = TIM1_UP_IRQn;
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 0;
	NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0;
	NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
	NVIC_Init(&NVIC_InitStructure);
	
	/* Time base configuration */
	TIM_TimeBaseStructure.TIM_Period = 719;
	TIM_TimeBaseStructure.TIM_Prescaler = 0;
	TIM_TimeBaseStructure.TIM_ClockDivision = 0;
	TIM_TimeBaseStructure.TIM_CounterMode = TIM_CounterMode_Up;
	TIM_TimeBaseStructure.TIM_RepetitionCounter = 0;
	TIM_TimeBaseInit(TIM1, &TIM_TimeBaseStructure);
}

void SendTriggerPulse()
{
	/* Reset tmer counter */
	TIM1->CNT = 0;
	/* Enable timer interrupts */
	TIM_ITConfig(TIM1, TIM_IT_Update, ENABLE);
	TIM_Cmd(TIM1, ENABLE);
	/* Use trig gpio map to set coresspoding pin high*/
	GPIOB->BSRR = gpio_trig_map[currently_active_sensor];
}


void TIM1_UP_IRQHandler(void)
{
	 /*If 10us elapsed, toggle active sensor trig pin and disable timer.*/
	if (TIM_GetITStatus(TIM1, TIM_IT_Update) != RESET)
	{
		GPIOB->BRR = gpio_trig_map[currently_active_sensor];
		/*Clear interrupt to avoid re-entry and disable timer*/
		TIM_Cmd(TIM1, DISABLE);
		TIM_ClearITPendingBit(TIM1, TIM_IT_Update);
		//printf("a");
	}
}



void TIM4_IRQHandler(void)
{
	static uint32_t cnt1, cnt2;
	
	if(TIM_GetITStatus(TIM4, TIM_IT_CC2) == SET)
	{
		/* Store current CC2 value*/
		cnt1 = TIM_GetCapture2(TIM4);
		sensor_read_done = 0;
		/* Clear TIM4 Capture compare interrupt pending bit */
		TIM_ClearITPendingBit(TIM4, TIM_IT_CC2);
	}else if(TIM_GetITStatus(TIM4, TIM_IT_CC1) == SET)
	{
		/* Store current CC1 vlaue*/
		cnt2 = TIM_GetCapture1(TIM4);
		/* Compute pulse duration*/
		if (cnt2 > cnt1)
		 {
			pulse_duration = (cnt2 - cnt1); 
		 }
		 else
		{
			pulse_duration = ((19999 - cnt1) + cnt2); 
		}
		sensor_read_done = 1;
		
		/* Clear TIM4 Capture compare interrupt pending bit */
		TIM_ClearITPendingBit(TIM4, TIM_IT_CC1);
	}
}

/*Init sensor bar.*/

// note TIM4 is used for both ESC&SERVO PWM driving and for sensor input capture, 
// all the needed TIM configuration is done in initMotorsTimer function found in montorControl.c
void InitSensorBar()
{
	uint8_t i;
	GPIO_InitTypeDef GPIO_InitStructure;
	// enable gpio clock
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB, ENABLE);
	
	// configure trigger pins
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	for(i = 0 ; i<NUM_SENSORS; i++)
	{
		GPIO_InitStructure.GPIO_Pin = gpio_trig_map[i];
		GPIO_Init(GPIOB, &GPIO_InitStructure);
	}
	
	initTIM1();
	
	// configure tim4 channel 2 input pin
	// TIM4 channel 2 pin B7 configuration 
	GPIO_InitStructure.GPIO_Pin =  GPIO_Pin_7;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN_FLOATING;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_Init(GPIOB, &GPIO_InitStructure);
	
	// configure MOSFET powerdown pin
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_6;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_Init(GPIOB, &GPIO_InitStructure);
	
	// inti mux pins
	initMux();
}


/* Read data from all sensors. */
void ReadSensorData()
{
		static uint8_t need_restart = 0;
		uint32_t t, i;
		
		if(need_restart)
		{
			GPIOB->BRR = GPIO_Pin_6;
			for(t = 0; t<100; t++); // short delay.
			GPIOB->BSRR = GPIO_Pin_6;
			// clear need restart flag.
			need_restart = 0;
		}
	
		i = 0;
		currently_active_sensor = sensor_order[i++];
		while(i <= NUM_SENSORS)
		{
			// switch to the correct mux channel.
			muxSelect(mux_map[currently_active_sensor]);
			
			// start measurement
			SendTriggerPulse();
			
			// while measurement not done and not timed out
			sensor_read_done = 0;
			t = msTicks; // store current time;
			while(!sensor_read_done)
			{
				if((msTicks - t) >=8) {
					need_restart = 1;
					pulse_duration = 8000;
					break;
				}
			}
			
			// compute distance
			sensorReadings[currently_active_sensor] = conversion_factor*pulse_duration;
			
			// update currently active sensor
			currently_active_sensor = sensor_order[i++];
		}
}



