#include <Arduino.h>

// countdown
const uint16_t countdownFrames[11] = {
    0xEE, // NINE
    0xFE, // HEIGHT
    0x2A, // SEVEN
    0xF6, // SIX
    0xE6, // FIVE
    0xAC, // FOUR
    0xEA, // THREE
    0xDA, // TWO
    0x28, // ONE
    0x7E, // ZERO
    256
};
const uint8_t countdownFrameNumber = 10;

// snake
const uint16_t snakeFrames[9] = {
    138, 152, 208, 112, 224, 164, 134, 14, 256
};
const uint8_t snakeFrameNumber = 8;

// circle
const uint16_t circleFrames[7] = {
    2, 8, 32, 64, 16, 4, 256
};
const uint8_t circleFrameNumber = 6;

// circle
const uint16_t circle2Frames[7] = {
    2, 10, 42, 106, 122, 126, 256
};
const uint8_t circle2FrameNumber = 6;

const uint16_t* frames[5] = {
    countdownFrames, snakeFrames, circleFrames, circle2Frames, NULL
};