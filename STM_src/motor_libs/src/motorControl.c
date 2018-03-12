#include "stdio.h"
#include "motorControl.h"

void initServoControlPin( void )
{
	GPIO_InitTypeDef GPIO_InitStructure;
	
	//enable clock on GPIOB, alternate funcion and timer1
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB | RCC_APB2Periph_AFIO, ENABLE);
	
	//initialize TIM1 Channel 1 -> port A, pin 8 (connected to servo data)
	GPIO_InitStructure.GPIO_Pin = servoPin;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	
	GPIO_Init(servoPort, &GPIO_InitStructure);
}

void initEscControlPin( void )
{
	GPIO_InitTypeDef GPIO_InitStructure;
	
	//enable clock on GPIOA, alternate funcion and timer1
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB | RCC_APB2Periph_AFIO, ENABLE);
	
	//initialize TIM1 Channel 3 -> port A, pin 10 (connected to esc data)
	GPIO_InitStructure.GPIO_Pin = escPin;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	
	GPIO_Init(escPort, &GPIO_InitStructure);
}


void initMotorsTimer( void )
{
	TIM_TimeBaseInitTypeDef TIM_TimeBaseInitStructure;
	TIM_OCInitTypeDef TIM_OCInitStructure;
	NVIC_InitTypeDef NVIC_InitStructure;
	TIM_ICInitTypeDef  TIM_ICInitStructure;
	
	//enable TIM4 clock
	RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM4, ENABLE);
	
	//setting up the timer
	TIM_TimeBaseInitStructure.TIM_Prescaler = PrescalerValue;
	TIM_TimeBaseInitStructure.TIM_Period = 19999;
	TIM_TimeBaseInitStructure.TIM_ClockDivision = 0;
	TIM_TimeBaseInitStructure.TIM_CounterMode = TIM_CounterMode_Up;
	TIM_TimeBaseInit(TIM4, &TIM_TimeBaseInitStructure);
	
	// configure output capture on channel 3 and 4
	TIM_OCInitStructure.TIM_Pulse = 0;//initial duty cycle
	TIM_OCInitStructure.TIM_OCMode = TIM_OCMode_PWM1;
	TIM_OCInitStructure.TIM_OCPolarity = TIM_OCPolarity_High;
	TIM_OCInitStructure.TIM_OCNPolarity = TIM_OCPolarity_High;
	TIM_OCInitStructure.TIM_OutputState = TIM_OutputState_Enable;
	TIM_OCInitStructure.TIM_OutputNState = TIM_OutputNState_Disable;
	TIM_OCInitStructure.TIM_OCIdleState = TIM_OCIdleState_Reset;
	
	//initialize channel 3 of timer 4 => servo
	TIM_OC3Init(TIM4, &TIM_OCInitStructure);
	TIM_OC3PreloadConfig(TIM4, TIM_OCPreload_Enable);
	
	//initialize channel 4 of timer 4 => esc
	TIM_OC4Init(TIM4, &TIM_OCInitStructure);
	TIM_OC4PreloadConfig(TIM4, TIM_OCPreload_Enable);
	
	// enable input capture on channel 1 and 2
	TIM_ICInitStructure.TIM_Channel = TIM_Channel_2;
	TIM_ICInitStructure.TIM_ICPolarity = TIM_ICPolarity_Rising;
	TIM_ICInitStructure.TIM_ICSelection = TIM_ICSelection_DirectTI;
	TIM_ICInitStructure.TIM_ICPrescaler = TIM_ICPSC_DIV1;
	TIM_ICInitStructure.TIM_ICFilter = 0x0;
	TIM_ICInit(TIM4, &TIM_ICInitStructure);
	
	TIM_ICInitStructure.TIM_Channel = TIM_Channel_1;
	TIM_ICInitStructure.TIM_ICPolarity = TIM_ICPolarity_Falling;
	TIM_ICInitStructure.TIM_ICSelection = TIM_ICSelection_IndirectTI;
	TIM_ICInitStructure.TIM_ICPrescaler = TIM_ICPSC_DIV1;
	TIM_ICInitStructure.TIM_ICFilter = 0x0;
	TIM_ICInit(TIM4, &TIM_ICInitStructure);
	
	// Enable the TIM4 global Interrupt 
	NVIC_InitStructure.NVIC_IRQChannel = TIM4_IRQn;
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 0;
	NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0;
	NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
	NVIC_Init(&NVIC_InitStructure);
	
	// enable timer
	//TIM_ARRPreloadConfig(TIM4, ENABLE);
	TIM_Cmd(TIM4, ENABLE);
	TIM_CtrlPWMOutputs(TIM4, ENABLE);	
	
	// Enable the CC1 && CC2Interrupt Request 
	TIM_ITConfig(TIM4, TIM_IT_CC1, ENABLE);
	TIM_ITConfig(TIM4, TIM_IT_CC2, ENABLE);
}


void setServoPWM(uint16_t pwm)
{
	if(pwm < 1000)
		pwm = 1000;
	else if(pwm > 2000)
		pwm = 2000;

	TIM4->CCR4 = pwm;
}

void setEscPWM(uint16_t pwm)
{
	if(pwm < 1000)
		pwm = 1000;
	else if(pwm > 2000)
		pwm = 2000;
	
	TIM4->CCR3 = pwm;
}


