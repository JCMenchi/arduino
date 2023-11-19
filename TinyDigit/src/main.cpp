#include <Arduino.h>

#include "shiftregister.h"
#include "button.h"
#include "frame.h"
#include "modeselector.h"

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

  // setup analog
  modeSelector.setup();

  // seed for random
  // all pin are connected so the best we can do is to use micros
  randomSeed(micros());
}

// Count number of time button is pressed
uint8_t count = 0;
void countButtonPress() {
  shiftreg.setMask(numberToRegister[count]);
  count += 1;
  if (count >= 10) {
    count = 0;
  }
}

// rool virtual dice
void rolldice() {
    const int period = 100;
    // blink red LED to sim rolling
    shiftreg.setMask(1);
    delay(period);
    shiftreg.setMask(0);
    delay(period);
    shiftreg.setMask(1);
    delay(period);
    shiftreg.setMask(0);
    delay(period);
    shiftreg.setMask(1);
    delay(period);
    shiftreg.setMask(0);
    delay(period);
    shiftreg.setMask(1);
    delay(period);
    shiftreg.setMask(0);
    delay(period);
    shiftreg.setMask(1);
    delay(period);
    shiftreg.setMask(0);
    delay(period);

    int num = random(10);
    shiftreg.setMask(numberToRegister[num]);
}

void runFrame(const uint16_t* frame, boolean loop = false) {
  int i = 0;
  int red = 1;
  while (frame[i] != 256) {
    shiftreg.setMask(frame[i]+red);
    delay(200);
    i++;
    if (red) {
      red = 0;
    } else {
      red = 1;
    }
    if (frame[i] == 256) {
      i = 0;
      if (!loop) {
        break;
      }
    }

    if (button.isPressed()) {
      shiftreg.setMask(0);
      button.reset();
      break;
    }
  }

  shiftreg.setMask(numberToRegister[modeSelector.mode()]);
}


// Main loop
void loop() {
  
  int8_t idx = modeSelector.update();
  if (idx != -1) {
    shiftreg.setMask(numberToRegister[idx]);
  }

  if (button.isPressed()) {
    button.reset();

    if (modeSelector.mode() == 0) {
      runFrame(circleFrames);
    } else if (modeSelector.mode() == 1) {
      runFrame(circle2Frames);
    } else if (modeSelector.mode() == 2) {
      runFrame(circle2Frames);
    } else if (modeSelector.mode() == 3) {
      rolldice();
    } else if (modeSelector.mode() == 4) {
      runFrame(snakeFrames);
    } else if (modeSelector.mode() == 5) {
      shiftreg.setMask(numberToRegister[5]+1);
      delay(200);
      shiftreg.setMask(numberToRegister[5]);
      delay(200);
      shiftreg.setMask(numberToRegister[5]+1);
      delay(200);
      shiftreg.setMask(numberToRegister[5]);
      delay(200);
      shiftreg.setMask(numberToRegister[5]+1);
    } else if (modeSelector.mode() == 6) {
      runFrame(circleFrames, true);
    } else if (modeSelector.mode() == 7) {
      runFrame(circle2Frames, true);
    } else if (modeSelector.mode() == 8) {
      runFrame(circle2Frames, true);
    } else if (modeSelector.mode() == 9) {
      runFrame(snakeFrames, true);
    }
  }
}

