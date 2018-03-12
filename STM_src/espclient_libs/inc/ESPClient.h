#ifndef __ESPCLIENT_H
#define __ESPCLIENT_H

#include "stdint.h"
#include "Delay.h"
#include "stm32f10x_usart.h"
#include <stdio.h>


/*
	Enable debug messages to USART2; 
	WARNING: retargets printf(), and scanf(), semihosting will no longer work.
*/

//#define DEBUG_EN /*Comment(uncomment) this line to disable/(enable)*/
#include "retarget.h"

#define MSGQUEUE_SIZE  512
#define OUTQUEUE_SIZE  512
#define MAX_MSG_LEN 128

#define timeoutInterval 300 // 300 ms


/*Used for message framing*/
#define SLIP_END      0300    /**< Indicates end of packet */
#define SLIP_ESC      0333    /**< Indicates byte stuffing */
#define SLIP_ESC_END  0334    /**< ESC ESC_END means END data byte */
#define SLIP_ESC_ESC  0335    /**< ESC ESC_ESC means ESC data byte */

typedef enum
{
	NULLCMD = 0,
	KEEPALIVE, /*command used to prevent tcp socket connection from reaching timeout.*/
	SET_MODE, /*set the current driving mode(auto, manual, test?)*/
	START, /*start motor.*/
	STOP, /*stop motor*/
	UPDATE_SPEED, /*absolute value of the motor speed.*/
	UPDATE_STEERING_ANGLE, /*absoulte value of the new steering angle(- if left, + if right)*/
	REVERSE_MOTOR_DIRECTION,/*?*/
	UPDATE_GAIN,  /*Update feedback controller gain, used when tuning online.*/
	UPDATE_STATS, /*Send feedback about the car state to the client.*/
	/*Connection relate commands.*/
	CONNECTION_TIMEOUT,
	_NB_OF_CMDS /*do not remove from enumerate, used to check number of cmds*/
}ESPCommand;

typedef struct
{
	uint8_t cmd; // command
	uint8_t len; // payload length
	uint8_t *payload; // actual data
	uint16_t crc; // crc16 
	
	uint8_t _cpos; // current position used when reading data from packets.
}ESPPacket;


/*
	ESPClient struct holds all data needed to handle transmit and recieve
	actions to the ESP module.
*/
typedef struct
{
	/*Message queue*/
	struct {
		uint8_t buf[MSGQUEUE_SIZE];
		uint32_t in;
		uint32_t out;
	} MSGQueue;
	
	/*Output queue*/
	struct {
		uint8_t buf[OUTQUEUE_SIZE];
		uint32_t in;
		uint32_t out;
	} OUTQueue;
	
	/*flag used in ISR*/
	uint8_t sending;
	
	/*Hash map for callbacks (aka. vector of function pointers) */
	/*callbacks have the following standard definition:
		- void cb_name(ESPPacket*)
	*/
	void (*cb_vec[_NB_OF_CMDS])(ESPPacket* );
	
	/*Pointer to usart*/
	USART_TypeDef* _USART;
	
	/*Used to check if client disconnected.*/
	uint32_t lastMsgTimeStamp;
	
	#ifdef DEBUG_EN
	uint32_t crc_fails;
	uint32_t msg_recieved;
	uint32_t msg_sent;
	#endif
}ESPClient;

/* Resets MSGQueue, OUTQueue and status indicators. */
void ESPClientInit(ESPClient* , USART_TypeDef* _USART);
/* 
	Register a callback function to a given ESP command.
*/
uint8_t ESPClientAttachCallback(ESPClient* , uint8_t, void(*)(ESPPacket*));
/* 
	Parse all the messges in the message queue:
	- verifiy data integrity
	- executes the corresponding callbacks.
*/
void ESPClientParseMessages(ESPClient* );

/*
	Adds SLIP-escaped data buffer to the output buffer. 
*/
uint8_t ESPClientSendData(ESPClient*, ESPPacket*);


/*
	Send data buffer over tcp
*/

uint8_t ESPClientSendDataBuffer(ESPClient*, uint8_t*, uint16_t);



/*Configure timeout timer, used to check if the connection is still active. */
void ESPSendKeepAliveMSG(ESPClient* e);
/*
	Utility funcitons for reading and writing packets.
*/
// clears all the data currently stored in the packet this function sould be called
// when reusing packets.
void setCommand(ESPPacket* p, uint8_t cmd); 
void setPayloadBuffer(ESPPacket* p, uint8_t* buf); // the same as p.payload = buf
void clearPacket(ESPPacket* p);

void putByte(ESPPacket* p, uint8_t data);
void putShort(ESPPacket* p, uint16_t data);
void putInt(ESPPacket* p, uint32_t data);
void putFloat(ESPPacket* p, float data);

void updateCrc(ESPPacket* p);
uint8_t getByte(ESPPacket* p);
uint16_t getShort(ESPPacket* p);
uint32_t getInt(ESPPacket* p);
float getFloat(ESPPacket* p);

/*
	This function should be called in the ISR of the UART used to communicate 
	with the ESP. 
	- copies the inbound bytes into the message queue.
	- send outbound bytes untill outqueue is empty.
*/
void ESPClientISRHandler(ESPClient* );


/* CRC 16 related functions.*/
uint16_t crc16Add(uint8_t b, uint16_t acc);
uint16_t crc16Data(uint8_t *data, uint16_t len, uint16_t acc);

/*Low level USART init function.*/
void ESPClientLowLevelInit(void);
void ESPTogglePower(void);
#endif  

