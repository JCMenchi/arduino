#ifndef frame_h
#define frame_h

#include <Arduino.h>

#include "shiftregister.h"
#include "button.h"

const uint8_t END_FRAME = 1;

// snake
const uint8_t snakeFrames[9] = {
    138, 152, 208, 112, 224, 164, 134, 14, END_FRAME
};

// circle
const uint8_t circleFrames[7] = {
    2, 8, 32, 64, 16, 4, END_FRAME
};

// circle
const uint8_t circle2Frames[11] = {
    2, 10, 42, 106, 122, 126, 118, 86, 22, 6, END_FRAME
};

void runFrame(const uint8_t* frame, SimpleShiftRegister* shiftreg, SimpleButton* button, boolean loop = false) {
  int i = 0;
  int red = 1;
  while (frame[i] != END_FRAME) {
    shiftreg->setMask(frame[i]+red);
    delay(200);
    i++;
    if (red) {
      red = 0;
    } else {
      red = 1;
    }
    if (frame[i] == END_FRAME) {
      i = 0;
      if (!loop) {
        break;
      }
    }

    if (button->isPressed()) {
      shiftreg->setMask(0);
      button->reset();
      break;
    }
  }
}

void runFrameByStep(const uint8_t* frame, SimpleShiftRegister* shiftreg, SimpleButton* button) {
  int i = 0;
  int prev = -1;
  int red = 1;

  while (frame[i] != END_FRAME) {

    if (i != prev) {
      shiftreg->setMask(frame[i]+red);
      prev = i;
      if (red) {
        red = 0;
      } else {
        red = 1;
      }
    }

    if (button->isPressed()) {
      i++;
      button->reset();
    }
  }
}

#endif