#include <pwm.h>

#include <gpio.h>

#include <avr/interrupt.h>

#ifdef HAS_SERIAL
#include <usart_serial.h>
#endif


#if F_CPU == 16000000L
const uint8_t NB_CYCLE_PER_MICRO_SEC = 16;
#elif F_CPU == 8000000L
const uint8_t NB_CYCLE_PER_MICRO_SEC = 8;

#else
#error "unknown CPU freq"
#endif


#if defined(__AVR_ATmega1284P__) 

// Use internal oscillator so F_CPU is 8MHz

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

        // calculate top for 20 ms period (1us per tick and fast PWM)
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

#elif defined(__AVR_ATmega8515__) 

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
