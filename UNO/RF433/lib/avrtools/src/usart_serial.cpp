
#include "usart_serial.h"

#include <stdlib.h>

#include <avr/io.h>
#include <avr/interrupt.h>


#if defined(__AVR_ATmega8__)

#define _UBRRH UBRRH
#define _UBRRL UBRRL
#define _UCSRA UCSRA
#define _UCSRB UCSRB
#define _UDR UDR
#define _RXEN RXEN
#define _TXEN TXEN
#define _RXCIE RXCIE
#define _UDRE UDRE
#define _U2X U2X

#define _UCSRC UCSRC

#else

#define _UBRRH UBRR0H
#define _UBRRL UBRR0L
#define _UCSRA UCSR0A
#define _UCSRB UCSR0B
#define _UDR UDR0
#define _RXEN RXEN0
#define _TXEN TXEN0
#define _RXCIE RXCIE0
#define _UDRE UDRE0
#define _U2X U2X0

#define _UCSRC UCSR0C
#define _RXC RXC0

#endif

/*
  Baud rate selection based on official AVR doc:

    AVR306: Using the AVR USART on tinyAVR and megaAVR devices

*/
void USART_SetBaudRate(uint8_t baudrate) {
  uint16_t br = 0;

  // TODO: this is only for 16MHz clock, need to update for 8MHz
  if (baudrate == BAUD_RATE_9600) {
    br = 103;
    _UCSRA = 0;
  } else if (baudrate == BAUD_RATE_57600) {
    br = 16;
    _UCSRA = 0;
  } else if (baudrate == BAUD_RATE_115200) {
    br = 16;
    _UCSRA = (1 << U2X0); // use 2X to lower error
  }

  _UBRRL = (unsigned char)(br & 0xFF);
  _UBRRH = (unsigned char)(br >> 8);
}

volatile void (*USART_REC_CB)(uint8_t,bool) = NULL;

void USART_Init(uint8_t baudrate, volatile void (*usart_rec_cb)(uint8_t, bool)) {
  USART_REC_CB = usart_rec_cb;

	USART_SetBaudRate(baudrate);
  // Enable Receiver and Transmitter and Interrupt
  _UCSRB = (1 << _RXEN) | (1 << _TXEN) | (1 << RXCIE0);
  // Set frame format: 8-bit (UCSZ=011), no parity (UPM=00), 1-stop bit (USBS=0)
  _UCSRC = 0 | (1 << UCSZ00) | (1 << UCSZ01);
  _UCSRC = 0x06;
}

uint8_t USART_Receive() {
  /* Wait for data to be received */
  while (!(_UCSRA & (1 << _RXC)))
    ;
  /* Get and return received data from buffer */
  return _UDR;
}


volatile uint8_t last_char = 0;

ISR(USART_RX_vect) 
{ 
  bool error = !bit_is_clear(_UCSRA, FE0);
  last_char = _UDR;
  if (USART_REC_CB != NULL) {
    (*USART_REC_CB)(last_char, error);
  }
}


uint8_t USART_GetLastChar() {
  return last_char;
}

void USART_Transmit(unsigned char data) {
  /* Wait for empty transmit buffer */
  while (!(_UCSRA & (1 << _UDRE)))
    ;
  /* Put data into buffer, sends the data */
  _UDR = data;
}

void USART_WriteString(const char* str) {
  while (*str)
    USART_Transmit(*str++);
}


char numberbuffer[12];

void USART_WriteInt(int32_t i) {
  ltoa(i, numberbuffer, 10);
  USART_WriteString(numberbuffer);
}

void USART_WriteUInt(uint32_t i) {
  ultoa(i, numberbuffer, 10);
  USART_WriteString(numberbuffer);
}
