
/**
 * @file int0_serial.cpp
 * @brief Implementation of INT0-based bit-bang serial communication
 * 
 * Implements software serial communication using the INT0 external interrupt.
 * Provides both transmit (bit-banging on configurable TX pin) and receive
 * (interrupt-driven on INT0 pin) functionality.
 * 
 * Device-specific configurations:
 * - ATtiny45: INT0 on B2, TX port A
 * - ATtiny84: INT0 on B2, TX port B (configurable)
 * - ATmega328P/1284P: INT0 on D2
 * - ATmega8535/8515/16/32: INT0 on D2
 * 
 * @note Fixed baud rate of 9600 bps
 * @note Receive uses interrupt-driven bit-sampling
 * @note Transmit uses software timing with _delay_loop_2()
 */

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

#elif defined(__AVR_ATmega8535__) || defined(__AVR_ATmega8515__) || defined(__AVR_ATmega16__) || defined(__AVR_ATmega32__)

#define INT0_PORT D
#define INT0_PIN PIND2
#define COM_LED_PORTID A
#define COM_LED_PIN 6

#elif defined(__AVR_ATmega328P__)

#define INT0_PORT D
#define INT0_PIN PIND2

#elif defined(__AVR_ATtiny84__)

#define INT0_PORT B
#define INT0_PIN PINB2
//#define COM_LED_PORTID A
//#define COM_LED_PIN 0

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

/**
 * @brief Subtract with lower bound (never returns 0)
 * @param num Minuend
 * @param sub Subtrahend
 * @return num - sub if positive, 1 otherwise
 */
uint16_t subtract_cap(uint16_t num, uint16_t sub) {
    if (num > sub)
        return num - sub;
    else
        return 1;
}

/** @brief GPIO pin number for serial transmit */
uint8_t _transmitPin;

/** @brief Receive centering delay in 4-cycle units (never 0) */
uint16_t _rx_delay_centering;
/** @brief Receive intra-bit delay in 4-cycle units (never 0) */
uint16_t _rx_delay_intrabit;
/** @brief Receive stop bit delay in 4-cycle units (never 0) */
uint16_t _rx_delay_stopbit;
/** @brief Transmit bit delay in 4-cycle units (never 0) */
uint16_t _tx_delay;

/**
 * @brief Calculate bit timing delays for 9600 baud
 * 
 * Pre-calculates the various timing delays needed for correct bit-bang
 * serial communication at 9600 bps. Timings are specified in 4-cycle
 * delay units for use with _delay_loop_2().
 * 
 * @note All delays are calculated based on F_CPU frequency
 * @note Internal function, called by INT0_Init()
 */
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

/** @brief Callback function pointer for received bytes */
volatile void (*INT0_REC_CB)(uint8_t) = NULL;

/**
 * @brief Initialize INT0 serial communication
 * 
 * Sets up the INT0 external interrupt for bit-bang serial reception and
 * configures the transmit pin for software serial transmission. Calculates
 * timing delays based on F_CPU and enables the INT0 interrupt on falling edge.
 * 
 * @param tpin GPIO pin number to use for transmission (e.g., PA0)
 * @param INT0_rec_cb Callback function invoked for each received byte
 * 
 * @note Must be called before transmitting or receiving data
 * @note Callback function is called from ISR context
 * @see INT0_Transmit() for transmission
 */
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
    GPIO_SET_HIGH(INT0_SERIAL_TRANSMIT_PORT, _transmitPin);
    GPIO_OUTPUT(INT0_SERIAL_TRANSMIT_PORT, _transmitPin);

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
    #elif defined(__AVR_ATtiny84__)
    GIMSK |= (1 << INT0);
    // interrupt on failing edge
    MCUCR = (1 << ISC01);
    #else
    GICR |= (1 << INT0);
    // interrupt on failing edge
    MCUCR = (1 << ISC01);
    #endif
    
    
    _delay_loop_2(_tx_delay);  // if we were low this establishes the end
}

/**
 * @brief INT0 external interrupt handler for serial receive
 * 
 * Implements bit-bang serial reception on INT0 pin. When a start bit
 * (falling edge) is detected, reads 8 data bits followed by stop bit.
 * Invokes the registered callback with each received byte.
 * 
 * @note Disables INT0 during reception to prevent false triggers
 * @note Automatically re-enables INT0 after stop bit is detected
 */
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
        #elif defined(__AVR_ATtiny84__)
        GIMSK &= ~(1 << INT0);
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
        #elif defined(__AVR_ATtiny84__)
        GIMSK |= (1 << INT0);
        #else
        GICR |= (1 << INT0);
        #endif

        #ifdef COM_LED_PORTID
        GPIO_SET_LOW(COM_LED_PORTID, COM_LED_PIN);
        #endif
    }
}

