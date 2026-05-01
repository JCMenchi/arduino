/**
 * @file pwm.cpp
 * @brief Implementation of PWM (Pulse Width Modulation) control for multiple timer channels
 * 
 * Provides PWM configuration and control for various AVR device families.
 * Supports both standard PWM (0-255 duty cycle) and servo PWM (microsecond pulse width).
 * 
 * Device support:
 * - ATmega1284P: Timers 0, 1, 2, 3 with multiple channels
 * - ATmega8515: Timers 0, 1, 2
 * - ATmega8535/16/32: Timers 0, 1, 2
 * - ATmega328P/ATtiny45/84: Limited timer support
 * 
 * @note Timer0A is reserved for millisecond timing
 * @note Some channels conflict with SPI on certain devices
 * @note Frequencies vary by device and prescaler settings
 */

#include <pwm.h>

#include <gpio.h>

#include <avr/interrupt.h>

#ifdef HAS_SERIAL
#include <usart_serial.h>
#endif

/** @brief CPU frequency in Hz (used for timing calculations) */
#if F_CPU == 16000000L
const uint8_t NB_CYCLE_PER_MICRO_SEC = 16;  /**< 16 cycles per microsecond at 16MHz */
#elif F_CPU == 8000000L
const uint8_t NB_CYCLE_PER_MICRO_SEC = 8;   /**< 8 cycles per microsecond at 8MHz */

#else
#error "unknown CPU freq"
#endif


/**
 * @brief ATmega1284P PWM implementation
 * 
 * Supports PWM on timers 0, 1, 2, 3 with various channels.
 * Typically configured with 8MHz internal oscillator.
 * Frequencies:
 * - Timer 0: ~2 kHz (8-bit, prescaler /8)
 * - Timer 1: ~0.5 kHz (10-bit, prescaler /8)  
 * - Timer 2: ~2 kHz (8-bit, prescaler /8)
 * - Timer 3: Similar to Timer 1
 */
#if defined(__AVR_ATmega1284P__) 

/**
 * @brief Enable PWM on specified pin (ATmega1284P)
 * 
 * Configures the appropriate timer and output compare unit for PWM generation
 * at 8-bit (0-255) resolution.
 * 
 * @param pwm_pin PWM channel identifier (PWM_OC0B, PWM_OC1A/B, PWM_OC2A/B, PWM_OC3A/B)
 */
void enablePWM(uint8_t pwm_pin) {

    if (pwm_pin == PWM_OC0B) {
        // use PWM, PWM, Phase Correct, 8-bit
        TCCR0A |= (1<<WGM00);
        TCCR0A &= ~(1<<WGM01);
        TCCR0B &= ~(1<<WGM02);
        
        // set prescaler /8 =>  8MHz / (2 * 8 * 255) => 2kHz 
        TCCR0B |= (1<<CS01);
        TCCR0B &= ~(1<<CS02 | 1<<CS00);
    } else if (pwm_pin == PWM_OC1A || pwm_pin == PWM_OC1B) {
        // use PWM, PWM, Phase Correct, 10-bit
        TCCR1A |= (1<<WGM10 | 1<<WGM11);
        //TCCR1A &= ~(1<<WGM11);
        TCCR1B &= ~(1<<WGM13 | 1<<WGM12);
        
        // set prescaler /8 =>  8MHz / (2 * 8 * 1024) => 0.5kHz 
        TCCR1B |= (1<<CS11);
        TCCR1B &= ~(1<<CS12 | 1<<CS10);
    } else if (pwm_pin == PWM_OC2A || pwm_pin == PWM_OC2B) {
        // activate timer 2
        PRR0 &= ~(1<<PRTIM2);

        // use PWM, PWM, Phase Correct, 10-bit
        TCCR2A |= (1<<WGM20);
        TCCR2A &= ~(1<<WGM21);
        TCCR2B &= ~(1<<WGM22);
        
        // set prescaler /8 =>  8MHz / (8 * 510) => 2kHz 
        TCCR2B |= (1<<CS21);
        TCCR2B &= ~(1<<CS22 | 1<<CS20);
    } else {
        #ifdef HAS_SERIAL
        USART_WriteString("PWM mode not yet implemented\n");
        #endif
    }

    // set corresponding pin as output
    if (pwm_pin == PWM_OC0B) { // PIN B4
        // use non-inverting Compare Output mode
        TCCR0A |= (1<<COM0B1);
        TCCR0A &= ~(1<<COM0B0);

        GPIO_OUTPUT(B, 4);
    } else if (pwm_pin == PWM_OC1A) { // PIN D5
        // use non-inverting Compare Output mode
        TCCR1A |= (1<<COM1A1);
        TCCR1A &= ~(1<<COM1A0);

        GPIO_OUTPUT(D, 5);
    } else if (pwm_pin == PWM_OC1B) { // PIN D4
        // use non-inverting Compare Output mode
        TCCR1A |= (1<<COM1B1);
        TCCR1A &= ~(1<<COM1B0);

        GPIO_OUTPUT(D, 4);
    } else if (pwm_pin == PWM_OC2A) { // PIN D5
        // use non-inverting Compare Output mode
        TCCR2A |= (1<<COM2A1);
        TCCR2A &= ~(1<<COM2A0);

        GPIO_OUTPUT(D, 7);
    } else if (pwm_pin == PWM_OC2B) { // PIN D4
        // use non-inverting Compare Output mode
        TCCR2A |= (1<<COM2B1);
        TCCR2A &= ~(1<<COM2B0);

        GPIO_OUTPUT(D, 6);
    }

}


