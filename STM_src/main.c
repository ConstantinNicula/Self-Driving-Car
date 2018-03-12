#include "stm32f10x.h"
#include "Setup.h"
#include "ESPClient.h"
#include "motorControl.h"
#include "delay.h"
#include "Sensorbar.h"
#include "control.h"
#include "battery.h"
#include "LSM6DS33.h"
#include "TCS34725.h"
#include "string.h"

//#define TRACK_HAS_OBSTACLE
//#define TEST_MODE
//#define TIME_PENALTY

//#define USE_BINARY
uint32_t start_time;

uint16_t nr = 0;

volatile uint32_t msTicks;
/*
volatile uint32_t time = 0;
	uint32_t t = 1, start=0, end;\
	
	*/
float count_pickup = 0;
	
	
/*Car state parameters.*/
typedef enum
{
	CAR_OFF = 0x0,
	CAR_ON = 0x1
}CAR_STATES;


typedef enum
{
	MANUAL = 0x0,
	AUTO = 0x1
}CAR_MODES;

volatile uint8_t running_state = CAR_OFF;
volatile uint8_t mode_state = MANUAL;


/**/
	
ESPClient client;

void KeepAliveCallback(ESPPacket* );
void SetModeCallback(ESPPacket* );
void UpdateSpeedCallback(ESPPacket* );
void UpdateSteeringAngleCallback(ESPPacket* );
void ConnectionTimedOutCallback(ESPPacket* ); //ESPPacket* is allways null
void StartCallback(ESPPacket* );
void StopCallback(ESPPacket* );
void GainUpdateCallback(ESPPacket* );


void sendCarStatus(void);

int main()
{
	float new_heading;
	uint16_t r, g, b, c;
	/*Init board clock*/
	InitBoard();
	
	/*Init USART for debug and communictaion*/
	InitDebugUSART();
	
	/*Init motor control libs*/
	initEscControlPin();
	initServoControlPin();
	initMotorsTimer();
	
	/*Enable encoder*/
	initEncoder();
	startReadingEncoder();
		
	/*Configure esp client*/

	ESPClientInit(&client, USART1);
	ESPClientAttachCallback(&client, KEEPALIVE, &KeepAliveCallback);
	ESPClientAttachCallback(&client, SET_MODE, &SetModeCallback);
	ESPClientAttachCallback(&client, UPDATE_SPEED, &UpdateSpeedCallback);
	ESPClientAttachCallback(&client, UPDATE_STEERING_ANGLE, &UpdateSteeringAngleCallback);
	ESPClientAttachCallback(&client, CONNECTION_TIMEOUT, &ConnectionTimedOutCallback);
	ESPClientAttachCallback(&client, START, &StartCallback);
	ESPClientAttachCallback(&client, STOP, &StopCallback);
	ESPClientAttachCallback(&client, UPDATE_GAIN, &GainUpdateCallback);
	
	/* Configure ESP USART for communicaion */ 
	ESPClientLowLevelInit();
	/* Toggle ESP Pin to ensure correct start */
	ESPTogglePower();
	
	/*Facing forward, with zero velocity.*/
	setEscPWM(1500);	
	setServoPWM(1500);
	
	/*Init Sensor bar*/
	InitSensorBar();
	
	/*Init status reading for battery and temp sensors.*/
	initBatteryADC();
	
	/*Init accelerometer and gyro.*/
	//initLSM6DS33();

	/*Init color sensor */
	initColorSensor();

	while(1)
	{	
		// parse incoming messages
		ESPClientParseMessages(&client);
		// get battery voltage
		getBatteryVoltage();
		
		// read sensor bar data.
		ReadSensorData();

		// read accelerometer data.
		
		// if the car is in running state 
		if((mode_state ==  AUTO) && (running_state == CAR_ON))
		{
			// get new servo direction
			new_heading = computeNewHeading();
			//update servo position.
			setServoPWM(getPWM_from_angle(new_heading));
			
			#ifdef TIME_PENALTY
			// check if line passed.		
			if((msTicks - start_time) > 133600)
			{
				//lower motor speed
				setEscPWM(1590);
			
			
				if(checkLinePassed())
				{
					// stop motor.
					setEscPWM(1000); // brake
					delay(700);
					setEscPWM(1500); // no throttle.
					// center servo.
					setServoPWM(1500);
					
					running_state = CAR_OFF;
				}
			}
			#endif
		}
		
		// sensor reading is complete send car status to phone.
		sendCarStatus();	

		}

}

