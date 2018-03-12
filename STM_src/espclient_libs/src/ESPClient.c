#include "ESPClient.h"
#include "string.h"

#define OUTQ_COUNT(e) ((uint16_t) (e->OUTQueue.in - e->OUTQueue.out))
#define MSGQ_COUNT(e) ((uint16_t) (e->MSGQueue.in - e->MSGQueue.out))
#define MSGQ_START ((uint16_t) (e->MSGQueue.out & (MSGQUEUE_SIZE-1)))
#define MSGQ_END ((uint16_t) (e->MSGQueue.in & (MSGQUEUE_SIZE-1)))

void ESPClientInit(ESPClient* e, USART_TypeDef* _USART)
{
	uint8_t i;
	for (i = 0 ; i< _NB_OF_CMDS; i++)
		e->cb_vec[i] = NULL;
	// configure queue states
	e->MSGQueue.in = 0;
	e->MSGQueue.out = 0;
	
	e->OUTQueue.in = 0;
	e->OUTQueue.out = 0;
	e->sending = 0;
	// save refrence to usart
	e->_USART = _USART;
	
	#ifdef DEBUG_EN
		e->msg_recieved =0;
		e->msg_sent = 0;
		e->crc_fails = 0;
	#endif
}

uint8_t ESPClientAttachCallback(ESPClient* e, uint8_t cmd, void(* cb)(ESPPacket*))
{
	if(cmd > _NB_OF_CMDS)
	{
		#ifdef DEBUG_EN
			printf("Invalid cmd value! Check _NB_OF_CMDS value!\r\n");
		#endif
		return 0;
	}
	#ifdef DEBUG_EN
		if(e->cb_vec[cmd] != NULL)
			printf("Overriding calback! cmd_id: %d cb_value: %p\r\n", cmd, cb);
	#endif
		
	e->cb_vec[cmd] = cb;
	#ifdef DEBUG_EN
		printf("Callback added! cmd_id: %d cb_value: %p\r\n", cmd, cb);
	#endif
		
	return 1;
}


void ESPClientParseMessages(ESPClient* e)
{
	static uint8_t isEsc = 0, buf[MAX_MSG_LEN];
	static uint16_t msgLen = 0;
	
	uint8_t value;
	ESPPacket packet;

	uint32_t end = e->MSGQueue.in, timeElapsed;
	void(*cb)(ESPPacket* );
	/*Parse only what is in the queue at the start of function call to avoid 
		blocking the main thread. */
	while(e->MSGQueue.out < end)
	{
		value = e->MSGQueue.buf[(e->MSGQueue.out++)&(MSGQUEUE_SIZE-1)];
		switch(value)
		{
			case SLIP_ESC:
				isEsc = 1; break;
			case SLIP_END:
				
				// if message has correct length (mimum packet size of 4 bytes)
				// and the passes crc test. 
				if( msgLen >= 4)
				{
					clearPacket(&packet);
					// extract packet header from buffer. 
					packet.cmd = buf[0];
					packet.len = buf[1];
					// get payload if any
					if(packet.len)
						packet.payload = buf + 2;
					else 
						packet.payload = NULL;
					// get terminating crc, last two bytes.
					packet.crc = *((uint16_t *) (buf + msgLen - 2));
					
					if(( (packet.len + 4) == msgLen) && (packet.crc == crc16Data(buf, msgLen-2, 0)))
					{
						// extract the callback function pointer. 
						if(packet.cmd <_NB_OF_CMDS)
						{
							cb = e->cb_vec[packet.cmd];
							/*if calback exists call it.*/
							(*cb)(&packet);
							
							/*a valid message has be recieved, store last message timestamp.*/
							e->lastMsgTimeStamp =  msTicks;
							#ifdef DEBUG_EN
								e->msg_recieved++;
								printf("CMD executed! CMD: %d\r\n", packet.cmd);
							#endif
						}						
						#ifdef DEBUG_EN
						else 
						{
								printf("No callback implemented! CMD: %d\r\n", packet.cmd);
						}
						#endif
					}
					#ifdef DEBUG_EN
					else
					{
						// check which condition failed.
						if(packet.len + 4 !=  msgLen)
							printf("Incorrect message length! Message length: %d, Recieved length: %d\r\n", packet.len,  msgLen);
						
						if(packet.crc != crc16Data(buf, msgLen-2, 0))
						{
							e->crc_fails++;
							printf("CRC16 checksum failed! Message length: %d, number of fails: %d\r\n", msgLen,  e->crc_fails);
							printf("CRC recieved: %d, CRC computed: %d", packet.crc, crc16Data(buf, msgLen-2, 0));
						}
					}
					#endif
				}

				/*Clear message length and is esc flag.*/
				msgLen = 0;
				isEsc = 0;
				break;
			default:
					if(isEsc)
					{
						if(value == SLIP_ESC_END) value = SLIP_END;
						else if(value == SLIP_ESC_ESC) value = SLIP_ESC;
						
						isEsc = 0;
					}
					
					if(msgLen < MAX_MSG_LEN)
					{
						buf[msgLen++] = value;
						#ifdef DEBUG_EN
							printf("%c", (char)value);
						#endif
					}
					#ifdef DEBUG_EN
					else 
					{
						printf("Message exceded available storage, consider increasing MAX_MSG_LEN!\r\n");
					}
					#endif
		}		
	}
	
	// if the time since the last valid message is greater then sendKeepAliveInterval
//	timeElapsed = msTicks -  e->lastMsgTimeStamp;
//	if(timeElapsed >= timeoutInterval)
//	{
//		cb = e->cb_vec[CONNECTION_TIMEOUT];
//		(*cb)(NULL);
//		// reset timer.
//		e->lastMsgTimeStamp =  msTicks;
//	}
	
}