/**
 * @brief Enable servo-compatible PWM on specified pin (ATmega1284P)
 * 
 * Configures 16-bit timer for servo control with 20ms period.
 * Servo pulse width typically 1-2ms (value range 1000-2000 at 1µs/tick).
 * 
 * @param pwm_pin PWM channel identifier (PWM_OC1A, PWM_OC1B, PWM_OC2A, PWM_OC2B)
 */
void enableServoPWM(uint8_t pwm_pin) {
    uint8_t oldSREG = SREG;
    cli();

    if (pwm_pin == PWM_OC1A || pwm_pin == PWM_OC1B) {
        // use PWM, Fast PWM with ICR1 as top
        TCCR1A |= (1<<WGM11);
        TCCR1A &= ~(1<<WGM10);
        TCCR1B |= (1<<WGM13 | 1<<WGM12);
        
        // set prescaler to 8 => 1MHz => 1 tick = 1 us 
        TCCR1B |= (1<<CS11);
        TCCR1B &= ~(1<<CS12 | 1<<CS10);

        // 20ms period with 1µs per tick (ICR1 = F_CPU/8/1000000*20000 = 20000)
        ICR1 = 20000;

        // disable interrupt
        TIMSK1 &=  ~(_BV(OCIE1A) | _BV(OCIE1B) | _BV(TOIE1) );

    } else if (pwm_pin == PWM_OC2A || pwm_pin == PWM_OC2B) {
        // activate timer 2
        PRR0 &= ~(1<<PRTIM2);

        // use PWM, PWM, Phase Correct, 8-bit
        TCCR2A |= (1<<WGM20);
        TCCR2A |= (1<<WGM21);
        TCCR2B &= ~(1<<WGM22);
        
        // set prescaler /256 =>  8MHz / (256 * 510) => 61 Hz period 16 ms
        TCCR2B &= ~(1<<CS22);
        TCCR2B |= (1<<CS20 | 1<<CS21);
    } else {
        #ifdef HAS_SERIAL
        USART_WriteString("PWM mode not yet implemented\n");
        #endif
    }

    // set corresponding pin as output
    if (pwm_pin == PWM_OC0B) { // PIN B4
        // use non-inverting Compare Output mode
        TCCR0A |= (1<<COM0B1);
        TCCR0A &= ~(1<<COM0B0);

        GPIO_OUTPUT(B, 4);
    } else if (pwm_pin == PWM_OC1A) { // PIN D5
        // use non-inverting Compare Output mode
        TCCR1A |= (1<<COM1A1);
        TCCR1A &= ~(1<<COM1A0);

        OCR1A = 3000;

        GPIO_OUTPUT(D, 5);
    } else if (pwm_pin == PWM_OC1B) { // PIN D4
        // use non-inverting Compare Output mode
        TCCR1A |= (1<<COM1B1);
        TCCR1A &= ~(1<<COM1B0);

        OCR1B = 3000;
        GPIO_OUTPUT(D, 4);
    } else if (pwm_pin == PWM_OC2A) { // PIN D5
        // use non-inverting Compare Output mode
        TCCR2A |= (1<<COM2A1);
        TCCR2A &= ~(1<<COM2A0);

        GPIO_OUTPUT(D, 7);
    } else if (pwm_pin == PWM_OC2B) { // PIN D4
        // use non-inverting Compare Output mode
        TCCR2A |= (1<<COM2B1);
        TCCR2A &= ~(1<<COM2B0);

        GPIO_OUTPUT(D, 6);
    }

    SREG = oldSREG;  // reactivate interrupt
    sei();
}