void ConnectionTimedOutCallback(ESPPacket* p) 
{
	// if in  manual mode
	if(mode_state == MANUAL)
	{
		// stop car and center servo
		setEscPWM(1500);
		setServoPWM(1500);
		// display timeout
		//printf("Disconnected!/n");
	}
}
void KeepAliveCallback(ESPPacket*p)
{
	uint16_t val = getShort(p);
	
	printf("Data recieved: %d\r\n", val);
	printf("Sending data!\r\n");
}

void SetModeCallback(ESPPacket*p)
{
	uint16_t mode = getShort(p);
	/*Change car working mode... TODO*/
	
	if(mode == 0)
	{
		mode_state = MANUAL;
	}
	else if(mode == 1)
	{
		mode_state = AUTO;
	}	
	running_state = CAR_OFF;
	// set speed
	setEscPWM(1500);
	// set servo
	setServoPWM(1500);

	/*Echo commmand.*/
	ESPClientSendData(&client, p);
}

void UpdateSpeedCallback(ESPPacket *p)
{
	// if in  manual mode
	if(mode_state == MANUAL && running_state == CAR_ON)
	{
		int16_t speed = getShort(p);
		//printf("Motor: %d\r\n", speed);
		speed  =  1500 + 1.40*speed; 
		if((speed <= 2000) && (speed >= 1000))
			setEscPWM(speed);
		else 
			setEscPWM(1500);
	}
	/*Change motor speed... TODO*/
	/*Echo commmand.*/
	ESPClientSendData(&client, p);
}

void UpdateSteeringAngleCallback(ESPPacket *p)
{
	if(mode_state == MANUAL && running_state == CAR_ON)
	{
		int16_t angle =  getShort(p);

		angle = 1500 + 4 * angle;
		setServoPWM(angle);
	}
	/*Change steering angle... TODO*/

	/*Echo commmand.*/
	ESPClientSendData(&client, p);
}


void StartCallback(ESPPacket* p)
{
	int16_t speed;
	
	printf("Starting car!\n");
	// update running state.
	running_state = CAR_ON;
	
	//if in automode set desired esc speed.
	if(mode_state == AUTO)
	{
		//record start time
		start_time = msTicks;
		//set speed 
		speed = 1680;
		
		if((speed <= 2000) && (speed >= 1000))
			setEscPWM(speed);
		else 
			setEscPWM(1500);
		
		// store initial position information
		getInitialReadings();
	}

	/*echo command back.*/
	ESPClientSendData(&client, p);	
	
	/*set count pickup to zero*/
	count_pickup = 0;
}
void StopCallback(ESPPacket* p)
{
	printf("Stopping car!\n");
	// update running state to stop.
	if(mode_state == AUTO && running_state == CAR_ON )
	{
		// stop motor.
		setEscPWM(1300); // brake
		delay(700);
		setEscPWM(1500); // no throttle.
		// center servo.
		setServoPWM(1500);
	}
	running_state = CAR_OFF;
	/*echo command back.*/
	ESPClientSendData(&client, p);	
}
void GainUpdateCallback(ESPPacket* p)
{
	// update gain.
	gain = getFloat(p);
	printf("Adjusting gain: %f", gain);
	
	/*echo command back.*/
	ESPClientSendData(&client, p);	
}


void sendCarStatus(void)
{
	static uint32_t prevt;
	// if 100 ms elapsed since last transmission.
	if(msTicks - prevt >=  60)
	{
		#ifdef USE_BINARY
		// create a new packet.
		ESPPacket p;
		uint8_t buffer[30];
		clearPacket(&p);
		setCommand(&p, UPDATE_STATS);
		setPayloadBuffer(&p, buffer);
		
		// send car velocity as float.
		putFloat(&p, velocity);
		
		// send battery level as float.
		putFloat(&p, batteryLevel);
		
		// send sensor values as as short.
		putShort(&p, (short) (sensorReadings[0]*1000.));
		putShort(&p, (short) (sensorReadings[1]*1000.));
		putShort(&p, (short) (sensorReadings[2]*1000.));
		putShort(&p, (short) (sensorReadings[3]*1000.));
		
		// update crc before sending.
		updateCrc(&p);
		// send packet to mobile phone.
		ESPClientSendData(&client, &p);
		#else 
		char buf[256];
		
		sprintf(buf,  "$cmd:%d,vel:%f,bat:%f,s1:%f,s2:%f,s3:%f,s4:%f;", UPDATE_STATS, velocity, batteryLevel, sensorReadings[0], sensorReadings[1], sensorReadings[2], sensorReadings[3]);
		ESPClientSendDataBuffer(&client, (uint8_t *)buf, strlen(buf));
		
		#endif
		
		
		//store current time
		prevt = msTicks;
	
	}
}
void USART1_IRQHandler(void)
{	
	ESPClientISRHandler(&client);
}

