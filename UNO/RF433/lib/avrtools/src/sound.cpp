
#include <avr/interrupt.h>
#include <avr/pgmspace.h>

#include <usart_serial.h>

// timerx_toggle_count:
//  > 0 - duration specified
//  = 0 - stopped
//  < 0 - infinitely (until stop() method called, or new play() called)

volatile long timer2_toggle_count;
volatile uint8_t *timer2_pin_port;  // CPU PORT
volatile uint8_t timer2_pin_mask;   // mask 

volatile uint8_t tone_pin = 255;

static int8_t toneBegin(volatile uint8_t* mcu_port, uint8_t pin_on_port) {
  if (tone_pin == 255) {
    tone_pin = pin_on_port;

    // Set timer specific stuff
    // All timers in CTC mode
    // 8 bit timers will require changing prescalar values,
    // whereas 16 bit timers are set to either ck/1 or ck/64 prescalar

    // 8 bit timer
    TCCR2A = 0;
    TCCR2B = 0;
    TCCR2A |= (1 << WGM21);
    TCCR2B |= (1 << CS20);
    timer2_pin_port = mcu_port;
    timer2_pin_mask = (1 << pin_on_port);

    return 1;
  }

  return 0;
}

// frequency (in hertz) and duration (in milliseconds).
void playNote(volatile uint8_t* mcu_port, volatile uint8_t* mcu_ddr, uint8_t pin_on_port, unsigned int frequency, unsigned long duration) {
  uint8_t prescalarbits = 0b001;
  long toggle_count = 0;
  uint32_t ocr = 0;
  int8_t _timer;

  _timer = toneBegin(mcu_port, pin_on_port);

  if (_timer == 1) {
    // Set the pinMode as OUTPUT
    *mcu_ddr |= (1 << pin_on_port);

    // if we are using an 8 bit timer, scan through prescalars to find the best
    // fit
    ocr = F_CPU / frequency / 2 - 1;
    prescalarbits = 0b001; // ck/1: same for both timers
    if (ocr > 255) {
      ocr = F_CPU / frequency / 2 / 8 - 1;
      prescalarbits = 0b010; // ck/8: same for both timers

      if (_timer == 2 && ocr > 255) {
        ocr = F_CPU / frequency / 2 / 32 - 1;
        prescalarbits = 0b011;
      }

      if (ocr > 255) {
        ocr = F_CPU / frequency / 2 / 64 - 1;
        prescalarbits = _timer == 0 ? 0b011 : 0b100;

        if (_timer == 2 && ocr > 255) {
          ocr = F_CPU / frequency / 2 / 128 - 1;
          prescalarbits = 0b101;
        }

        if (ocr > 255) {
          ocr = F_CPU / frequency / 2 / 256 - 1;
          prescalarbits = _timer == 0 ? 0b100 : 0b110;
          if (ocr > 255) {
            // can't do any better than /1024
            ocr = F_CPU / frequency / 2 / 1024 - 1;
            prescalarbits = _timer == 0 ? 0b101 : 0b111;
          }
        }
      }
    }

    TCCR2B = (TCCR2B & 0b11111000) | prescalarbits;

    // Calculate the toggle count
    if (duration > 0) {
      toggle_count = 2 * frequency * duration / 1000;
    } else {
      toggle_count = -1;
    }

    // Set the OCR for the given timer,
    // set the toggle count,
    // then turn on the interrupts
    OCR2A = ocr;
    timer2_toggle_count = toggle_count;
    TIMSK2 |= (1 << OCIE2A);
  }
}

// XXX: this function only works properly for timer 2 (the only one we use
// currently).  for the others, it should end the tone, but won't restore
// proper PWM functionality for the timer.
void disableTimer() {
  TIMSK2 &= ~(1 << OCIE2A); // disable interrupt
  TCCR2A = (1 << WGM20);
  TCCR2B = (TCCR2B & 0b11111000) | (1 << CS22);
  OCR2A = 0;
}

void stopNote() {
  if (tone_pin != 255) {
    // stop timer
    disableTimer();
    // disable ouput (set LOW)
    *timer2_pin_port &= ~timer2_pin_mask;
    //
    tone_pin = 255;
  }
}

ISR(TIMER2_COMPA_vect) {

  if (timer2_toggle_count != 0) {
    // toggle the pin
    *timer2_pin_port ^= timer2_pin_mask;

    if (timer2_toggle_count > 0)
      timer2_toggle_count--;
  } else {
    stopNote();
  }
}
