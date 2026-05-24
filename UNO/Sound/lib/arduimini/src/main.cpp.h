void init()
{
	// this needs to be called before setup() or some functions won't
	// work there
	cli();
	
	// on the ATmega168, timer 0 is also used for fast hardware pwm
	// (using phase-correct PWM would mean that timer 0 overflowed half as often
	// resulting in different millis() behavior on the ATmega8 and ATmega168)
#if !defined(__AVR_ATmega8__) && !defined(__AVR_ATtiny26__)
	sbi(TCCR0A, WGM01);
	sbi(TCCR0A, WGM00);
#endif  
	// set timer 0 prescale factor to 64
#if defined(__AVR_ATmega8__) || defined(__AVR_ATtiny26__)
	sbi(TCCR0, CS01);
	sbi(TCCR0, CS00);
#else
	sbi(TCCR0B, CS01);
	sbi(TCCR0B, CS00);
#endif
	// enable timer 0 overflow interrupt
#if defined(__AVR_ATmega8__)
	sbi(TIMSK, TOIE0);
#elif defined(__AVR_ATtiny2313__) 
	sbi(TIMSK, TOIE0);
#elif defined(__AVR_ATtiny26__)
    sbi(TIMSK, TOIE0);
#else
	sbi(TIMSK0, TOIE0);
#endif

	// timers 1 and 2 are used for phase-correct hardware pwm
	// this is better for motors as it ensures an even waveform
	// note, however, that fast pwm mode can achieve a frequency of up
	// 8 MHz (with a 16 MHz clock) at 50% duty cycle

	// set timer 1 prescale factor to 64
	sbi(TCCR1B, CS11);
	sbi(TCCR1B, CS10);
#if defined(__AVR_ATtiny26__)
    sbi(TCCR1A, PWM1A);
    sbi(TCCR1A, PWM1B);
    OCR1C = 0xFF; //8bit PWM
#else
	// put timer 1 in 8-bit phase correct pwm mode
	sbi(TCCR1A, WGM10);
#endif

	// set timer 2 prescale factor to 64
#if defined(__AVR_ATmega8__)
	sbi(TCCR2, CS22);
#else

#if (!defined(__AVR_ATtiny26__) && !defined(__AVR_ATtiny2313__) )
	sbi(TCCR2B, CS22);
#endif

#endif
	// configure timer 2 for phase correct pwm (8-bit)
#if defined(__AVR_ATmega8__)
	sbi(TCCR2, WGM20);
#else

#if (!defined(__AVR_ATtiny26__) && !defined(__AVR_ATtiny2313__) )
	sbi(TCCR2A, WGM20);
#endif

#endif

#if defined(__AVR_ATmega1280__)
	// set timer 3, 4, 5 prescale factor to 64
	sbi(TCCR3B, CS31);	sbi(TCCR3B, CS30);
	sbi(TCCR4B, CS41);	sbi(TCCR4B, CS40);
	sbi(TCCR5B, CS51);	sbi(TCCR5B, CS50);
	// put timer 3, 4, 5 in 8-bit phase correct pwm mode
	sbi(TCCR3A, WGM30);
	sbi(TCCR4A, WGM40);
	sbi(TCCR5A, WGM50);
#endif


#if !defined(__AVR_ATtiny2313__) // no ad device on tiny2313

	// set a2d prescale factor according to the cpu clock
#if (F_CPU < 400000) //16
    cbi(ADCSRA, ADPS2);
	cbi(ADCSRA, ADPS1);
	sbi(ADCSRA, ADPS0);
#elif (F_CPU >= 400000) && (F_CPU < 800000) //16
	cbi(ADCSRA, ADPS2);
	sbi(ADCSRA, ADPS1);
	cbi(ADCSRA, ADPS0);
#elif (F_CPU >= 800000) && (F_CPU < 1600000) //16
	cbi(ADCSRA, ADPS2);
	sbi(ADCSRA, ADPS1);
	sbi(ADCSRA, ADPS0);
#elif (F_CPU >= 1600000) && (F_CPU < 3200000) //16
	sbi(ADCSRA, ADPS2);
	cbi(ADCSRA, ADPS1);
	cbi(ADCSRA, ADPS0);
#elif (F_CPU >= 3200000) && (F_CPU < 6400000) //32
	sbi(ADCSRA, ADPS2);
	cbi(ADCSRA, ADPS1);
	sbi(ADCSRA, ADPS0);
#elif (F_CPU >= 6400000) && (F_CPU < 12800000L) //64
	sbi(ADCSRA, ADPS2);
	sbi(ADCSRA, ADPS1);
	cbi(ADCSRA, ADPS0);
#elif (F_CPU >= 12800000L)  //128
	sbi(ADCSRA, ADPS2);
	sbi(ADCSRA, ADPS1);
	sbi(ADCSRA, ADPS0);
#endif

	// disable a2d conversions by default, please invoke enable_adc() to active it


#if !defined(__AVR_ATtiny26__)
	// the bootloader connects pins 0 and 1 to the USART; disconnect them
	// here so they can be used as normal digital i/o; they will be
	// reconnected in Serial.begin()
#if defined(__AVR_ATmega8__)
	UCSRB = 0;
#else
	UCSR0B = 0;
#endif

#endif

#endif // ! tiny2313
    sei();
}


int main(void)
{
  init();
	setup();
    
	for (;;) {
		loop();
	}
        
	return 0;
}