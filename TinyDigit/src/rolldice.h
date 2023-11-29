#ifndef rolldice_h
#define rolldice_h

#include <Arduino.h>

#include "shiftregister.h"
#include "frame.h"

// rool virtual dice
void rolldice(SimpleShiftRegister* shiftreg, uint8_t nbside) {
    const int period = 200;
    uint8_t num = random(nbside);

    // blink red LED to sim rolling
    shiftreg->setMask(numberToRegister[9] + 1);
    delay(period);
    shiftreg->setMask(numberToRegister[8] + 0);
    delay(period);
    shiftreg->setMask(numberToRegister[7] + 1);
    delay(period);
    shiftreg->setMask(numberToRegister[6] + 0);
    delay(period);
    shiftreg->setMask(numberToRegister[5] + 1);
    delay(period);
    shiftreg->setMask(numberToRegister[4] + 0);
    delay(period);
    shiftreg->setMask(numberToRegister[3] + 1);
    delay(period);
    shiftreg->setMask(numberToRegister[2] + 0);
    delay(period);
    shiftreg->setMask(numberToRegister[1] + 1);
    delay(period);
    shiftreg->setMask(numberToRegister[0] + 0);
    delay(period);

    
    shiftreg->setMask(numberToRegister[num]);
}

#endif