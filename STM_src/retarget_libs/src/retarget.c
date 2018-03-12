#include "retarget.h"

void USART2_IRQHandler(void)
{
	buf_st *p;
	if(USART_GetITStatus(USART2, USART_IT_RXNE) != RESET)
	{
		p = &rbuf;
		if(((p->end - p->start)& ~(QBUF_SIZE-1) ) == 0)
		{
			p->buf[(p->end++) & (QBUF_SIZE-1)] = USART_ReceiveData(USART2);
		}
	}
	
	if(USART_GetITStatus(USART2, USART_IT_TXE) != RESET)
	{
		p = &tbuf;
		if(p->end != p->start)
		{
			USART_SendData(USART2, p->buf[p->start & (QBUF_SIZE-1)]);
			p->start++;
			sending = 1;
		}else
		{
			sending = 0;
			USART_ITConfig(USART2, USART_IT_TXE, DISABLE);
		}
	}
}


int SendChar(char c)
{
	buf_st *p = &tbuf;
	if(QBUF_COUNT(p) >= QBUF_SIZE)
		return -1;
	p->buf[(p->end++) & (QBUF_SIZE - 1)] = c;
	
	if(!sending)
	{
		sending = 1;
		USART_SendData(USART2, p->buf[(p->start++)&(QBUF_SIZE - 1)]);
		USART_ITConfig(USART2, USART_IT_TXE, ENABLE);
	}
	return 0;
}

int GetChar(void)
{
	buf_st *p = &rbuf;
	if(QBUF_COUNT(p) == 0)
		return -1;
	
	return (p->buf[(p->start++)&(QBUF_SIZE - 1)]);
}


/*Retarget stuff here:*/

struct __FILE {
	int handle;
};

FILE __stdout;
FILE __stdin;

int fputc(int ch, FILE*f){
	while(SendChar(ch) == -1);
	return 0;
}

int fgetc(FILE* f)
{
	int ch;
	do
	{
		ch = GetChar();
	}while(ch == -1);
	//SendChar(ch);
	return ch;
}


void _ttywrch(int ch)
{
	SendChar(ch);
}

int ferrror(FILE*f)
{
	return EOF;
}

void _sys_exit(int return_code)
{
	label: goto label;
}

void InitDebugUSART(void)
{
	GPIO_InitTypeDef GPIO_InitStruct;
	NVIC_InitTypeDef NVIC_InitStruct;
	USART_InitTypeDef USART_InitStruct;
	
	/* Enable clocks */
	/* Enable gpio clock */
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA| RCC_APB2Periph_AFIO, ENABLE);
	
	/* Enable usart clock */
	RCC_APB1PeriphClockCmd(RCC_APB1Periph_USART2, ENABLE);
	
	/* Configure GPIO */
	/* Config USART2 GPIO RX input floating.*/
	GPIO_InitStruct.GPIO_Pin = GPIO_Pin_3; 
	GPIO_InitStruct.GPIO_Mode = GPIO_Mode_IN_FLOATING;
	GPIO_InitStruct.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_Init(GPIOA, &GPIO_InitStruct);	
	
	/* Config USART2 GPIO TX AF-PP*/
	GPIO_InitStruct.GPIO_Pin = GPIO_Pin_2;
	GPIO_InitStruct.GPIO_Mode = GPIO_Mode_AF_PP;
	GPIO_InitStruct.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_Init(GPIOA, &GPIO_InitStruct);
	
	
	/*Configure NVIC for USART2*/
	NVIC_InitStruct.NVIC_IRQChannel = USART2_IRQn;
	NVIC_InitStruct.NVIC_IRQChannelPreemptionPriority = 0x3;
	NVIC_InitStruct.NVIC_IRQChannelSubPriority = 0x3;
	NVIC_InitStruct.NVIC_IRQChannelCmd = ENABLE;
	NVIC_Init(&NVIC_InitStruct);
	 
	/*Configure USART2 settings*/
	USART_InitStruct.USART_BaudRate = 115200;
	USART_InitStruct.USART_WordLength = USART_WordLength_8b;
	USART_InitStruct.USART_StopBits = USART_StopBits_1;
	USART_InitStruct.USART_Parity = USART_Parity_No;
	USART_InitStruct.USART_HardwareFlowControl = USART_HardwareFlowControl_None;
	USART_InitStruct.USART_Mode = USART_Mode_Rx|USART_Mode_Tx;
	USART_Init(USART2, &USART_InitStruct);
	
	/*Configure interrupts*/
	USART_ITConfig(USART2, USART_IT_RXNE, ENABLE);
	
	/* Enable USART2*/
	USART_Cmd(USART2, ENABLE);
}


