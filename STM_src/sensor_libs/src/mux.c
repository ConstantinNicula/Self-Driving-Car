//#include "mux.h"

///*
//	Low level init of mux gpio.
//	PB14 - A 
//	PB13 - B
//	PB12 - C
//*/
//uint16_t mux_pins [] = {
//	GPIO_Pin_14, GPIO_Pin_13, GPIO_Pin_12
//};

//GPIO_TypeDef* mux_pin_ports[] = {
//	GPIOB, GPIOB, GPIOB
//};

//void initMux()
//{
//	GPIO_InitTypeDef GPIO_InitStruct;
//	uint8_t i;
//	
//	/*Enable GPIOB clock*/
//	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB, ENABLE);
//	
//	/*Configure MUX pins*/
//	GPIO_InitStruct.GPIO_Mode = GPIO_Mode_Out_PP;
//	GPIO_InitStruct.GPIO_Speed = GPIO_Speed_50MHz;
//	for(i = 0; i<3; i++)
//	{
//		GPIO_InitStruct.GPIO_Pin = mux_pins[i];
//		GPIO_Init(mux_pin_ports[i], &GPIO_InitStruct);
//	}
//}

///*
//	Configure mux A, B, C to connected to the requested line.
//	val: 0x1 - 0x8;
//*/


//void muxSelect(uint8_t val)
//{	
//	uint8_t i;
//	for(i = 0; i<3; i++)
//	{
//		if(val&(1<<i))
//			mux_pin_ports[i]->BSRR = mux_pins[i];
//		else 
//			mux_pin_ports[i]->BRR = mux_pins[i];
//	}
//}

#include "mux.h"



/*
	Low level init of mux gpio.
	PB12 - A 
	PB13 - B
	PB14 - C
*/
void initMux()
{
	GPIO_InitTypeDef GPIO_InitStruct;
	
	/*Enable GPIOB clock*/
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB, ENABLE);
	
	/*Configure Channel A pin as AF-PP.*/
	GPIO_InitStruct.GPIO_Pin = GPIO_Pin_12|GPIO_Pin_13|GPIO_Pin_14;
	GPIO_InitStruct.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_InitStruct.GPIO_Mode = GPIO_Mode_Out_PP;
	GPIO_Init(GPIOB, &GPIO_InitStruct);
}

/*
	Configure mux A, B, C to connected to the requested line.
	line: 0x1 - 0x8;
*/


void muxSelect(uint8_t line)
{	
	switch(line) 
	{
		case 0:

			GPIO_WriteBit(GPIOB, CHANNEL_A_Pin, Bit_RESET);
			GPIO_WriteBit(GPIOB, CHANNEL_B_Pin, Bit_RESET);
			GPIO_WriteBit(GPIOB, CHANNEL_C_Pin, Bit_RESET);
			break;
		case 1:
			GPIO_WriteBit(GPIOB, CHANNEL_A_Pin, Bit_SET);
			GPIO_WriteBit(GPIOB, CHANNEL_B_Pin, Bit_RESET);
			GPIO_WriteBit(GPIOB, CHANNEL_C_Pin, Bit_RESET);
			break;
		case 2:
			GPIO_WriteBit(GPIOB, CHANNEL_A_Pin, Bit_RESET);
			GPIO_WriteBit(GPIOB, CHANNEL_B_Pin, Bit_SET);
			GPIO_WriteBit(GPIOB, CHANNEL_C_Pin, Bit_RESET);
			break;
		case 3:
			GPIO_WriteBit(GPIOB, CHANNEL_A_Pin, Bit_SET);
			GPIO_WriteBit(GPIOB, CHANNEL_B_Pin, Bit_SET);
			GPIO_WriteBit(GPIOB, CHANNEL_C_Pin, Bit_RESET);
			break;
		case 4:
			GPIO_WriteBit(GPIOB, CHANNEL_A_Pin, Bit_RESET);
			GPIO_WriteBit(GPIOB, CHANNEL_B_Pin, Bit_RESET);
			GPIO_WriteBit(GPIOB, CHANNEL_C_Pin, Bit_SET);
			break;
		case 5:
			GPIO_WriteBit(GPIOB, CHANNEL_A_Pin, Bit_SET);
			GPIO_WriteBit(GPIOB, CHANNEL_B_Pin, Bit_RESET);
			GPIO_WriteBit(GPIOB, CHANNEL_C_Pin, Bit_SET);
			break;
		case 6:			
			GPIO_WriteBit(GPIOB, CHANNEL_A_Pin, Bit_RESET);
			GPIO_WriteBit(GPIOB, CHANNEL_B_Pin, Bit_SET);
			GPIO_WriteBit(GPIOB, CHANNEL_C_Pin, Bit_SET);
			break;
		case 7:
			GPIO_WriteBit(GPIOB, CHANNEL_A_Pin, Bit_SET);
			GPIO_WriteBit(GPIOB, CHANNEL_B_Pin, Bit_SET);
			GPIO_WriteBit(GPIOB, CHANNEL_C_Pin, Bit_SET);
			break;
	}
}

