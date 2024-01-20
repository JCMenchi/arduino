#include <avr/interrupt.h>
#include <avr/pgmspace.h>

#ifndef cbi
#define cbi(sfr, bit) (_SFR_BYTE(sfr) &= ~_BV(bit))
#endif
#ifndef sbi
#define sbi(sfr, bit) (_SFR_BYTE(sfr) |= _BV(bit))
#endif

void init() {
  // this needs to be called before setup() or some functions won't
  // work there
  cli();

  // on the ATmega168, timer 0 is also used for fast hardware pwm
  // (using phase-correct PWM would mean that timer 0 overflowed half as often
  // resulting in different millis() behavior on the ATmega8 and ATmega168)
  sbi(TCCR0A, WGM01);
  sbi(TCCR0A, WGM00);

  // set timer 0 prescale factor to 64
  sbi(TCCR0B, CS01);
  sbi(TCCR0B, CS00);

  // enable timer 0 overflow interrupt
  sbi(TIMSK0, TOIE0);

  // timers 1 and 2 are used for phase-correct hardware pwm
  // this is better for motors as it ensures an even waveform
  // note, however, that fast pwm mode can achieve a frequency of up
  // 8 MHz (with a 16 MHz clock) at 50% duty cycle

  // set timer 1 prescale factor to 64
  sbi(TCCR1B, CS11);
  sbi(TCCR1B, CS10);

  // put timer 1 in 8-bit phase correct pwm mode
  sbi(TCCR1A, WGM10);

  // set timer 2 prescale factor to 64
  sbi(TCCR2B, CS22);

  // configure timer 2 for phase correct pwm (8-bit)
  sbi(TCCR2A, WGM20);

  // set a2d prescale factor according to the cpu clock
#if (F_CPU >= 12800000L)  //128
	sbi(ADCSRA, ADPS2);
	sbi(ADCSRA, ADPS1);
	sbi(ADCSRA, ADPS0);
#endif

  // the bootloader connects pins 0 and 1 to the USART; disconnect them
  // here so they can be used as normal digital i/o; they will be
  // reconnected in Serial.begin()
  UCSR0B = 0;

  sei();
}

int main(void) {
  init();
  setup();

  for (;;) {
    loop();
  }

  return 0;
}