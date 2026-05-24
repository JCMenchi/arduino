/**
 * @file millisec.h
 * @brief Millisecond timer functionality for AVR microcontrollers
 * 
 * Provides functions to initialize a timer and retrieve elapsed time in milliseconds.
 * Typically uses Timer0 with interrupt-based counting for accurate timing.
 * 
 * @note Requires proper timer initialization before use
 * @note Timer resolution depends on clock frequency and prescaler settings
 */

#ifndef _AVRTOOLS_MILLISEC_H
#define _AVRTOOLS_MILLISEC_H

#include <stdint.h>

/**
 * @brief Get elapsed time in milliseconds
 * 
 * Returns the number of milliseconds elapsed since init_timer() was called.
 * The counter is a 32-bit unsigned integer that will overflow after
 * approximately 49.7 days of continuous operation.
 * 
 * @return Elapsed milliseconds as uint32_t
 */
uint32_t milliseconds();

/**
 * @brief Initialize the timer for millisecond counting
 * 
 * Sets up Timer0 (or equivalent) with appropriate prescaler and interrupt
 * configuration for 1ms resolution. Must be called once during system
 * initialization before using milliseconds().
 * 
 * @note This function configures interrupts; ensure global interrupts are
 *       enabled after calling this function
 */
void init_timer();

#endif