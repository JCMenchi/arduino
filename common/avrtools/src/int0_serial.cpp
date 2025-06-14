
#include "int0_serial.h"

#include <avr/interrupt.h>
#include <avr/io.h>
#include <avr/pgmspace.h>
#include <gpio.h>
#include <stdlib.h>
#include <string.h>
#include <util/delay_basic.h>



#if defined(__AVR_ATtiny45__)

#define INT0_PORT B
#define INT0_PIN PINB2
#define COM_LED_PORTID A
#define COM_LED_PIN 6

#else
#if defined(__AVR_ATmega8__)

#elif defined(__AVR_ATmega8535__) || defined(__AVR_ATmega8515__) || defined(__AVR_ATmega16__) || defined(__AVR_ATmega32__)

#define INT0_PORT D
#define INT0_PIN PIND2
#define COM_LED_PORTID A
#define COM_LED_PIN 6

#elif defined(__AVR_ATmega328P__)

#define INT0_PORT D
#define INT0_PIN PIND2

#elif defined(__AVR_ATmega1284P__)

#define INT0_PORT D
#define INT0_PIN PIND2
#define COM_LED_PORTID A
#define COM_LED_PIN 6

#endif

/*
  Baud rate selection based on official AVR doc:

    AVR306: Using the AVR USART on tinyAVR and megaAVR devices

    only 9600
*/

uint16_t subtract_cap(uint16_t num, uint16_t sub) {
    if (num > sub)
        return num - sub;
    else
        return 1;
}

uint8_t _transmitPin;
// Expressed as 4-cycle delays (must never be 0!)
uint16_t _rx_delay_centering;
uint16_t _rx_delay_intrabit;
uint16_t _rx_delay_stopbit;
uint16_t _tx_delay;

void INT0_SetBaudRate() {

    _rx_delay_centering = _rx_delay_intrabit = _rx_delay_stopbit = _tx_delay = 0;

    // Precalculate the various delays, in number of 4-cycle delays
    uint16_t bit_delay = (F_CPU / 9600) / 4;

    // 12 (gcc 4.8.2) or 13 (gcc 4.3.2) cycles from start bit to first bit,
    // 15 (gcc 4.8.2) or 16 (gcc 4.3.2) cycles between bits,
    // 12 (gcc 4.8.2) or 14 (gcc 4.3.2) cycles from last bit to stop bit
    // These are all close enough to just use 15 cycles, since the inter-bit
    // timings are the most critical (deviations stack 8 times)
    _tx_delay = subtract_cap(bit_delay, 15 / 4);

    // Timings counted from gcc 4.8.2 output. This works up to 115200 on
    // 16Mhz and 57600 on 8Mhz.
    //
    // When the start bit occurs, there are 3 or 4 cycles before the
    // interrupt flag is set, 4 cycles before the PC is set to the right
    // interrupt vector address and the old PC is pushed on the stack,
    // and then 75 cycles of instructions (including the RJMP in the
    // ISR vector table) until the first delay. After the delay, there
    // are 17 more cycles until the pin value is read (excluding the
    // delay in the loop).
    // We want to have a total delay of 1.5 bit time. Inside the loop,
    // we already wait for 1 bit time - 23 cycles, so here we wait for
    // 0.5 bit time - (71 + 18 - 22) cycles.
    _rx_delay_centering = subtract_cap(bit_delay / 2, (4 + 4 + 75 + 17 - 23) / 4);

    // There are 23 cycles in each loop iteration (excluding the delay)
    _rx_delay_intrabit = subtract_cap(bit_delay, 23 / 4);

    // There are 37 cycles from the last bit read to the start of
    // stopbit delay and 11 cycles from the delay until the interrupt
    // mask is enabled again (which _must_ happen during the stopbit).
    // This delay aims at 3/4 of a bit time, meaning the end of the
    // delay will be at 1/4th of the stopbit. This allows some extra
    // time for ISR cleanup, which makes 115200 baud at 16Mhz work more
    // reliably
    _rx_delay_stopbit = subtract_cap(bit_delay * 3 / 4, (37 + 11) / 4);
}

volatile void (*INT0_REC_CB)(uint8_t) = NULL;

void INT0_Init(uint8_t tpin, volatile void (*INT0_rec_cb)(uint8_t)) {
    INT0_REC_CB = INT0_rec_cb;

    INT0_SetBaudRate();

    // set IO pin
    _transmitPin = tpin;
    // Transmit
    //  First write, then set output. If we do this the other way around,
    //  the pin would be output low for a short while before switching to
    //  output high. Now, it is input with pullup for a short while, which
    //  is fine. With inverse logic, either order is fine.
    GPIO_SET_HIGH(INT0_PORT, _transmitPin);
    GPIO_OUTPUT(INT0_PORT, _transmitPin);

    // setRX(receivePin);
    GPIO_INPUT_PULLUP(INT0_PORT, INT0_PIN);

#ifdef COM_LED_PORTID
    GPIO_SET_LOW(COM_LED_PORTID, COM_LED_PIN);
    GPIO_OUTPUT(COM_LED_PORTID, COM_LED_PIN);
#endif

    // Enable the PCINT for the entire port here, but never disable it
    // (others might also need it, so we disable the interrupt by using
    // the per-pin PCMSK register).

    // Global Enable INT0 interrupt
    #if defined(__AVR_ATmega1284P__) || defined(__AVR_ATmega328P__)
    EIMSK |= (1 << INT0);
    // interrupt on failing edge
    EIMSK = (1 << ISC01);
    #else
    GICR |= (1 << INT0);
    // interrupt on failing edge
    MCUCR = (1 << ISC01);
    #endif
    
    
    _delay_loop_2(_tx_delay);  // if we were low this establishes the end
}

