/*#include "stm32f10x.h"

#ifndef _MUX_H_
#define _MUX_H_

void initMux(void);
void muxSelect(uint8_t line);

#endif 
*/

#include "stm32f10x.h"

#ifndef _MUX_H_
#define _MUX_H_

#define REVERSE_PIN_ORDER

#ifndef REVERSE_PIN_ORDER
	#define CHANNEL_A_Pin GPIO_Pin_12 //
	#define CHANNEL_B_Pin GPIO_Pin_13 //
	#define CHANNEL_C_Pin GPIO_Pin_14 // 
#else 
	#define CHANNEL_A_Pin GPIO_Pin_14
	#define CHANNEL_B_Pin GPIO_Pin_13
	#define CHANNEL_C_Pin GPIO_Pin_12
#endif

void initMux(void);
void muxSelect(uint8_t line);

#endif 