/**
 * @brief Transmit a byte via INT0 serial (bit-bang method)
 * 
 * Transmits 8 data bits with start and stop bits at 9600 baud.
 * Uses software timing (_delay_loop_2) to generate accurate bit periods.
 * 
 * @param data Byte to transmit
 * @return 1 if successful, 0 if not initialized (_tx_delay == 0)
 * 
 * @note Disables interrupts during transmission for accurate timing
 * @note Local variables used to ensure compiler places them in registers
 *       before interrupt disable, improving cycle timing accuracy
 */
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
    GPIO_SET_LOW(INT0_SERIAL_TRANSMIT_PORT, _transmitPin);

    _delay_loop_2(delay);

    // Write each of the 8 bits
    for (uint8_t i = 8; i > 0; --i) {
        if (data & 1)  // choose bit
            //*reg |= reg_mask;  // send 1
            GPIO_SET_HIGH(INT0_SERIAL_TRANSMIT_PORT, _transmitPin);
        else
            //*reg &= inv_mask;  // send 0
            GPIO_SET_LOW(INT0_SERIAL_TRANSMIT_PORT, _transmitPin);

        _delay_loop_2(delay);
        data >>= 1;
    }

    // restore pin to natural state
    // *reg |= reg_mask;
    GPIO_SET_HIGH(INT0_SERIAL_TRANSMIT_PORT, _transmitPin);

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

/**
 * @brief Transmit a single character
 * 
 * Transmits a single character (byte) via INT0 serial.
 * 
 * @param d Character to transmit
 * 
 * @see INT0_WriteString() for transmitting null-terminated strings
 */
void INT0_WriteChar(char d) {
    INT0_Transmit(d);
}

/**
 * @brief Transmit a null-terminated string
 * 
 * Sends each character of the string via INT0 serial. The string must be
 * stored in RAM (use INT0_WritePString for PROGMEM strings).
 * 
 * @param str Pointer to null-terminated string in RAM
 * 
 * @see INT0_WritePString() for PROGMEM strings
 */
void INT0_WriteString(const char *str) {
    while (*str)
        INT0_Transmit(*str++);
}

/**
 * @brief Transmit a null-terminated string from program memory (PROGMEM)
 * 
 * Sends each character of a PROGMEM-stored string via INT0 serial.
 * Use this for constant strings defined with PROGMEM to save RAM.
 * 
 * Example:
 *   const char myString[] PROGMEM = "Hello";
 *   INT0_WritePString(myString);
 * 
 * @param str Pointer to null-terminated string in program memory (PROGMEM)
 * 
 * @see INT0_WriteString() for RAM strings
 */
void INT0_WritePString(const char *str) {
    uint8_t c;
    for (uint8_t i = 0; i < strlen_P(str); i++) {
        c = pgm_read_byte(&(str[i]));
        INT0_Transmit(c);
    }
}

/** @brief Buffer for number-to-string conversion (supports up to 32-bit integers) */
static char numberbuffer[12];

/**
 * @brief Transmit a signed 32-bit integer with optional base prefix
 * 
 * Converts a signed integer to string and transmits via INT0 serial.
 * Automatically adds prefixes for non-decimal bases:
 * - 0x for hexadecimal (base 16)
 * - 0b for binary (base 2)
 * - No prefix for decimal (base 10)
 * 
 * @param i Signed integer to transmit
 * @param base Number base for conversion (2, 10, 16, etc.)
 * 
 * @see INT0_WriteUInt() for unsigned integers
 * @see INT0_WriteFloat() for floating-point numbers
 */
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

/**
 * @brief Transmit an unsigned 32-bit integer with optional base prefix
 * 
 * Converts an unsigned integer to string and transmits via INT0 serial.
 * Automatically adds prefixes for non-decimal bases:
 * - 0x for hexadecimal (base 16)
 * - 0b for binary (base 2, zero-padded to 8 bits if < 256)
 * - No prefix for decimal (base 10)
 * 
 * @param i Unsigned integer to transmit
 * @param base Number base for conversion (2, 10, 16, etc.)
 * 
 * @see INT0_WriteInt() for signed integers
 * @see INT0_WriteFloat() for floating-point numbers
 */
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

/**
 * @brief Transmit a floating-point number with specified width and precision
 * 
 * Converts a float to string and transmits via INT0 serial. Formatting
 * follows standard printf conventions with dtostrf().
 * 
 * @param d Floating-point number to transmit
 * @param width Total field width (including decimal point and sign)
 * @param prec Number of decimal places (precision)
 * 
 * @see INT0_WriteInt() for integer transmission
 * @see INT0_WriteUInt() for unsigned integer transmission
 */
void INT0_WriteFloat(float d, uint8_t width, uint8_t prec) {
    dtostrf(d, width, prec, numberbuffer);
    INT0_WriteString(numberbuffer);
}

/** @brief Statically allocated buffer for received command strings */
char INT0_SerialCommandMgr::commandBuffer[];

/** @brief Current write position in the command buffer */
uint8_t INT0_SerialCommandMgr::commandBufferPos = 0;

/** @brief Flag indicating a complete command has been received (non-zero if command ready) */
uint8_t INT0_SerialCommandMgr::_hasCommand = 0;