void ESPClientISRHandler(ESPClient* e)
{
	if(USART_GetITStatus(e->_USART, USART_IT_RXNE) != RESET)
	{
		if(((e->MSGQueue.in - e->MSGQueue.out)& ~(MSGQUEUE_SIZE-1) ) == 0)
		{
			#ifdef DEBUG_EN
				if(((e->MSGQueue.in) & (MSGQUEUE_SIZE-1)) >= MSGQUEUE_SIZE)
					printf("ERROR! Out of bounds MSGQ!");
			#endif
			e->MSGQueue.buf[(e->MSGQueue.in++) & (MSGQUEUE_SIZE-1)] = (uint8_t)USART_ReceiveData(e->_USART);
		}
	}
	
	if(USART_GetITStatus(e->_USART, USART_IT_TXE) != RESET)
	{
		if(e->OUTQueue.out != e->OUTQueue.in)
		{
			#ifdef DEBUG_EN
				if((e->OUTQueue.out & (OUTQUEUE_SIZE-1)) >= OUTQUEUE_SIZE)
					printf("ERROR! Out of bounds MSGQ!");
			#endif
				
			USART_SendData(e->_USART, e->OUTQueue.buf[e->OUTQueue.out & (OUTQUEUE_SIZE-1)]);
			e->OUTQueue.out++;
			e->sending = 1;
		}else
		{
			e->sending = 0;
			USART_ITConfig(e->_USART, USART_IT_TXE, DISABLE);
		}
	}
}


/*
	Writes the escaped sequence to the output queue.
	Return value:
	0 - no error
	1 - out queue is full				
*/
uint8_t _WriteEscapedBuffer(ESPClient* e,uint8_t*buf, uint8_t len) 
{
	uint8_t i;
	for(i = 0; i < len; i++)
	{
		if(((e->OUTQueue.in - e->OUTQueue.out)& ~(OUTQUEUE_SIZE-1) ) != 0)
		{
		#ifdef DEBUG_EN
			printf("Out queue is full!\r\n");
		#endif
			return 1;
		}
			
		switch(buf[i])
		{
			case SLIP_ESC:
				e->OUTQueue.buf[(e->OUTQueue.in++) & (OUTQUEUE_SIZE-1)] = SLIP_ESC;
				e->OUTQueue.buf[(e->OUTQueue.in++) & (OUTQUEUE_SIZE-1)] = SLIP_ESC_ESC;
				break;
			case SLIP_END:
				e->OUTQueue.buf[(e->OUTQueue.in++) & (OUTQUEUE_SIZE-1)] = SLIP_ESC;
				e->OUTQueue.buf[(e->OUTQueue.in++) & (OUTQUEUE_SIZE-1)] = SLIP_ESC_END;
				break;
			default:
				e->OUTQueue.buf[(e->OUTQueue.in++) & (OUTQUEUE_SIZE-1)] = buf[i];
		}
	}
	
	/*Start sending data.*/
	if(!e->sending)
	{
		e->sending = 1;
		USART_SendData(e->_USART, e->OUTQueue.buf[(e->OUTQueue.out++)&(OUTQUEUE_SIZE - 1)]);
		USART_ITConfig(e->_USART, USART_IT_TXE, ENABLE);
	}
	
	return 0;
}

/*
	Writes the escaped byte to the output queue.
	Return value:
	0 - no error
	1 - out queue is full				
*/

uint8_t _WriteEscapedByte(ESPClient* e, uint8_t data) 
{
	return _WriteEscapedBuffer(e, &data, 1);
}
/*
	Writes the escaped Short to the output queue.
	Return value:
	0 - no error
	1 - out queue is full				
*/

