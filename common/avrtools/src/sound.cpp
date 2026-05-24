
/**
 * @file sound.cpp
 * @brief Implementation of tone generation via Timer2 for AVR microcontrollers
 * 
 * Provides PWM-based tone generation at specified frequencies with
 * optional duration control. Uses Timer2 compare match interrupt
 * to toggle output pin at the appropriate rate.
 * 
 * Implementation notes:
 * - Tone pin is toggled on every timer interrupt
 * - For frequency f, toggle rate is 2*f (to complete full cycle)
 * - Supported on ATmega328P and similar; no-op on tinyAVR
 * 
 * @note Device-specific: Functions return immediately on tinyAVR (no tone support)
 */

#include <avr/interrupt.h>
#include <avr/pgmspace.h>

/**
 * Timer2 toggle counter:
 *  > 0: Duration specified in milliseconds (counts down to 0)
 *  = 0: Tone is stopped
 *  < 0: Plays infinitely (until stopNote() or new playNote() called)
 */
volatile long timer2_toggle_count;

/** @brief Pointer to port register for tone output */
volatile uint8_t *timer2_pin_port;
/** @brief Bit mask for tone pin within port */
volatile uint8_t timer2_pin_mask;

/** @brief Currently active tone pin (255 = no tone) */
volatile uint8_t tone_pin = 255;


/**
 * @brief Tone generation not supported on these devices
 * 
 * ATtiny and some ATmega devices don't have Timer2 available
 * or have other limitations that prevent reliable tone generation.
 */
#if defined(__AVR_ATtiny45__) || defined(__AVR_ATtiny84__) || defined(__AVR_ATmega32__) || defined(__AVR_ATmega16__) || defined(__AVR_ATmega8535__)

/** @brief No-op playNote for unsupported devices */
void playNote(volatile uint8_t* mcu_port, volatile uint8_t* mcu_ddr, uint8_t pin_on_port, unsigned int frequency, unsigned long duration) {}
/** @brief No-op stopNote for unsupported devices */
void stopNote() {}
/**
 * @brief Tone generation supported (ATmega328P and similar with Timer2)
 */
#else

/**
 * @brief Initialize Timer2 for tone generation on first call
 * 
 * Sets up Timer2 in CTC (Clear Timer on Compare) mode and records port/pin information.
 * Subsequent calls while a tone is active return immediately without reinitializing.
 * This is a static function only called internally.
 * 
 * @param mcu_port Pointer to PORT register for output pin
 * @param pin_on_port Bit number within the port (0-7)
 * @return 1 if Timer2 was successfully initialized, 0 if tone already active
 * 
 * @see toneBegin uses Timer2A compare interrupt for PWM generation
 */
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

/**
 * @brief Play a tone at specified frequency for a duration
 * 
 * Generates a square wave at the specified frequency using Timer2 in CTC mode.
 * Automatically selects the optimal prescaler and OCR value to achieve the target
 * frequency. The pin is configured as an output and toggled by the Timer2 interrupt.
 * 
 * If a tone is already playing, this call returns without starting a new tone.
 * 
 * @param mcu_port Pointer to PORT register for output pin (e.g., &PORTB)
 * @param mcu_ddr Pointer to DDR register for output pin (e.g., &DDRB)
 * @param pin_on_port Bit number within the port (0-7)
 * @param frequency Tone frequency in Hz (supported range depends on CPU clock and prescaler)
 * @param duration Duration in milliseconds; 0 = play indefinitely until stopNote() is called
 * 
 * @note Prescaler selection:
 * - F_CPU / freq / 2 with various prescalers (1, 8, 32, 64, 128, 256, 1024)
 * - Selects smallest prescaler that fits OCR value in 8-bit register
 * 
 * @note Timer toggle frequency is 2 * frequency (to complete a full square wave cycle)
 */
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

    // Set prescaler bits in Timer2 control register B
    TCCR2B = (TCCR2B & 0b11111000) | prescalarbits;

    // Calculate toggle count: 2 toggles per period, duration in ms
    // Toggle count = 2 * frequency * duration_ms / 1000
    if (duration > 0) {
      toggle_count = 2 * frequency * duration / 1000;
    } else {
      // Negative = play infinitely
      toggle_count = -1;
    }

    // Set the OCR for the given timer,
    // set the toggle count,
    // then turn on the interrupts
    // Set output compare register and enable interrupt
    OCR2A = ocr;
    timer2_toggle_count = toggle_count;
    TIMSK2 |= (1 << OCIE2A);
  }
}

/**
 * @brief Disable Timer2 and restore default state
 * 
 * Disables Timer2 compare match interrupt (OCIE2A) and resets Timer2 control
 * registers to default values. This is an internal helper function.
 * 
 * @see disableTimer is called by stopNote() to clean up Timer2 configuration
 */
void disableTimer() {
  // Disable Timer2 compare A interrupt
  TIMSK2 &= ~(1 << OCIE2A);
  // Reset to normal mode (not CTC)
  TCCR2A = (1 << WGM20);
  // Set prescaler to /64
  TCCR2B = (TCCR2B & 0b11111000) | (1 << CS22);
  // Clear compare value
  OCR2A = 0;
}

/**
 * @brief Stop tone generation immediately
 * 
 * Stops the currently playing tone by disabling Timer2 interrupt,
 * setting the tone output pin LOW, and marking tone generation as inactive.
 * Safe to call even when no tone is active.
 * 
 * @note Restores Timer2 to default state after stopping
 */
void stopNote() {
  if (tone_pin != 255) {
    // Disable timer interrupt
    disableTimer();
    // Set output pin LOW
    *timer2_pin_port &= ~timer2_pin_mask;
    // Mark no tone active
    tone_pin = 255;
  }
}

/**
 * @brief Timer2 compare match interrupt handler (TIMER2_COMPA_vect)
 * 
 * Interrupt service routine that executes on Timer2 compare match A.
 * Toggles the tone output pin at each interrupt to generate square wave.
 * 
 * Behavior:
 * - If toggle_count > 0: Decrements counter each interrupt (finite duration tone)
 * - If toggle_count < 0: Plays indefinitely (infinite duration)
 * - If toggle_count = 0: Stops tone generation and calls stopNote()
 * 
 * Toggling the pin at 2 * frequency completes one full square wave period
 * per note cycle.
 */
ISR(TIMER2_COMPA_vect) {

  if (timer2_toggle_count != 0) {
    // Toggle the tone output pin
    *timer2_pin_port ^= timer2_pin_mask;

    // Decrement counter if finite duration
    if (timer2_toggle_count > 0)
      timer2_toggle_count--;
  } else {
    // Tone finished, clean up
    stopNote();
  }
}

#endif