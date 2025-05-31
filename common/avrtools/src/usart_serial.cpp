
#include "usart_serial.h"

#include <avr/interrupt.h>
#include <avr/io.h>
#include <stdlib.h>
#include <string.h>
#include <avr/pgmspace.h>

#if defined(__AVR_ATtiny45__)

void USART_Init(uint8_t baudrate,
                volatile void (*usart_rec_cb)(uint8_t, bool)) {}

void USART_Transmit(uint8_t data) {}
uint8_t USART_Receive() { return 0; }

void USART_WriteString(const char *str) {}

void USART_WriteInt(int32_t i, uint8_t base) {}
void USART_WriteUInt(uint32_t i, uint8_t base) {}

uint8_t USART_GetLastChar() {}

#else
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

#elif defined(__AVR_ATmega8535__) || defined(__AVR_ATmega8515__) || defined(__AVR_ATmega16__) || defined(__AVR_ATmega32__)

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
#define _RXC RXC
#define _UCSZ0 UCSZ0
#define _UCSZ1 UCSZ1
#define _FE FE

#elif defined(__AVR_ATmega328P__)

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
#define _UCSZ0 UCSZ00
#define _UCSZ1 UCSZ01
#define _FE FE0

#elif defined(__AVR_ATmega1284P__)

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
#define _UCSZ0 UCSZ00
#define _UCSZ1 UCSZ01
#define _FE FE0

#endif

/*
  Baud rate selection based on official AVR doc:

    AVR306: Using the AVR USART on tinyAVR and megaAVR devices

*/
void USART_SetBaudRate(uint8_t baudrate) {
  uint16_t br = 0;

#if F_CPU == 16000000L
  if (baudrate == BAUD_RATE_9600) {
    br = 103;
    _UCSRA = 0;
  } else if (baudrate == BAUD_RATE_57600) {
    br = 16;
    _UCSRA = 0;
  } else if (baudrate == BAUD_RATE_115200) {
    br = 16;
    _UCSRA = (1 << _U2X); // use 2X to lower error
  }
#elif F_CPU == 8000000L
  if (baudrate == BAUD_RATE_9600) {
    br = 51;
    _UCSRA = 0;
  } else if (baudrate == BAUD_RATE_57600) {
    br = 16;
    _UCSRA = (1 << _U2X); // use 2X to lower error
  } else if (baudrate == BAUD_RATE_115200) {
    br = 8;
    _UCSRA = (1 << _U2X); // use 2X to lower error
  }
#else
#error "BAUDRATE not calculated for this freq"
#endif

  _UBRRL = (unsigned char)(br & 0xFF);
  _UBRRH = (unsigned char)(br >> 8);
}

volatile void (*USART_REC_CB)(uint8_t, bool) = NULL;

void USART_Init(uint8_t baudrate,
                volatile void (*usart_rec_cb)(uint8_t, bool)) {
  USART_REC_CB = usart_rec_cb;

  USART_SetBaudRate(baudrate);
  // Enable Receiver and Transmitter and Interrupt
  _UCSRB |= (1 << _RXEN) | (1 << _TXEN) | (1 << _RXCIE);

  // Set frame format: 8-bit (UCSZ=011), no parity (UPM=00), 1-stop bit (USBS=0)
#if defined(__AVR_ATmega8535__) || defined(__AVR_ATmega8515__) || defined(__AVR_ATmega16__) || defined(__AVR_ATmega32__)
  // URSEL needed because UCSRC shared with UBRRH (see datasheet)
  _UCSRC = (1 << URSEL) | (1 << _UCSZ0) | (1 << _UCSZ1);

  // add pullup on receive line to avoid noise if serial is not connected
  #if defined(__AVR_ATmega8535__)
  PORTD |= (1 << PD0); // assuming RXD is on PD0
  #endif
#else
  _UCSRC |= (1 << _UCSZ0) | (1 << _UCSZ1);
#endif
}

uint8_t USART_Receive() {
  /* Wait for data to be received */
  while (!(_UCSRA & (1 << _RXC)))
    ;
  /* Get and return received data from buffer */
  return _UDR;
}

volatile uint8_t last_char = 0;

#if defined(__AVR_ATmega1284P__)
ISR(USART0_RX_vect) {
#elif defined(__AVR_ATmega16__) || defined(__AVR_ATmega32__)
ISR(USART_RXC_vect) {
#else
ISR(USART_RX_vect) {
#endif
  bool error = !bit_is_clear(_UCSRA, _FE);
  last_char = _UDR;
  if (USART_REC_CB != NULL) {
    (*USART_REC_CB)(last_char, error);
  }
}

uint8_t USART_GetLastChar() { return last_char; }

void USART_Transmit(unsigned char data) {
  /* Wait for empty transmit buffer */
  while (!(_UCSRA & (1 << _UDRE)))
    ;
  /* Put data into buffer, sends the data */
  _UDR = data;
}

void USART_WriteChar(char d) {
  USART_Transmit(d);
}

void USART_WriteString(const char *str) {
  while (*str)
    USART_Transmit(*str++);
}

void USART_WritePString(const char *str) {
  uint8_t c;
  for (uint8_t i=0; i < strlen_P(str); i++)
  {
      c = pgm_read_byte(&(str[i]));
      USART_Transmit(c);
  }
}

static char numberbuffer[12];

void USART_WriteInt(int32_t i, uint8_t base) {
  ltoa(i, numberbuffer, base);
  if (base == 16) {
    USART_WriteString("0x");
    if (i < 16)
      USART_WriteString("0");
  } else if (base == 2) {
    USART_WriteString("0b");
    if (i < 255) {
      size_t nbzero = 8 - strlen(numberbuffer);
      while (nbzero) {
        nbzero--;
        USART_WriteString("0");
      }
    }
  }
  USART_WriteString(numberbuffer);
}

void USART_WriteUInt(uint32_t i, uint8_t base) {
  ultoa(i, numberbuffer, base);
  if (base == 16) {
    USART_WriteString("0x");
    if (i < 16)
      USART_WriteString("0");
  } else if (base == 2) {
    USART_WriteString("0b");
    if (i < 255) {
      size_t nbzero = 8 - strlen(numberbuffer);
      while (nbzero) {
        nbzero--;
        USART_WriteString("0");
      }
    }
  }
  USART_WriteString(numberbuffer);
}

void USART_WriteFloat(float d, uint8_t width, uint8_t prec) {
  dtostrf(d, width, prec, numberbuffer);
  USART_WriteString(numberbuffer);
}

char SerialCommandMgr::commandBuffer[];
uint8_t SerialCommandMgr::commandBufferPos = 0;
uint8_t SerialCommandMgr::_hasCommand = 0;

#endif
