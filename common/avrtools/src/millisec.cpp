/**
 * @file millisec.cpp
 * @brief Implementation of millisecond timer for AVR microcontrollers
 * 
 * Provides interrupt-driven millisecond counting using Timer0.
 * Supports multiple AVR device families with appropriate register mappings.
 * 
 * Device-specific timer configurations:
 * - ATmega8535/16/32: TCCR0 (8-bit), prescaler 64, OCR0 = 125 (1ms)
 * - ATtiny45/84: TCCR0A/B (newer style), prescaler 64, OCR0A = 250 (1ms)
 * - ATmega1284P/328P: TCCR0A/B (newer style), prescaler 64, OCR0A = 125/250
 * 
 * @note Uses atomic operations to safely read 32-bit counter
 * @note Counter overflows after ~49.7 days of continuous operation
 */

#include <millisec.h>

#include <avr/interrupt.h>
#include <util/atomic.h>

/** @brief Global millisecond counter (volatile, atomically updated) */
volatile uint32_t _milliseconds = 0;

/**
 * @brief Get elapsed milliseconds since init_timer() was called
 * 
 * Reads the 32-bit millisecond counter with atomic operations to
 * ensure consistent 32-bit value even if interrupt fires during read.
 * 
 * @return Milliseconds elapsed since timer initialization
 * 
 * @note Thread-safe via ATOMIC_BLOCK
 * @note Counter wraps after ~49.7 days of continuous operation
 */
uint32_t milliseconds() {
  uint32_t ms;
  ATOMIC_BLOCK(ATOMIC_RESTORESTATE) { ms = _milliseconds; }
  return ms;
}

/**
 * @brief Initialize Timer0 for 1ms interrupt-based counting
 * 
 * Configures Timer0 with compare match interrupt to increment
 * the millisecond counter every 1ms. Device-specific register
 * configurations handle differences in timer layout across AVR family.
 * 
 * Prescaler and compare values adjusted per device:
 * - ATmega8535/16/32: Prescaler 64, OCR0=125 (125*8µs = 1ms)
 * - ATtiny45/84/ATmega328P/1284P: Prescaler 64, OCR0A=250 (250*4µs = 1ms)
 * 
 * @note Must be called during initialization before using milliseconds()
 * @note Enables global interrupts via sei()
 */
void init_timer() {
  cli();

/** @brief ATmega8535/8515/16/32 timer setup (older style 8-bit timer) */
#if defined(__AVR_ATmega8535__) || defined(__AVR_ATmega8515__) || defined(__AVR_ATmega16__) || defined(__AVR_ATmega32__)
  TCCR0 = (1 << COM01) | (1 << WGM01);

  // Prescaler 64: tick = 8µs, OCR0=125 gives 1000µs = 1ms
  TCCR0 |= (1 << CS01) | (1 << CS00);
  OCR0 = 125;

  // Enable compare match interrupt
  TIMSK |= (1 << OCIE0);

/** @brief ATtiny45 timer setup (newer style) */
#elif defined(__AVR_ATtiny45__)
  TCCR0A |= (1 << COM0A1) | (1 << WGM01);

  // Prescaler 64: tick = 4µs, OCR0A=250 gives 1000µs = 1ms
  TCCR0B |= (1 << CS01) | (1 << CS00);
  OCR0A = 250;

  // Enable compare match interrupt
  TIMSK |= (1 << OCIE0A);

/** @brief ATtiny84 timer setup (newer style) */
#elif defined(__AVR_ATtiny84__)
  TCCR0A |= (1 << COM0A1) | (1 << WGM01);

  // Prescaler 64: tick = 4µs, OCR0A=250 gives 1000µs = 1ms
  TCCR0B |= (1 << CS01) | (1 << CS00);
  OCR0A = 250;

  // Enable compare match interrupt
  TIMSK0 |= (1 << OCIE0A);

/** @brief ATmega1284P timer setup (newer style, 8MHz) */
#elif defined(__AVR_ATmega1284P__)
  TCCR0A = (1 << COM0A1) | (1 << WGM01);

  // Prescaler 64: tick = 8µs, OCR0A=125 gives 1000µs = 1ms
  TCCR0B = (1 << CS01) | (1 << CS00);
  OCR0A = 125;

  // Enable compare match interrupt
  TIMSK0 |= (1 << OCIE0A);

/** @brief Default timer setup for other devices (newer style) */
#else
  TCCR0A = (1 << COM0A1) | (1 << WGM01);

  // Prescaler 64: tick = 4µs, OCR0A=250 gives 1000µs = 1ms
  TCCR0B = (1 << CS01) | (1 << CS00);
  OCR0A = 250;

  // Enable compare match interrupt
  TIMSK0 |= (1 << OCIE0A);
#endif

  sei();  // Enable global interrupts
}

/**
 * @brief Timer0 compare match interrupt handler
 * 
 * Increments the global millisecond counter. Called every 1ms
 * as configured by init_timer().
 * 
 * @note Device-specific ISR vector names due to different naming
 *       conventions across AVR family
 */
#if defined(__AVR_ATtiny45__)
/** @brief ATtiny45 Timer0 Compare A interrupt */
ISR(TIMER0_COMPA_vect) { _milliseconds++; }
#elif defined(__AVR_ATtiny84__)
/** @brief ATtiny84 Timer0 Compare A interrupt */
ISR(TIM0_COMPA_vect) { _milliseconds++; }
#elif defined(__AVR_ATmega328P__)
/** @brief ATmega328P Timer0 Compare A interrupt */
ISR(TIMER0_COMPA_vect) { _milliseconds++; }
#elif defined(__AVR_ATmega1284P__)
/** @brief ATmega1284P Timer0 Compare A interrupt */
ISR(TIMER0_COMPA_vect) { _milliseconds++; }
#else
/** @brief Other ATmega devices Timer0 Compare interrupt */
ISR(TIMER0_COMP_vect) { _milliseconds++; }
#endif