ISR(INT0_vect) {
    uint8_t d = 0;

    // If RX line is high, then we don't see any start bit
    // so interrupt is probably not for us
    uint8_t readBit = GPIO_READ(INT0_PORT, INT0_PIN);
    if (!readBit) {
        #ifdef COM_LED_PORTID
        GPIO_SET_HIGH(COM_LED_PORTID, COM_LED_PIN);
        #endif

        // Disable further interrupts during reception, this prevents
        // triggering another interrupt directly after we return, which can
        // cause problems at higher baudrates.
        #if defined(__AVR_ATmega1284P__) || defined(__AVR_ATmega328P__)
        EIMSK &= ~(1 << INT0);
        #else
        GICR &= ~(1 << INT0);
        #endif


        // Wait approximately 1/2 of a bit width to "center" the sample
        _delay_loop_2(_rx_delay_centering);

        // Read each of the 8 bits
        for (uint8_t i = 8; i > 0; --i) {
            _delay_loop_2(_rx_delay_intrabit);
            d >>= 1;
            readBit = GPIO_READ(INT0_PORT, INT0_PIN);
            if (readBit)
                d |= 0x80;
        }

        if (INT0_REC_CB != NULL) {
            (*INT0_REC_CB)(d);
        }

        // skip the stop bit
        _delay_loop_2(_rx_delay_stopbit);

        // Re-enable interrupts when we're sure to be inside the stop bit
        #if defined(__AVR_ATmega1284P__) || defined(__AVR_ATmega328P__)
        EIMSK |= (1 << INT0);
        #else
        GICR |= (1 << INT0);
        #endif

        #ifdef COM_LED_PORTID
        GPIO_SET_LOW(COM_LED_PORTID, COM_LED_PIN);
        #endif
    }
}

uint8_t INT0_Transmit(uint8_t data) {
    if (_tx_delay == 0) {
        return 0;
    }

    #ifdef COM_LED_PORTID
    GPIO_SET_HIGH(COM_LED_PORTID, COM_LED_PIN);
    #endif

    // By declaring these as local variables, the compiler will put them
    // in registers _before_ disabling interrupts and entering the
    // critical timing sections below, which makes it a lot easier to
    // verify the cycle timings
    uint8_t oldSREG = SREG;

    uint16_t delay = _tx_delay;

    cli();  // turn off interrupts for a clean txmit

    // Write the start bit
    //*reg &= inv_mask;
    GPIO_SET_LOW(INT0_PORT, _transmitPin);

    _delay_loop_2(delay);

    // Write each of the 8 bits
    for (uint8_t i = 8; i > 0; --i) {
        if (data & 1)  // choose bit
            //*reg |= reg_mask;  // send 1
            GPIO_SET_HIGH(INT0_PORT, _transmitPin);
        else
            //*reg &= inv_mask;  // send 0
            GPIO_SET_LOW(INT0_PORT, _transmitPin);

        _delay_loop_2(delay);
        data >>= 1;
    }

    // restore pin to natural state
    // *reg |= reg_mask;
    GPIO_SET_HIGH(INT0_PORT, _transmitPin);

    SREG = oldSREG;  // turn interrupts back on
    #ifdef COM_LED_PORTID
    GPIO_SET_LOW(COM_LED_PORTID, COM_LED_PIN);
    #endif
    _delay_loop_2(_tx_delay);

    return 1;
}

// --------------------------------------------------------------------------------------------
//  code independant of hardware
// --------------------------------------------------------------------------------------------
void INT0_WriteChar(char d) {
    INT0_Transmit(d);
}

void INT0_WriteString(const char *str) {
    while (*str)
        INT0_Transmit(*str++);
}

void INT0_WritePString(const char *str) {
    uint8_t c;
    for (uint8_t i = 0; i < strlen_P(str); i++) {
        c = pgm_read_byte(&(str[i]));
        INT0_Transmit(c);
    }
}

static char numberbuffer[12];

void INT0_WriteInt(int32_t i, uint8_t base) {
    ltoa(i, numberbuffer, base);
    if (base == 16) {
        INT0_WriteString("0x");
        if (i < 16)
            INT0_WriteString("0");
    } else if (base == 2) {
        INT0_WriteString("0b");
        if (i < 255) {
            size_t nbzero = 8 - strlen(numberbuffer);
            while (nbzero) {
                nbzero--;
                INT0_WriteString("0");
            }
        }
    }
    INT0_WriteString(numberbuffer);
}

void INT0_WriteUInt(uint32_t i, uint8_t base) {
    ultoa(i, numberbuffer, base);
    if (base == 16) {
        INT0_WriteString("0x");
        if (i < 16)
            INT0_WriteString("0");
    } else if (base == 2) {
        INT0_WriteString("0b");
        if (i < 255) {
            size_t nbzero = 8 - strlen(numberbuffer);
            while (nbzero) {
                nbzero--;
                INT0_WriteString("0");
            }
        }
    }
    INT0_WriteString(numberbuffer);
}

void INT0_WriteFloat(float d, uint8_t width, uint8_t prec) {
    dtostrf(d, width, prec, numberbuffer);
    INT0_WriteString(numberbuffer);
}

char INT0_SerialCommandMgr::commandBuffer[];
uint8_t INT0_SerialCommandMgr::commandBufferPos = 0;
uint8_t INT0_SerialCommandMgr::_hasCommand = 0;

#endif
