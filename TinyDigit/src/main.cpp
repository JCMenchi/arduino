#include <Arduino.h>

#include "shiftregister.h"
#include "button.h"
#include "frame.h"

const uint8_t numberToRegister[10] = {
  0x7E, // ZERO
  0x28, // ONE
  0xDA, // TWO
  0xEA, // THREE
  0xAC, // FOUR
  0xE6, // FIVE
  0xF6, // SIX
  0x2A, // SEVEN
  0xFE, // HEIGHT
  0xEE  // NINE
};

uint8_t count = 0;

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

// Set up environment
void setup() {
  // setup shiftregister
  shiftreg.setup();
  
  // setup button interrupt
  button.setup();

  // all pin are connected so the best we can do is to use micros
  randomSeed(micros());

  shiftreg.setMask(0);

  // setup analog
  pinMode(POT_PIN, INPUT);
}

void countButtonPress() {

  shiftreg.setMask(numberToRegister[count]);
  shiftreg.sendData();

  count += 1;
  if (count >= 10) {
    count = 0;
  }
}

void rolldice() {
    const int period = 30;
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

void runFrame(const uint16_t* frame) {
  int i = 0;

  while (frame[i] != 256) {
    shiftreg.setMask(frame[i]);
    delay(50);
    i++;
    if (frame[i] == 256) i = 0;

    if (button.isPressed()) {
      shiftreg.setMask(0);
      delay(20);
      button.reset();
      break;
    }
  }
  rolldice();
}

int previousValue = -1;
int hyst = 2;

// Main loop
void loop() {
  int v = analogRead(POT_PIN);

  if (v < previousValue - hyst || v > previousValue + hyst) {
    int idx = v * 9 / 1020;
    shiftreg.setMask(numberToRegister[idx]);
    previousValue = v;
  }

  if (button.isPressed()) {
    button.reset();
    runFrame(circleFrames);
  }
}

