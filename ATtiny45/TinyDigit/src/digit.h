#ifndef digit_h
#define digit_h

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

const uint8_t TOP = 2;
const uint8_t MIDDLE = 128;
const uint8_t BOTTOM = 64;
const uint8_t TOP_RIGHT = 8;
const uint8_t BOTTOM_RIGHT = 32;
const uint8_t TOP_LEFT = 4;
const uint8_t BOTTOM_LEFT = 16;

#endif