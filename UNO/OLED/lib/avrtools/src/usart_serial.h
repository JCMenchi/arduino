#ifndef _USART_SERIAL_H
#define _USART_SERIAL_H

#include <stdint.h>


#define BAUD_RATE_9600    1
#define BAUD_RATE_57600   2
#define BAUD_RATE_115200  3

void USART_Init(uint8_t baudrate, volatile void (*usart_rec_cb)(uint8_t,bool));

void USART_Transmit(uint8_t data);
uint8_t USART_Receive();

void USART_WriteString(const char* str);

void USART_WriteInt(int32_t i);
void USART_WriteUInt(uint32_t i);

uint8_t USART_GetLastChar();

#endif
