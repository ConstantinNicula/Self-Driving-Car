#include "TCS34725.h"
#include "I2CRoutines.h"
#include "Delay.h"
#include "math.h"
#include "retarget.h"

/*Low level read write functions.*/
void write8(uint8_t reg, uint32_t value)
{
	uint8_t buf[2];
	buf[0] = TCS34725_COMMAND_BIT | reg;
	buf[1] = value&0xff;
	I2C_Master_BufferWrite(I2C2, buf, 2, DMA, TCS34725_ADDRESS);
}

uint8_t read8(uint8_t reg)
{
	uint8_t value;
	reg = TCS34725_COMMAND_BIT |reg;
	I2C_Master_BufferWrite(I2C2, &reg, 1, Polling, TCS34725_ADDRESS);
	I2C_Master_BufferRead(I2C2, &value, 1, Polling, TCS34725_ADDRESS);
	return value;
}

uint16_t read16(uint8_t reg)
{
	uint8_t t, x;
	reg = TCS34725_COMMAND_BIT |reg;
	I2C_Master_BufferWrite(I2C2, &reg, 1, Polling, TCS34725_ADDRESS);
	I2C_Master_BufferRead(I2C2, &t, 1, Polling, TCS34725_ADDRESS);
	I2C_Master_BufferRead(I2C2, &x, 1, Polling, TCS34725_ADDRESS);
	return (x<<8)|t;
}


// Enable the device
void enable(void)
{
  write8(TCS34725_ENABLE, TCS34725_ENABLE_PON);
  delay(3);
  write8(TCS34725_ENABLE, TCS34725_ENABLE_PON | TCS34725_ENABLE_AEN);  
}

//  Disables the device (putting it in lower power sleep mode)

void disable(void)
{
  /* Turn the device off to save power */
  uint8_t reg = 0;
  reg = read8(TCS34725_ENABLE);
  write8(TCS34725_ENABLE, reg & ~(TCS34725_ENABLE_PON | TCS34725_ENABLE_AEN));
}

//Initializes I2C and configures the sensor (call this function before doing anything else)
void initColorSensor(void)
{		
	uint8_t x;

	I2C_LowLevel_Init(I2C2);

	x = read8(TCS34725_ID);
	
	printf("d\n");
  if ((x != 0x44) && (x != 0x10))
  {
    return;
  }

  /* Set default integration time and gain */
  setIntegrationTime(TCS34725_INTEGRATIONTIME_2_4MS);
  setGain(TCS34725_GAIN_1X);

	  /* Note: by default, the device is in power down mode on bootup */
  enable();
}


//Sets the integration time for the TC34725
void setIntegrationTime(tcs34725IntegrationTime_t it)
{
  /* Update the timing register */
  write8(TCS34725_ATIME, it);
}

//Adjusts the gain on the TCS34725 (adjusts the sensitivity to light)

void setGain(tcs34725Gain_t gain)
{
  /* Update the timing register */
  write8(TCS34725_CONTROL, gain);
}


// Reads the raw red, green, blue and clear channel values

void getRawData (uint16_t *r, uint16_t *g, uint16_t *b, uint16_t *c)
{
  *c = read16(TCS34725_CDATAL);
  *r = read16(TCS34725_RDATAL);
  *g = read16(TCS34725_GDATAL);
  *b = read16(TCS34725_BDATAL);
}

// Converts the raw R/G/B values to color temperature in degrees Kelvin

uint16_t calculateColorTemperature(uint16_t r, uint16_t g, uint16_t b)
{
  float X, Y, Z;      /* RGB to XYZ correlation      */
  float xc, yc;       /* Chromaticity co-ordinates   */
  float n;            /* McCamy's formula            */
  float cct;

  /* 1. Map RGB values to their XYZ counterparts.    */
  /* Based on 6500K fluorescent, 3000K fluorescent   */
  /* and 60W incandescent values for a wide range.   */
  /* Note: Y = Illuminance or lux                    */
  X = (-0.14282F * r) + (1.54924F * g) + (-0.95641F * b);
  Y = (-0.32466F * r) + (1.57837F * g) + (-0.73191F * b);
  Z = (-0.68202F * r) + (0.77073F * g) + ( 0.56332F * b);

  /* 2. Calculate the chromaticity co-ordinates      */
  xc = (X) / (X + Y + Z);
  yc = (Y) / (X + Y + Z);

  /* 3. Use McCamy's formula to determine the CCT    */
  n = (xc - 0.3320F) / (0.1858F - yc);

  /* Calculate the final CCT */
  cct = (449.0F * pow(n, 3)) + (3525.0F * pow(n, 2)) + (6823.3F * n) + 5520.33F;

  /* Return the results in degrees Kelvin */
  return (uint16_t)cct;
}

// Converts the raw R/G/B values to lux

uint16_t calculateLux(uint16_t r, uint16_t g, uint16_t b)
{
  float illuminance;

  /* This only uses RGB ... how can we integrate clear or calculate lux */
  /* based exclusively on clear since this might be more reliable?      */
  illuminance = (-0.32466F * r) + (1.57837F * g) + (-0.73191F * b);

  return (uint16_t)illuminance;
}


uint8_t inInterval(uint16_t low, uint16_t high, uint16_t val)
{
	return (val>=low) && (val <= high);
}
	
// check if the car passed the line
uint8_t checkLinePassed()
{
	uint16_t R_low = 0, R_high = 1300;
	uint16_t B_low = 0, B_high = 900;
	uint16_t G_low  = 0, G_high = 1300;
	
	uint16_t r, g, b, c;
	
	// read sensor data.
	getRawData(&r, &g, &b, &c);
	
	// check if rgb values are in range
	return  inInterval(R_low, R_high, r) && inInterval(G_low, G_high, g) && inInterval(B_low, B_high, b);

}
	

