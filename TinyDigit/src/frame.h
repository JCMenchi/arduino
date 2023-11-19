#ifndef frame_h
#define frame_h

#include <Arduino.h>

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

// snake
const uint16_t snakeFrames[17] = {
    138, 138, 152, 152, 208, 208, 112, 112, 224, 224, 164, 164, 134, 134, 14, 14, 256
};

// circle
const uint16_t circleFrames[7] = {
    2, 8, 32, 64, 16, 4, 256
};

// circle
const uint16_t circle2Frames[7] = {
    2, 10, 42, 106, 122, 126, 256
};

#endif