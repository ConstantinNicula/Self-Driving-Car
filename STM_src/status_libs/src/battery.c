#include "stm32f10x.h"
#include "battery.h"

#define ADC_Channel 

float batteryLevel;

/*Configure ADC for reading battery voltage.*/
void initBatteryADC()
{
	ADC_InitTypeDef ADC_InitStruct;
	/*Enable ADC1 clock*/
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_ADC1, ENABLE);
	/*Clear settings and power down adc.*/
	ADC_DeInit(ADC1);
	
	/*Configure ADC1*/
	ADC_InitStruct.ADC_Mode = ADC_Mode_Independent;
	/*Disable scan conversion.*/
	ADC_InitStruct.ADC_ScanConvMode = DISABLE;
	/*Disable continous conversion.*/
	ADC_InitStruct.ADC_ContinuousConvMode = DISABLE;
	/*Set conversion start to software trigger.*/
	ADC_InitStruct.ADC_ExternalTrigConv = ADC_ExternalTrigConv_None;
	/*Align result to the right.*/
	ADC_InitStruct.ADC_DataAlign = ADC_DataAlign_Right;
	/*Specifiy that we will use one channel.*/
	ADC_InitStruct.ADC_NbrOfChannel = 1;
	
	/*Setu up peripheral.*/
	ADC_Init(ADC1, &ADC_InitStruct);
	/*Enable peripheral.*/
	ADC_Cmd(ADC1, ENABLE);
	
	/*Enable reset callibration.*/
	ADC_ResetCalibration(ADC1);
	/*Busy wait until callibration is reset.*/
	while(ADC_GetResetCalibrationStatus(ADC1));
	/*Start ADC1 calibration cycle*/
	ADC_StartCalibration(ADC1);
	/*Wait for end of calibration.*/
	while(ADC_GetCalibrationStatus(ADC1));
}
/*Returns a float indicating the source voltage of the VDIV.*/

float getBatteryVoltage()
{
	static uint16_t num_samples = 20;
	uint16_t  adc_value, i;
	batteryLevel = 0;
	
	/*Read 10 values and return average.*/
	for (i = 0 ; i<num_samples; i++)
	{
		ADC_RegularChannelConfig(ADC1, ADC_Channel_4, 1, ADC_SampleTime_7Cycles5);
		/*Start conversion*/
		ADC_SoftwareStartConvCmd(ADC1, ENABLE);
		/*Wait until conversion  completion*/
		while(ADC_GetFlagStatus(ADC1, ADC_FLAG_EOC) == RESET);
		/*Get conversion value*/
		adc_value = ADC_GetConversionValue(ADC1);
		
		/*Update battery level variable*/
		batteryLevel += (float) adc_value/((1<<12)-1) * 3.3 * (8.6/3);
	}
	batteryLevel /= num_samples;
	return batteryLevel; 
} 

