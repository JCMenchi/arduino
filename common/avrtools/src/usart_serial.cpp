
/**
 * @file usart_serial.cpp
 * @brief Implementation of USART serial communication driver
 * 
 * Provides hardware USART functionality for serial communication
 * at configurable baud rates (9600, 57600, 115200 bps).
 * 
 * Device support:
 * - ATmega328P/1284P: Full USART support with 16-bit baud rate register
 * - ATmega8535/8515/16/32: Full USART support with older register names
 * - ATmega8: Legacy 8-bit device support
 * - ATtiny45/84: No USART (stubs provided)
 * 
 * Features:
 * - Interrupt-driven receive with callback
 * - Error detection (framing errors reported via callback)
 * - Formatted output functions for strings, integers, floats
 * - Built-in command buffering and parsing via SerialCommandMgr class
 * 
 * @note CPU frequencies: 8MHz or 16MHz
 */

#include "usart_serial.h"

#include <avr/interrupt.h>
#include <avr/io.h>
#include <stdlib.h>
#include <string.h>
#include <avr/pgmspace.h>

/**
 * @brief Stub implementations for ATtiny devices (no USART available)
 * 
 * ATtiny microcontrollers do not have USART hardware.
 * Users should use INT0_serial instead.
 */
#if defined(__AVR_ATtiny45__) || defined(__AVR_ATtiny84__)

/** @brief No-op USART_Init for tinyAVR devices */
void USART_Init(uint8_t baudrate,
                volatile void (*usart_rec_cb)(uint8_t, bool)) {}

/** @brief No-op USART_Transmit for tinyAVR devices */
void USART_Transmit(uint8_t data) {}
/** @brief No-op USART_Receive for tinyAVR devices */
uint8_t USART_Receive() { return 0; }

/** @brief No-op USART_WriteString for tinyAVR devices */
void USART_WriteString(const char *str) {}

/** @brief No-op USART_WritePString for tinyAVR devices */
void USART_WritePString(const char *str) {}

/** @brief No-op USART_WriteInt for tinyAVR devices */
void USART_WriteInt(int32_t i, uint8_t base) {}
/** @brief No-op USART_WriteUInt for tinyAVR devices */
void USART_WriteUInt(uint32_t i, uint8_t base) {}

/** @brief No-op USART_WriteBool for tinyAVR devices */
void USART_WriteBool(bool b) {}

/** @brief No-op USART_GetLastChar for tinyAVR devices */
uint8_t USART_GetLastChar() { return 32; }

/**
 * @brief Full USART implementation for devices with hardware USART
 */
#else

/**
 * @brief ATmega8 register mapping (legacy compatibility)
 * 
 * Older ATmega8 used different register names.
 * Macro definitions allow same code to work across device families.
 */
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

/**
 * @brief Set USART baud rate based on F_CPU frequency
 * 
 * Calculates and sets the UBRR (USART Baud Rate Register) value.
 * Optionally enables 2X mode for baud rates with high error at normal speed.
 * 
 * Baud rate values calculated per AVR306 application note.
 * Supports:
 * - 16MHz: 9600/57600/115200 bps
 * - 8MHz: 9600/57600/115200 bps (some with 2X mode)
 * 
 * @param baudrate Baud rate selection (BAUD_RATE_9600, BAUD_RATE_57600, BAUD_RATE_115200)
 */
void USART_SetBaudRate(uint8_t baudrate) {
  uint16_t br = 0;

  /** @brief 16MHz clock frequency */
#if F_CPU == 16000000L
  if (baudrate == BAUD_RATE_9600) {
    br = 103;             // UBRR for 9600 at 16MHz
    _UCSRA = 0;           // Normal 2X mode disabled
  } else if (baudrate == BAUD_RATE_57600) {
    br = 16;              // UBRR for 57600 at 16MHz
    _UCSRA = 0;
  } else if (baudrate == BAUD_RATE_115200) {
    br = 16;              // UBRR for 115200 with 2X mode
    _UCSRA = (1 << _U2X); // Enable 2X mode to reduce error
  }
  /** @brief 8MHz clock frequency */
#elif F_CPU == 8000000L
  if (baudrate == BAUD_RATE_9600) {
    br = 51;              // UBRR for 9600 at 8MHz
    _UCSRA = 0;
  } else if (baudrate == BAUD_RATE_57600) {
    br = 16;              // UBRR for 57600 with 2X mode
    _UCSRA = (1 << _U2X); // Enable 2X mode
  } else if (baudrate == BAUD_RATE_115200) {
    br = 8;               // UBRR for 115200 with 2X mode
    _UCSRA = (1 << _U2X); // Enable 2X mode
  }
#else
#error "BAUDRATE not calculated for this freq"
#endif

  // Set UBRR register (split into high and low bytes)
  _UBRRL = (unsigned char)(br & 0xFF);
  _UBRRH = (unsigned char)(br >> 8);
}

/** @brief Callback function pointer for receive interrupt */
volatile void (*USART_REC_CB)(uint8_t, bool) = NULL;

