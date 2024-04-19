#include <millisec.h>

#include <avr/interrupt.h>
#include <util/atomic.h>

volatile uint32_t _milliseconds = 0;

uint32_t milliseconds() {
  uint32_t ms;
  ATOMIC_BLOCK(ATOMIC_RESTORESTATE) { ms = _milliseconds; }
  return ms;
}

void init_timer() {
  cli();

#if defined(__AVR_ATmega8535__) || defined(__AVR_ATmega8515__)
  TCCR0 = (1 << COM01) | (1 << WGM01);

  // set timer 0 prescale factor to 64, so one tick equals 8us
  TCCR0 |= (1 << CS01) | (1 << CS00);
  OCR0 = 125; // set compare at 125 => 1000us = 1 ms

  // enable timer 0 compare A
  TIMSK |= (1 << OCIE0);
#elif defined(__AVR_ATtiny45__)
  TCCR0A |= (1 << COM0A1) | (1 << WGM01);

  // set timer 0 prescale factor to 64, so one tick equals 4us
  TCCR0B |= (1 << CS01) | (1 << CS00);
  OCR0A = 250; // set compare at 250 => 1000us = 1 ms

  // enable timer 0 compare A
  TIMSK |= (1 << OCIE0A);
#elif defined(__AVR_ATmega1284P__)
  TCCR0A = (1 << COM0A1) | (1 << WGM01);

  // set timer 0 prescale factor to 64, so one tick equals 8us
  TCCR0B = (1 << CS01) | (1 << CS00);
  OCR0A = 125; // set compare at 125 => 1000us = 1 ms

  // enable timer 0 compare A
  TIMSK0 |= (1 << OCIE0A);
#else
  TCCR0A = (1 << COM0A1) | (1 << WGM01);

  // set timer 0 prescale factor to 64, so one tick equals 4us
  TCCR0B = (1 << CS01) | (1 << CS00);
  OCR0A = 250; // set compare at 250 => 1000us = 1 ms

  // enable timer 0 compare A
  TIMSK0 |= (1 << OCIE0A);
#endif

  sei();
}

#if defined(__AVR_ATtiny45__)
ISR(TIMER0_COMPA_vect) { _milliseconds++; }
#elif defined(__AVR_ATmega328P__)
ISR(TIMER0_COMPA_vect) { _milliseconds++; }
#elif defined(__AVR_ATmega1284P__)
ISR(TIMER0_COMPA_vect) { _milliseconds++; }
#else
ISR(TIMER0_COMP_vect) { _milliseconds++; }
#endif