#include "LSM6DS33.h"
#include "I2CRoutines.h"
#define LSM6DS33_ADDRESS 0xD6
#include "math.h"
#include "retarget.h"

/*Storage location for sensor data.*/
int16_t acc_data[3];
int16_t gyro_data[3];

#define NUM_DATA_POINTS 20
float mean_buff[NUM_DATA_POINTS];
uint16_t buf_index = 0;

/*Low level init funciton for acc and gyro.*/
void initLSM6DS33(void)
{
	I2C_LowLevel_Init(I2C2);
	
	// Accelerometer
	// 0x84 = 0b10000100
	// ODR = 1000 (1.66 kHz (high performance)); FS_XL = 01 (+/-16 g full scale)
	writeReg(CTRL1_XL, 0x84);

	// Gyro
	// 0x80 = 0b010000000
	// ODR = 1000 (1.66 kHz (high performance)); FS_XL = 00 (245 dps)
	writeReg(CTRL2_G, 0x80);

	// Common
	// 0x04 = 0b00000100
	// IF_INC = 1 (automatically increment register address)
	writeReg(CTRL3_C, 0x04);
}

/*Low level read write functions.*/
void writeReg(uint8_t reg, uint8_t value)
{
	uint8_t buf[2];
	buf[0] = reg;
	buf[1] = value;
	I2C_Master_BufferWrite(I2C2, buf, 2, DMA, LSM6DS33_ADDRESS);
}

uint8_t readReg(uint8_t reg)
{
	uint8_t value;
	I2C_Master_BufferWrite(I2C2, &reg, 1, Polling, LSM6DS33_ADDRESS);
	I2C_Master_BufferRead(I2C2, &value, 1, Polling, LSM6DS33_ADDRESS);
	return value;
}

/*Read funcitions for accelerometer data.*/
void readAcc(void)
{
	uint8_t buf[6], reg = OUTX_L_XL;
	// read the accelerometer data and store them in a vector.
	I2C_Master_BufferWrite(I2C2, &reg, 1, DMA, LSM6DS33_ADDRESS);
	I2C_Master_BufferRead(I2C2, buf, 6, DMA, LSM6DS33_ADDRESS);
	// get combine xL and XH values.
	acc_data[0] = buf[1] << 8 | buf[0];
	acc_data[1] = buf[3] << 8 | buf[2];
	acc_data[2] = buf[5] << 8 | buf[4];
	
	//printf("$%d %d %d;\n", acc_data[0], acc_data[1], acc_data[2]);
}
void readGyro(void)
{
	uint8_t buf[6], reg  = OUTX_L_G;
	// read the accelerometer data and store them in a vector.
	I2C_Master_BufferWrite(I2C2, &reg, 1, DMA, LSM6DS33_ADDRESS);
	I2C_Master_BufferRead(I2C2, buf, 6, DMA, LSM6DS33_ADDRESS);
	// get combine xL and XH values.
	gyro_data[0] = buf[1] << 8 | buf[0];
	gyro_data[1] = buf[3] << 8 | buf[2];
	gyro_data[2] = buf[5] << 8 | buf[4];
}

uint8_t checkCarRaised()
{
	static uint8_t state_verif = 0, i;
	float treshold = 1.5;
	float avg = 0, ax, ay, az;
	
	// compute new mean value and add it to buffer
	ax = acc_data[0]*0.488*1e-3;
	ay = acc_data[1]*0.488*1e-3;
	az = acc_data[2]*0.488*1e-3;
	mean_buff[(buf_index++)%NUM_DATA_POINTS] = sqrt(ax*ax + ay*ay+  az*az);
	
	// check if average of last NUM_DATA_POINTS  is less then a treshold.
	for(i = 0; i<NUM_DATA_POINTS; i++)
	 avg += mean_buff[i];
	
	avg /=NUM_DATA_POINTS;
	
	// check if state persists across a number of time frames.
	if(avg > treshold && (buf_index >NUM_DATA_POINTS))
		state_verif++;
	
	
	//printf("avg: %f\n", avg);
	if(state_verif > 12)
	{
		buf_index = 0;
		state_verif = 0;
		return 1;
	}
	return 0;

}