/**
 * @brief Set PWM duty cycle for standard PWM output
 * 
 * Updates the output compare register to set pulse width.
 * Value range 0-255 represents 0-100% duty cycle.
 * 
 * @param pwm_pin PWM channel identifier
 * @param value Duty cycle (0-255, where 0=always off, 255=always on)
 * 
 * @note PWM must be enabled with enablePWM() first
 */
void setPWM(uint8_t pwm_pin, uint8_t value) {
    if (pwm_pin == PWM_OC0B) {
        OCR0B = value;
    } else if (pwm_pin == PWM_OC1A) {
        OCR1A = value;
    } else if (pwm_pin == PWM_OC1B) {
        OCR1B = value;
    } else if (pwm_pin == PWM_OC2A) {
        OCR2A = value;
    } else if (pwm_pin == PWM_OC2B) {
        OCR2B = value;
    }
}

/**
 * @brief Set servo PWM pulse width
 * 
 * Updates output compare register for servo control.
 * Pulse width in timer ticks (typically microseconds at 1MHz clock).
 * 
 * @param pwm_pin PWM channel identifier
 * @param value Pulse width in timer ticks (typically 1000-2000 for 1-2ms)
 * 
 * @note Servo PWM must be enabled with enableServoPWM() first
 * @note Value is capped at 20000 (20ms maximum period)
 */
void setServoPWM(uint8_t pwm_pin, uint16_t value) {
    if (pwm_pin == PWM_OC1A) {

        // value cannot be more than 20ms
        if (value > 20000) {
            #ifdef HAS_SERIAL
            USART_WriteString("Value ");
            USART_WriteUInt(value);
            USART_WriteString(" too big reset to ");
            #endif
            value = 20000;

            #ifdef HAS_SERIAL
            USART_WriteUInt(value);
            USART_WriteString("\n");
            #endif
        }

        #ifdef HAS_SERIAL
        USART_WriteString("Set OC1A to ");
        USART_WriteUInt(value);
        USART_WriteString("\n");
        #endif
        OCR1A = value;
    } else if (pwm_pin == PWM_OC2A) {
        // value cannot be more than 256
        if (value > 255) {
            #ifdef HAS_SERIAL
            USART_WriteString("Value ");
            USART_WriteUInt(value);
            USART_WriteString(" too big reset to ");
            #endif

            value = 255;

            #ifdef HAS_SERIAL
            USART_WriteUInt(value);
            USART_WriteString("\n");
            #endif
        }

        #ifdef HAS_SERIAL
        USART_WriteString("Set OC2A to ");
        USART_WriteUInt(value);
        USART_WriteString("\n");
        #endif
        
        OCR2A = value;
    }
}

/**
 * @brief ATmega8515 PWM implementation
 * 
 * Supports PWM on timers 0, 1, 2.
 */
#elif defined(__AVR_ATmega8515__) 

/**
 * @brief Enable PWM on specified pin (ATmega8515)
 * @param pwm_pin PWM channel identifier
 */
