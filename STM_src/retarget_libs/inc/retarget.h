#ifndef __RETARGET_H
#define __RETARGET_H

#include <misc.h>
#include <stdio.h>
#include "stm32f10x_usart.h"

#pragma import(__use_no_semihosting_swi)
#define QBUF_SIZE 2048
#define QBUF_COUNT(b) ((uint16_t) (b->end - b->start))
/*Buffer structure used for rx and tx of debug usart*/
typedef struct {
	uint32_t start;
	uint32_t end;
	char buf[QBUF_SIZE];
}buf_st;


static buf_st rbuf = {0, 0};
static buf_st tbuf = {0, 0};
static volatile uint8_t sending = 0;

void USART2_IRQHandler(void);
int SendChar(char c);
int GetChar(void);
int fputc(int ch, FILE*f);
int fgetc(FILE* f);
void _ttywrch(int ch);
int ferrror(FILE*f);
void _sys_exit(int return_code);

void InitDebugUSART(void);

#endif