uint8_t _WriteEscapedShort(ESPClient* e, uint16_t data) 
{
	return _WriteEscapedBuffer(e, (uint8_t* )&data, 2);
}

/*
	Writes the SLIP_END byte to the output queue.
	Return value:
	0 - no error
	1 - out queue is full				
*/
uint8_t _WriteDelimiter(ESPClient* e)
{
	if(((e->OUTQueue.in - e->OUTQueue.out)& ~(OUTQUEUE_SIZE-1) ) != 0)
		{
		#ifdef DEBUG_EN
			printf("Out queue is full!\r\n");
		#endif
			return 1;
		}
		
	e->OUTQueue.buf[(e->OUTQueue.in++) & (OUTQUEUE_SIZE-1)] = SLIP_END;
	
	/*Start sending data.*/
	if(!e->sending)
	{
		e->sending = 1;
		USART_SendData(e->_USART, e->OUTQueue.buf[(e->OUTQueue.out++)&(OUTQUEUE_SIZE - 1)]);
		USART_ITConfig(e->_USART, USART_IT_TXE, ENABLE);
	}
	
	return 0;
}


/*
	Writes the packet to the output queue.
	Return value:
	0 - no error
	1 - out queue is full				
*/
uint8_t ESPClientSendData(ESPClient* e, ESPPacket* p)
{
	uint8_t rez = 0;
	/*Write start byte*/
	rez = rez | _WriteDelimiter(e);
	/*Write command*/
	rez = rez | _WriteEscapedByte(e, p->cmd);
	/*Write message length*/
	rez = rez | _WriteEscapedByte(e, p->len);
	/*Write the payload*/
	rez = rez | _WriteEscapedBuffer(e, p->payload, p->len);
	/*Write crc*/
	rez = rez | _WriteEscapedShort(e, p->crc);
	/*Write stop byte.*/
	rez = rez | _WriteDelimiter(e);
	
	return rez;
}


/*
	Writes the escaped sequence to the output queue.
	Return value:
	0 - no error
	1 - out queue is full				
*/
uint8_t ESPClientSendDataBuffer(ESPClient* e, uint8_t* buf, uint16_t len)
{
	uint8_t i;
	for(i = 0; i < len; i++)
	{
		if(((e->OUTQueue.in - e->OUTQueue.out)& ~(OUTQUEUE_SIZE-1) ) != 0)
		{
		#ifdef DEBUG_EN
			printf("Out queue is full!\r\n");
		#endif
			return 1;
		}
			
		e->OUTQueue.buf[(e->OUTQueue.in++) & (OUTQUEUE_SIZE-1)] = buf[i];

	}
	
	/*Start sending data.*/
	if(!e->sending)
	{
		e->sending = 1;
		USART_SendData(e->_USART, e->OUTQueue.buf[(e->OUTQueue.out++)&(OUTQUEUE_SIZE - 1)]);
		USART_ITConfig(e->_USART, USART_IT_TXE, ENABLE);
	}
	
	return 0;
}


void ESPSendKeepAliveMSG(ESPClient* e)
{
	ESPPacket p;
	clearPacket(&p);
	setCommand(&p, KEEPALIVE);
	ESPClientSendData(e, &p);
}


void clearPacket(ESPPacket* p) 
{
	p->cmd = 0;
	p->len = 0;
	p->payload = NULL;
	p->crc = 0;
	p->_cpos = 0;
}

void setCommand(ESPPacket* p, uint8_t cmd)
{
	p->cmd = cmd;
	p->crc = crc16Add(cmd, 0);
}

void setPayloadBuffer(ESPPacket* p, uint8_t* buf)
{
	p->payload = buf;
}


void putByte(ESPPacket* p, uint8_t data)
{
	p->payload[p->len++] = data;
}
void putShort(ESPPacket* p, uint16_t data)
{
	p->payload[p->len++] = data & 0x00ff;
	p->payload[p->len++] = (data & 0xff00) >> 8;
}
void putInt(ESPPacket* p, uint32_t data)
{
	p->payload[p->len++] = (data & 0x000000ff);
	p->payload[p->len++] = (data & 0x0000ff00) >> 8;
	p->payload[p->len++] = (data & 0x00ff0000) >> 16;
	p->payload[p->len++] = (data & 0xff000000) >> 24;
}
void putFloat(ESPPacket* p, float data)
{
	// cast to uint32
	putInt(p, *((uint32_t*) &data));
}
void updateCrc(ESPPacket* p)
{
	p->crc = crc16Add(p->cmd, 0);
	p->crc = crc16Add(p->len, p->crc);
	p->crc = crc16Data(p->payload, p->len, p->crc);
}

