/**
 * @file main.cpp.h
 * @brief Standard Arduino-style main entry point for AVR microcontrollers
 * 
 * Provides a main() function that follows the Arduino framework pattern:
 * 1. Initializes system resources (timer for millisecond counting)
 * 2. Calls user-defined setup() function for initialization
 * 3. Enters infinite loop calling user-defined loop() function
 * 
 * Users must implement two functions:
 * - void setup() - Called once at startup for initialization
 * - void loop() - Called repeatedly in main loop
 * 
 * @note This file is typically included from a main.cpp implementation
 * @note Requires interrupt vector definitions for any interrupt-based peripherals
 */

#include <avr/interrupt.h>
#include <avr/pgmspace.h>
#include <millisec.h>

/**
 * @brief Initialize system resources
 * 
 * Currently initializes the millisecond timer.
 * Called automatically by main() before setup().
 */
void init() {
  init_timer();
}

/**
 * @brief Main program entry point
 * 
 * Standard Arduino-style entry point that:
 * 1. Calls init() for system initialization
 * 2. Calls setup() for user initialization
 * 3. Continuously calls loop() for main program logic
 * 
 * This function never returns; the infinite loop continues until
 * the microcontroller is reset or powered off.
 * 
 * @return This function does not return normally (infinite loop)
 */
int main(void) {
  init();
  setup();

  for (;;) {
    loop();
  }

  return 0;
}