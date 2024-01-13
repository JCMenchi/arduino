#ifndef _ARDUIMINI_H
#define _ARDUIMINI_H

#ifdef USE_MINICORE

#include <stdint.h>
#include <avr/interrupt.h>
#include <avr/pgmspace.h>
#include "wiring_lit.h"

typedef uint8_t boolean;
typedef uint8_t byte;

#define HIGH 0x1
#define LOW  0x0

#define INPUT 0x0
#define OUTPUT 0x1
#define INPUT_PULLUP 0x2

#define NOT_A_PIN 0
#define NOT_A_PORT 0
#define NOT_ON_TIMER 0
#define TIMER0A 1
#define TIMER0B 2
#define TIMER1A 3
#define TIMER1B 4
#define TIMER1C 5
#define TIMER2  6
#define TIMER2A 7
#define TIMER2B 8

#define PA 1
#define PB 2
#define PC 3
#define PD 4
#define PE 5
#define PF 6
#define PG 7
#define PH 8
#define PJ 10
#define PK 11
#define PL 12

#define SDA 18
#define SCL 19

#ifndef cbi
#define cbi(sfr, bit) (_SFR_BYTE(sfr) &= ~_BV(bit))
#endif
#ifndef sbi
#define sbi(sfr, bit) (_SFR_BYTE(sfr) |= _BV(bit))
#endif

#define digitalWrite(pin, val)  \
    if (val == LOW) {           \
        D_WRITE_LOW(pin);       \
    } else {                    \
        D_WRITE_HIGH(pin);      \
	}

unsigned long millis();

unsigned long micros();

void delay(unsigned long ms);

/* Delay for the given number of microseconds.  Assumes a 1, 8, 12, 16, 20 or 24 MHz clock. */
void delayMicroseconds(unsigned int us);

class HardwareSerial
{
  public:
    HardwareSerial() {}
    void begin(unsigned long baud);
    
    int available(void);

    int read(void);
   
	void print(const char* t);
	void println(const char* t);
	void print(unsigned long i);
	void println(unsigned long l);
	void print(uint8_t i, uint8_t base);
};

extern HardwareSerial Serial;

#endif

#endif