void USART_Init(uint8_t baudrate,
                volatile void (*usart_rec_cb)(uint8_t, bool)) {
  USART_REC_CB = usart_rec_cb;

  USART_SetBaudRate(baudrate);
  // Enable Receiver, Transmitter, and Receive Interrupt
  _UCSRB |= (1 << _RXEN) | (1 << _TXEN) | (1 << _RXCIE);

  // Set frame format: 8-bit data (UCSZ=011), no parity, 1 stop bit
#if defined(__AVR_ATmega8535__) || defined(__AVR_ATmega8515__) || defined(__AVR_ATmega16__) || defined(__AVR_ATmega32__)
  // URSEL bit must be 1 because UCSRC shares address with UBRRH (see datasheet)
  _UCSRC = (1 << URSEL) | (1 << _UCSZ0) | (1 << _UCSZ1);

  // Add pull-up on receive line to avoid noise if not connected
  #if defined(__AVR_ATmega8535__)
  PORTD |= (1 << PD0); // RXD on PD0
  #endif
#else
  // On newer devices, UCSRC is separate from UBRRH
  _UCSRC |= (1 << _UCSZ0) | (1 << _UCSZ1);
#endif
}

/**
 * @brief Blocking receive of a single byte from USART
 * 
 * Waits (busy-loop) until a byte is available in the receive buffer,
 * then returns it.
 * 
 * @return Received byte
 * 
 * @note This is a blocking/synchronous function. For non-blocking
 *       reception, use the interrupt callback mechanism instead.
 */
uint8_t USART_Receive() {
  // Wait for data to be received (busy-loop)
  while (!(_UCSRA & (1 << _RXC)))
    ;
  // Get and return received data from buffer
  return _UDR;
}

/** @brief Storage for most recently received character (via interrupt) */
volatile uint8_t last_char = 0;

/**
 * @brief USART receive interrupt handler (device-specific vector names)
 * 
 * Reads received byte from UDR and invokes callback with error status.
 * Error flag indicates framing errors (stop bit not detected).
 * 
 * @note Vector name varies by device (USART0_RX_vect, USART_RXC_vect, USART_RX_vect)
 */
#if defined(__AVR_ATmega1284P__)
/** @brief ATmega1284P USART0 receive interrupt */
ISR(USART0_RX_vect) {
#elif defined(__AVR_ATmega16__) || defined(__AVR_ATmega32__)
/** @brief ATmega16/32 USART receive interrupt */
ISR(USART_RXC_vect) {
#else
/** @brief Default USART receive interrupt */
ISR(USART_RX_vect) {
#endif
  // Check for framing error (FE bit in UCSRA)
  bool error = !bit_is_clear(_UCSRA, _FE);
  // Read received byte
  last_char = _UDR;
  // Invoke callback if registered
  if (USART_REC_CB != NULL) {
    (*USART_REC_CB)(last_char, error);
  }
}

/**
 * @brief Get the most recently received character
 * 
 * Returns the last character received via interrupt.
 * Useful for synchronous polling of received data.
 * 
 * @return Last received byte
 */
uint8_t USART_GetLastChar() { return last_char; }

/**
 * @brief Transmit a single byte via USART (blocking)
 * 
 * Waits until transmit buffer is empty, then writes byte to UDR.
 * 
 * @param data Byte to transmit
 */
void USART_Transmit(unsigned char data) {
  // Wait for empty transmit buffer (UDRE flag)
  while (!(_UCSRA & (1 << _UDRE)))
    ;
  // Put data into buffer, which sends it
  _UDR = data;
}

/**
 * @brief Write a single character via USART
 * @param d Character to transmit
 */
void USART_WriteChar(char d) {
  USART_Transmit(d);
}

/**
 * @brief Write a null-terminated string via USART
 * @param str Pointer to string in RAM
 */
void USART_WriteString(const char *str) {
  while (*str)
    USART_Transmit(*str++);
}

/**
 * @brief Write a boolean value as "on" or "off"
 * @param b Boolean value (true = "on", false = "off")
 */
void USART_WriteBool(bool b) {
  b ? USART_WriteString("on") : USART_WriteString("off");
}

/**
 * @brief Write a null-terminated string stored in program memory
 * @param str Pointer to string in PROGMEM
 */
void USART_WritePString(const char *str) {
  uint8_t c;
  // Read each byte from program memory
  for (uint8_t i=0; i < strlen_P(str); i++)
  {
      c = pgm_read_byte(&(str[i]));
      USART_Transmit(c);
  }
}

/** @brief Buffer for number-to-string conversion */
static char numberbuffer[12];

/**
 * @brief Write a signed 32-bit integer via USART
 * 
 * Converts integer to string in specified base and outputs it.
 * Hex numbers prefixed with "0x", binary with "0b".
 * 
 * @param i Integer to write
 * @param base Number base (2, 8, 10, 16; default: 10)
 */
void USART_WriteInt(int32_t i, uint8_t base) {
  ltoa(i, numberbuffer, base);
  if (base == 16) {
    USART_WriteString("0x");
    if (i < 16)
      USART_WriteString("0");
  } else if (base == 2) {
    USART_WriteString("0b");
    if (i < 255) {
      // Pad binary numbers to 8 bits
      size_t nbzero = 8 - strlen(numberbuffer);
      while (nbzero) {
        nbzero--;
        USART_WriteString("0");
      }
    }
  }
  USART_WriteString(numberbuffer);
}

/**
 * @brief Write an unsigned 32-bit integer via USART
 * 
 * Converts unsigned integer to string in specified base and outputs it.
 * Hex numbers prefixed with "0x", binary with "0b".
 * 
 * @param i Unsigned integer to write
 * @param base Number base (2, 8, 10, 16; default: 10)
 */
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
