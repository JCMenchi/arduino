/**
 * @file gpio.h
 * @brief GPIO (General Purpose Input/Output) macro definitions for AVR microcontrollers
 * 
 * Provides low-level bitwise operations for GPIO control including:
 * - Pin direction configuration (input/output)
 * - Pin state control (high/low)
 * - Pull-up/pull-down resistor configuration
 * - Pin state reading
 * 
 * @note These macros work with PORT, DDR, and PIN registers of AVR devices
 * @note PORTID parameter should be a letter (A, B, C, D, etc.) without quotes
 * @note BITNUM should be 0-7 representing individual bits in the port
 */

#ifndef _AVRTOOLS_GPIO_H
#define _AVRTOOLS_GPIO_H

#include <avr/io.h>

/** @brief GPIO logic level HIGH (1) */
#define GPIO_HIGH 0x1
/** @brief GPIO logic level LOW (0) */
#define GPIO_LOW  0x0

/** @brief Internal macro for concatenating preprocessor tokens */
#define CONCAT(x,y) x ##  y

/**
 * @def GPIO_SET_HIGH(PORTID, BITNUM)
 * @brief Set a GPIO pin to HIGH logic level
 * @param PORTID Port letter (A, B, C, etc.)
 * @param BITNUM Bit number (0-7)
 */
#define GPIO_SET_HIGH(PORTID, BITNUM) CONCAT(PORT, PORTID) |= (1 << BITNUM)

/**
 * @def GPIO_SET_LOW(PORTID, BITNUM)
 * @brief Set a GPIO pin to LOW logic level
 * @param PORTID Port letter (A, B, C, etc.)
 * @param BITNUM Bit number (0-7)
 */
#define GPIO_SET_LOW(PORTID, BITNUM) CONCAT(PORT, PORTID) &= ~(1 << BITNUM)

/**
 * @def GPIO_OUTPUT(PORTID, BITNUM)
 * @brief Configure a GPIO pin as output
 * @param PORTID Port letter (A, B, C, etc.)
 * @param BITNUM Bit number (0-7)
 */
#define GPIO_OUTPUT(PORTID, BITNUM) CONCAT(DDR, PORTID) |= (1 << BITNUM)

/**
 * @def GPIO_INPUT(PORTID, BITNUM)
 * @brief Configure a GPIO pin as input (high-impedance)
 * @param PORTID Port letter (A, B, C, etc.)
 * @param BITNUM Bit number (0-7)
 */
#define GPIO_INPUT(PORTID, BITNUM) CONCAT(DDR, PORTID) &= ~(1 << BITNUM)

/**
 * @def GPIO_INPUT_PULLUP(PORTID, BITNUM)
 * @brief Configure a GPIO pin as input with internal pull-up resistor enabled
 * @param PORTID Port letter (A, B, C, etc.)
 * @param BITNUM Bit number (0-7)
 */
#define GPIO_INPUT_PULLUP(PORTID, BITNUM) CONCAT(DDR, PORTID) &= ~(1 << BITNUM); CONCAT(PORT, PORTID) |= (1 << BITNUM)

/**
 * @def GPIO_INPUT_PULLDOWN(PORTID, BITNUM)
 * @brief Configure a GPIO pin as input with internal pull-down resistor enabled
 * @param PORTID Port letter (A, B, C, etc.)
 * @param BITNUM Bit number (0-7)
 */
#define GPIO_INPUT_PULLDOWN(PORTID, BITNUM) CONCAT(DDR, PORTID) &= ~(1 << BITNUM); CONCAT(PORT, PORTID) &= ~(1 << BITNUM)

/**
 * @def GPIO_READ(PORTID, BITNUM)
 * @brief Read the current state of a GPIO pin
 * @param PORTID Port letter (A, B, C, etc.)
 * @param BITNUM Bit number (0-7)
 * @return GPIO_HIGH if pin is high, GPIO_LOW if pin is low
 */
#define GPIO_READ(PORTID, BITNUM) (CONCAT(PIN, PORTID) & (1 << BITNUM))?GPIO_HIGH:GPIO_LOW

/**
 * @def GPIO_IS_LOW(PORTID, BITNUM)
 * @brief Check if a GPIO pin is at LOW logic level
 * @param PORTID Port letter (A, B, C, etc.)
 * @param BITNUM Bit number (0-7)
 * @return true if pin is low, false if pin is high
 */
#define GPIO_IS_LOW(PORTID, BITNUM) (CONCAT(PIN, PORTID) & (1 << BITNUM))?false:true

/**
 * @def GPIO_IS_HIGH(PORTID, BITNUM)
 * @brief Check if a GPIO pin is at HIGH logic level
 * @param PORTID Port letter (A, B, C, etc.)
 * @param BITNUM Bit number (0-7)
 * @return true if pin is high, false if pin is low
 */
#define GPIO_IS_HIGH(PORTID, BITNUM) (CONCAT(PIN, PORTID) & (1 << BITNUM))?true:false

#endif