uint8_t getByte(ESPPacket* p)
{
	return p->payload[p->_cpos++];
}

uint16_t getShort(ESPPacket* p)
{
	return p->payload[p->_cpos++] |  p->payload[p->_cpos++]<<8;
}

uint32_t getInt(ESPPacket* p)
{
	return p->payload[p->_cpos++] |  p->payload[p->_cpos++]<<8 | 
				 p->payload[p->_cpos++] << 16|p->payload[p->_cpos++] << 24;
}
float getFloat(ESPPacket* p) 
{
	int32_t val = getInt(p);
	float *pval = (float * )&val;
	return *pval;
}


/*CRC16 implementation.*/
uint16_t crc16Add(uint8_t b, uint16_t acc)
{
	
	acc = ((acc>>8) | (acc <<8)) & 0xffff;
	acc ^= b & 0xff;
	acc ^= (acc & 0xff) >> 4;
	acc ^= (acc << 12) & 0xffff;
	acc ^= ((acc & 0xff) << 5) & 0xffff;
	
	return acc;
}
uint16_t crc16Data(uint8_t *data, uint16_t len, uint16_t acc)
{
	uint16_t i;
	for ( i = 0; i<len; i++)
		acc = crc16Add(*data++, acc);
	return acc;
}

uint8_t* ParseShort(uint8_t* buff, uint16_t* data)
{
	*data = buff[0] + (buff[1]<<8);
	return buff + 2;
}
uint8_t* PutShort(uint8_t* buff, uint16_t data)
{
	*buff++ = data & 0xff;
	*buff++ = data >> 8;
	return buff;
}


// initialize USART1 and configure ESP power down pin
void ESPClientLowLevelInit(void)
{
	GPIO_InitTypeDef GPIO_InitStruct;
	NVIC_InitTypeDef NVIC_InitStruct;
	USART_InitTypeDef USART_InitStruct;
	
	/* Enable gpio clock */
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA| RCC_APB2Periph_AFIO, ENABLE);
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_USART1, ENABLE);
	
	/* Config USART1 GPIO RX input floating.*/
	GPIO_InitStruct.GPIO_Pin = GPIO_Pin_10; 
	GPIO_InitStruct.GPIO_Mode = GPIO_Mode_IN_FLOATING;
	GPIO_InitStruct.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_Init(GPIOA, &GPIO_InitStruct);
		
	/* Config USART1 GPIO TX AF-PP*/
	GPIO_InitStruct.GPIO_Pin = GPIO_Pin_9;
	GPIO_InitStruct.GPIO_Mode = GPIO_Mode_AF_PP;
	GPIO_InitStruct.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_Init(GPIOA, &GPIO_InitStruct);
	
	
	/* Configure NVIC for USART1*/
	NVIC_PriorityGroupConfig(NVIC_PriorityGroup_0);
	NVIC_InitStruct.NVIC_IRQChannel = USART1_IRQn;
	NVIC_InitStruct.NVIC_IRQChannelPreemptionPriority = 0x2;
	NVIC_InitStruct.NVIC_IRQChannelSubPriority = 0;
	NVIC_InitStruct.NVIC_IRQChannelCmd = ENABLE;
	NVIC_Init(&NVIC_InitStruct);
	
	
	/* Configure USART2 settings*/
	USART_InitStruct.USART_BaudRate = 115200;
	USART_InitStruct.USART_WordLength = USART_WordLength_8b;
	USART_InitStruct.USART_StopBits = USART_StopBits_1;
	USART_InitStruct.USART_Parity = USART_Parity_No;
	USART_InitStruct.USART_HardwareFlowControl = USART_HardwareFlowControl_None;
	USART_InitStruct.USART_Mode = USART_Mode_Rx|USART_Mode_Tx;
	
	/*Configure USART1 settings*/
	USART_Init(USART1, &USART_InitStruct);
	
	/*Configure interrupts*/
	USART_ITConfig(USART1, USART_IT_RXNE, ENABLE);
	
	/* Enable USART1*/
	USART_Cmd(USART1, ENABLE);

	/* Configure power down pin*/
	/* GPIOA Pin 1 as out PP*/
	GPIO_InitStruct.GPIO_Mode = GPIO_Mode_Out_PP;
	GPIO_InitStruct.GPIO_Pin =  GPIO_Pin_1;
	GPIO_InitStruct.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_Init(GPIOA, &GPIO_InitStruct);
	
	
}

// toggle ESP enable pin to ensure correct start.
void ESPTogglePower(void)
{
	GPIO_WriteBit(GPIOA, GPIO_Pin_1, Bit_RESET);
	delay(500);
	GPIO_WriteBit(GPIOA, GPIO_Pin_1, Bit_SET);
}
