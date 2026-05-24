/**
 * @file pwm.h
 * @brief Pulse Width Modulation (PWM) support for various AVR microcontrollers
 * 
 * Provides PWM configuration and control for multiple timer channels across
 * different AVR device families. Supports both standard PWM and servo PWM modes.
 * 
 * PWM channels are identified by constants specific to each microcontroller:
 * - OC0A, OC0B: 8-bit Timer0 compare outputs
 * - OC1A, OC1B: 16-bit Timer1 compare outputs
 * - OC2A, OC2B: 8-bit Timer2 compare outputs
 * - OC3A, OC3B: 16-bit Timer3 compare outputs (ATmega1284P only)
 * 
 * @note Not all channels are available on all devices
 * @note Some channels may conflict with SPI or other functionality
 * @note Timer0A is reserved for millisecond timing and should not be used
 */

#ifndef _AVRTOOLS_PWM_H
#define _AVRTOOLS_PWM_H

#include <avr/io.h>
#include <stdint.h>

/** @brief ATmega8535/8515/16/32 devices - Timer0 (not available as OC0A is reserved for timing) */
#if defined(__AVR_ATmega8535__) || defined(__AVR_ATmega8515__) || defined(__AVR_ATmega16__) || defined(__AVR_ATmega32__)
// const uint8_t PWM_OC0 = 0; // OC0A is used for millisecond timing, do not use
#endif

/** @brief ATmega1284P/328P/ATtiny45 devices - Timer0 Channel B */
#if defined(__AVR_ATmega1284P__) || defined(__AVR_ATmega328P__) || defined(__AVR_ATtiny45__)
// const uint8_t PWM_OC0A = 1; // OC0A is used for millisecond timing, do not use
/** @brief Timer0 Channel B (avoid if SPI is used on ATmega1284P) */
const uint8_t PWM_OC0B = 2;
#endif

/** @brief ATmega8515/1284P/328P/ATtiny45 devices - Timer1 Channels */
#if defined(__AVR_ATmega8515__) || defined(__AVR_ATmega1284P__) || defined(__AVR_ATmega328P__) || defined(__AVR_ATtiny45__)
/** @brief Timer1 Channel A */
const uint8_t PWM_OC1A = 3;
/** @brief Timer1 Channel B (avoid if SPI is used on ATmega328P) */
const uint8_t PWM_OC1B = 4;
#endif

/** @brief ATmega8535/16/32 devices - Timer1 and Timer2 */
#if defined(__AVR_ATmega8535__) || defined(__AVR_ATmega16__) || defined(__AVR_ATmega32__)
/** @brief Timer1 Channel A */
const uint8_t PWM_OC1A = 5;
/** @brief Timer1 Channel B */
const uint8_t PWM_OC1B = 4;
/** @brief Timer2 */
const uint8_t PWM_OC2 = 7;
#endif

/** @brief ATmega1284P/328P devices - Timer2 Channels */
#if defined(__AVR_ATmega1284P__) || defined(__AVR_ATmega328P__)
/** @brief Timer2 Channel A (avoid if SPI is used on ATmega328P) */
const uint8_t PWM_OC2A = 5;
/** @brief Timer2 Channel B */
const uint8_t PWM_OC2B = 6;
#endif

/** @brief ATmega1284P device - Timer3 Channels */
#if defined(__AVR_ATmega1284P__)
/** @brief Timer3 Channel A (avoid if SPI is used on ATmega1284P) */
const uint8_t PWM_OC3A = 7;
/** @brief Timer3 Channel B (avoid if SPI is used on ATmega1284P) */
const uint8_t PWM_OC3B = 8;
#endif

/**
 * @brief Enable standard PWM on the specified pin
 * 
 * Configures the timer and output compare unit for PWM generation.
 * The pin is set up in PWM mode with 8-bit resolution (0-255).
 * 
 * @param pwm_pin PWM channel identifier (PWM_OC0B, PWM_OC1A, etc.)
 * 
 * @see setPWM()
 */
void enablePWM(uint8_t pwm_pin);

/**
 * @brief Set PWM duty cycle for standard PWM
 * 
 * Updates the output compare register for the specified PWM pin.
 * 
 * @param pwm_pin PWM channel identifier (PWM_OC0B, PWM_OC1A, etc.)
 * @param value Duty cycle value (0-255, where 0 = always off, 255 = always on)
 * 
 * @note pwm_pin must be initialized with enablePWM() first
 */
void setPWM(uint8_t pwm_pin, uint8_t value);

/**
 * @brief Enable servo-compatible PWM on the specified pin
 * 
 * Configures the timer for servo control with 20ms period and variable
 * pulse width (typically 1-2ms for servo control).
 * 
 * @param pwm_pin PWM channel identifier (PWM_OC1A, PWM_OC1B, etc.)
 * 
 * @note Servo PWM typically requires 16-bit timer (Timer1 or Timer3)
 * @see setServoPWM()
 */
void enableServoPWM(uint8_t pwm_pin);

/**
 * @brief Set servo PWM pulse width
 * 
 * Sets the pulse width for servo control. Values typically range from 1000-2000
 * representing 1ms-2ms pulse widths (at 16MHz clock), but exact range depends
 * on timer prescaler settings.
 * 
 * @param pwm_pin PWM channel identifier (PWM_OC1A, PWM_OC1B, etc.)
 * @param value Pulse width value in timer ticks
 * 
 * @note pwm_pin must be initialized with enableServoPWM() first
 */
void setServoPWM(uint8_t pwm_pin, uint16_t value);

#endif