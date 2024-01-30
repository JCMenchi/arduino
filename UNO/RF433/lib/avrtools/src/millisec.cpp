#include <millisec.h>

#include <avr/interrupt.h>
#include <util/atomic.h>

#if F_CPU == 16000000L

#endif

volatile uint32_t _milliseconds = 0;

uint32_t milliseconds() {
  uint32_t ms;
  ATOMIC_BLOCK(ATOMIC_RESTORESTATE) { ms = _milliseconds; }
  return ms;
}

void init_timer() {
  cli();

  TCCR0A = (1 << COM0A1) | (1 << WGM01);

  // set timer 0 prescale factor to 64, so one tick equals 4us
  TCCR0B = (1 << CS01) | (1 << CS00);
  OCR0A = 250; // set compare at 250 => 1000us = 1 ms

  // enable timer 0 compare A
  TIMSK0 = (1 << OCIE0A);

  sei();
}

ISR(TIMER0_COMPA_vect) { _milliseconds++; }