#include "stm32f10x.h"
#include "stm32f10x_flash.h"

/**
 * Initializes system clock
 */
void InitSystemClock( void )
{
	// HSE initialization status
	ErrorStatus HSEStartUpStatus;
	
	// clear configuration of RCC
	RCC_DeInit();
	// enable HSE
	RCC_HSEConfig( RCC_HSE_ON );
	// wait for HSE to become ready
	HSEStartUpStatus = RCC_WaitForHSEStartUp();
	// verify the initialization
	if( HSEStartUpStatus == SUCCESS ) {
		// setup FLASH communication for supporting this clock coonfiguration
		FLASH_PrefetchBufferCmd( FLASH_PrefetchBuffer_Enable );
		FLASH_SetLatency( FLASH_Latency_2 );
		// set HCLK frequency to 72MHz
		RCC_HCLKConfig( RCC_SYSCLK_Div1 ); 
		// set PCLK2 frequency to 72MHz
		RCC_PCLK2Config( RCC_HCLK_Div1 ); 
		// set PCLK1 frequency to 36MHz
		RCC_PCLK1Config( RCC_HCLK_Div2 );
		// set PLL multiplier for 72MHz
		RCC_PLLConfig( RCC_PLLSource_HSE_Div1, RCC_PLLMul_6 );
		// start PLL
		RCC_PLLCmd( ENABLE );
		// wait for PLL to stabilize
		while( RCC_GetFlagStatus( RCC_FLAG_PLLRDY ) == RESET ) {}
		// use PLL as system clock
		RCC_SYSCLKConfig( RCC_SYSCLKSource_PLLCLK );
		// wait for system clock source selection
		while( RCC_GetSYSCLKSource() != 0x08 ) {}
		// configure ADC clock for 12MHz
		RCC_ADCCLKConfig( RCC_PCLK2_Div6 );
		// update system core clock
		SystemCoreClockUpdate();
	} else {
		// failed to start, block program execution
		while( 1 ) {}
	}
}


void InitBoardDebug( void ) 
{
	// IO initialization structure
	GPIO_InitTypeDef GPIO_InitStructure;
	
	// enable clock to GPIOB
	RCC_APB2PeriphClockCmd( RCC_APB2Periph_GPIOB | RCC_APB2Periph_AFIO, ENABLE );
	
	// save pin speed and pin mode
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_9;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_2MHz;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IPD;
	// set LED pin
	GPIO_Init( GPIOB, &GPIO_InitStructure );
}

uint8_t HasDebugConnected( void )
{
	return ( uint8_t ) GPIO_ReadInputDataBit( GPIOB, GPIO_Pin_9 );
}


/**
 * Setup board LED
 */ 
void InitBoardLED( void )
{
	// IO initialization structure
	GPIO_InitTypeDef GPIO_InitStructure;
	
	// enable clock to GPIOB
	RCC_APB2PeriphClockCmd( RCC_APB2Periph_GPIOB, ENABLE );
	// enable AFIO clock
	RCC_APB2PeriphClockCmd( RCC_APB2Periph_AFIO, ENABLE );
	// disable JTAG and JTAG-DP
	GPIO_PinRemapConfig( GPIO_Remap_SWJ_NoJTRST, ENABLE );    
	GPIO_PinRemapConfig( GPIO_Remap_SWJ_JTAGDisable, ENABLE );	
	
	// save pin speed and pin mode
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_5;//GPIO_Pin_4;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_2MHz;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
	// set LED pin
	GPIO_Init( GPIOB, &GPIO_InitStructure );
}


void InitBoard( void )
{
	InitSystemClock();
	InitBoardLED();
	InitBoardDebug();
	SysTick_Config(SystemCoreClock/1000);
}


