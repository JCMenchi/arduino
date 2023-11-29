#include <Arduino.h>

#include "digit.h"
#include "shiftregister.h"
#include "button.h"
#include "frame.h"
#include "modeselector.h"
#include "rolldice.h"

// declare a button
const uint8_t BUTTON_PIN = PB2;
SimpleButton button(BUTTON_PIN);

// declare a shift register
const uint8_t SHIFT_REG_DATA_PIN = PB4;
const uint8_t SHIFT_REG_LATCH_PIN = PB0;
const uint8_t SHIFT_REG_CLOCK_PIN = PB1;
SimpleShiftRegister shiftreg(SHIFT_REG_DATA_PIN, SHIFT_REG_LATCH_PIN, SHIFT_REG_CLOCK_PIN);

// declare pin to read analog
const uint8_t POT_PIN = PB3;
ModeSelector modeSelector(POT_PIN);

// Set up environment
void setup() {
  // setup shiftregister
  shiftreg.setup();
  shiftreg.setMask(0);
  
  // setup button interrupt
  button.setup();

  // setup mode selector
  modeSelector.setup();

  // seed for random
  // all pin are connected so the best we can do is to use micros
  randomSeed(micros());
}

// Main loop
void loop() {
  
  // check if new mode is selected
  int8_t idx = modeSelector.update();
  if (idx != -1) {
    shiftreg.setMask(numberToRegister[idx]);
  }

  // wait for button pressed
  if (button.isPressed()) {
    // enter mode main loop
    button.reset();

    if (modeSelector.mode() == 0) {
      // MODE 0: turn off all LED
      shiftreg.setMask(0);
    } else if (modeSelector.mode() == 1) {
      // MODE 1: turn on all LED
      shiftreg.setMask(0xFF);
    } else if (modeSelector.mode() == 2) {
      // MODE 2: execute animation step by step
      runFrameByStep(snakeFrames, &shiftreg, &button);
      shiftreg.setMask(numberToRegister[modeSelector.mode()]);
    } else if (modeSelector.mode() == 3) {
      // MODE 3: execute same animation once
      runFrame(snakeFrames, &shiftreg, &button);
      shiftreg.setMask(numberToRegister[modeSelector.mode()]);
    } else if (modeSelector.mode() == 4) {
      // MODE 4: execute same animation in loop
      runFrame(snakeFrames, &shiftreg, &button, true);
      shiftreg.setMask(numberToRegister[modeSelector.mode()]);
    } else if (modeSelector.mode() == 5) {
      // MODE 5: simulate rolling a 6 side dice
      rolldice(&shiftreg, 6);
    } else if (modeSelector.mode() == 6) {
      // MODE 6: 
      runFrame(circleFrames , &shiftreg, &button, false);
      shiftreg.setMask(numberToRegister[modeSelector.mode()]);
    } else if (modeSelector.mode() == 7) {
      // MODE 7: 
      runFrame(circle2Frames , &shiftreg, &button, false);
      shiftreg.setMask(numberToRegister[modeSelector.mode()]);
    } else if (modeSelector.mode() == 8) {
      // MODE 8: 
      runFrame(circleFrames, &shiftreg, &button, true);
      shiftreg.setMask(numberToRegister[modeSelector.mode()]);
    } else if (modeSelector.mode() == 9) {
      // MODE 9: 
      runFrame(circle2Frames, &shiftreg, &button, true);
      shiftreg.setMask(numberToRegister[modeSelector.mode()]);
    }
  }
}