void enablePWM(uint8_t pwm_pin) {

    if (pwm_pin == PWM_OC0B) {
        // use PWM, PWM, Phase Correct, 8-bit
        TCCR0A |= (1<<WGM00);
        TCCR0A &= ~(1<<WGM01);
        TCCR0B &= ~(1<<WGM02);
        
        // set prescaler /8 =>  8MHz / (2 * 8 * 255) => 2kHz 
        TCCR0B |= (1<<CS01);
        TCCR0B &= ~(1<<CS02 | 1<<CS00);
    } else if (pwm_pin == PWM_OC1A || pwm_pin == PWM_OC1B) {
        // use PWM, PWM, Phase Correct, 10-bit
        TCCR1A |= (1<<WGM10 | 1<<WGM11);
        //TCCR1A &= ~(1<<WGM11);
        TCCR1B &= ~(1<<WGM13 | 1<<WGM12);
        
        // set prescaler /8 =>  8MHz / (2 * 8 * 1024) => 0.5kHz 
        TCCR1B |= (1<<CS11);
        TCCR1B &= ~(1<<CS12 | 1<<CS10);
    } else if (pwm_pin == PWM_OC2A || pwm_pin == PWM_OC2B) {
        // activate timer 2
        PRR0 &= ~(1<<PRTIM2);

        // use PWM, PWM, Phase Correct, 10-bit
        TCCR2A |= (1<<WGM20);
        TCCR2A &= ~(1<<WGM21);
        TCCR2B &= ~(1<<WGM22);
        
        // set prescaler /8 =>  8MHz / (8 * 510) => 2kHz 
        TCCR2B |= (1<<CS21);
        TCCR2B &= ~(1<<CS22 | 1<<CS20);
    } else {
        USART_WriteString("PWM mode not yet implemented\n");
    }

    // set corresponding pin as output
    if (pwm_pin == PWM_OC0B) { // PIN B4
        // use non-inverting Compare Output mode
        TCCR0A |= (1<<COM0B1);
        TCCR0A &= ~(1<<COM0B0);

        GPIO_OUTPUT(B, 4);
    } else if (pwm_pin == PWM_OC1A) { // PIN D5
        // use non-inverting Compare Output mode
        TCCR1A |= (1<<COM1A1);
        TCCR1A &= ~(1<<COM1A0);

        GPIO_OUTPUT(D, 5);
    } else if (pwm_pin == PWM_OC1B) { // PIN D4
        // use non-inverting Compare Output mode
        TCCR1A |= (1<<COM1B1);
        TCCR1A &= ~(1<<COM1B0);

        GPIO_OUTPUT(D, 4);
    } else if (pwm_pin == PWM_OC2A) { // PIN D5
        // use non-inverting Compare Output mode
        TCCR2A |= (1<<COM2A1);
        TCCR2A &= ~(1<<COM2A0);

        GPIO_OUTPUT(D, 7);
    }

}

void setPWM(uint8_t pwm_pin, uint8_t value) {
    if (pwm_pin == PWM_OC0B) {
        OCR0B = value;
    } else if (pwm_pin == PWM_OC1A) {
        OCR1A = value;
    } else if (pwm_pin == PWM_OC1B) {
        OCR1B = value;
    } else if (pwm_pin == PWM_OC2A) {
        OCR2A = value;
    }
}

#elif defined(__AVR_ATmega8535__)

void enablePWM(uint8_t pwm_pin) {

    if (pwm_pin == PWM_OC1A || pwm_pin == PWM_OC1B) {
        // use PWM, PWM, Phase Correct, 10-bit
        TCCR1A |= (1<<WGM10 | 1<<WGM11);
        //TCCR1A &= ~(1<<WGM11);
        TCCR1B &= ~(1<<WGM13 | 1<<WGM12);
        
        // set prescaler /8 =>  8MHz / (2 * 8 * 1024) => 0.5kHz 
        TCCR1B |= (1<<CS11);
        TCCR1B &= ~(1<<CS12 | 1<<CS10);
    } else if (pwm_pin == PWM_OC2) {
        // use PWM, PWM, Phase Correct, 10-bit
        TCCR2 |= (1<<WGM20);
        TCCR2 &= ~(1<<WGM21);

        // set prescaler /8 =>  8MHz / (8 * 510) => 2kHz 
        TCCR2 |= (1<<CS21);
        TCCR2 &= ~(1<<CS22 | 1<<CS20);
    } else {
        #ifdef HAS_SERIAL
        USART_WriteString("PWM mode not yet implemented\n");
        #endif
    }

    // set corresponding pin as output
    if (pwm_pin == PWM_OC1A) { // PIN D5
        // use non-inverting Compare Output mode
        TCCR1A |= (1<<COM1A1);
        TCCR1A &= ~(1<<COM1A0);

        GPIO_OUTPUT(D, 5);
    } else if (pwm_pin == PWM_OC1B) { // PIN D4
        // use non-inverting Compare Output mode
        TCCR1A |= (1<<COM1B1);
        TCCR1A &= ~(1<<COM1B0);

        GPIO_OUTPUT(D, 4);
    } else if (pwm_pin == PWM_OC2) { // PIN D5
        // use non-inverting Compare Output mode
        TCCR2 |= (1<<COM21);
        TCCR2 &= ~(1<<COM20);

        GPIO_OUTPUT(D, 7);
    }

}

void setPWM(uint8_t pwm_pin, uint8_t value) {
    if (pwm_pin == PWM_OC1A) {
        OCR1A = value;
    } else if (pwm_pin == PWM_OC1B) {
        OCR1B = value;
    } else if (pwm_pin == PWM_OC2) {
        OCR2 = value;
    }
}

#else
void enablePWM(uint8_t pwm_pin) {
}

void setPWM(uint8_t pwm_pin, uint8_t value) {

}
#endif
