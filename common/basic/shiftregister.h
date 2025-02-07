#ifndef shiftregister_h
#define shiftregister_h

#include <gpio.h>
#include <string.h>

#define SHR_LSBFIRST 0
#define SHR_MSBFIRST 1

#ifndef SHREG_PORTID
#define SHREG_PORTID A
#endif

class ShiftRegisterPort {
   public:
    /*
        All PIN are on PORTA
    */
    ShiftRegisterPort(uint8_t data, uint8_t latch, uint8_t clock)
        : dataPin(data), latchPin(latch), clockPin(clock) {
        memset(state, 0, 8);
    }

    void setup() {

        GPIO_OUTPUT(SHREG_PORTID, this->dataPin);
        GPIO_OUTPUT(SHREG_PORTID, this->latchPin);
        GPIO_OUTPUT(SHREG_PORTID, this->clockPin);

        GPIO_SET_HIGH(SHREG_PORTID, this->latchPin);
        GPIO_SET_LOW(SHREG_PORTID, this->dataPin);
        GPIO_SET_HIGH(SHREG_PORTID, this->clockPin);
    }

    void toggleBit(uint8_t pos) {
        if (pos >= 8) {
            return;
        }

        if (state[pos] == 0) {
            state[pos] = 1;
        } else {
            state[pos] = 0;
        }
    }

    void writeRegister(uint8_t val) {
        uint8_t i;

        for (i = 7; i >= 0; i--) {
            if (this->state[i] != 0) {
                GPIO_SET_HIGH(SHREG_PORTID, this->dataPin);
            } else {
                GPIO_SET_LOW(SHREG_PORTID, this->dataPin);
            }

            GPIO_SET_HIGH(SHREG_PORTID, this->clockPin);
            GPIO_SET_LOW(SHREG_PORTID, this->clockPin);
        }
    }

    void sendData() {
        uint8_t v = this->getMask();

        // take the latchPin low so
        // the LEDs don't change while we are sending in bits:
        GPIO_SET_LOW(SHREG_PORTID, this->latchPin);
        // shift out the bits:
        writeRegister(v);
        // take the latch pin high so the LEDs will light up:
        GPIO_SET_HIGH(SHREG_PORTID, this->latchPin);

        GPIO_SET_LOW(SHREG_PORTID, this->dataPin);
    }

    void setMask(uint8_t v) {
        for (int i = 0; i < 8; i++) {
            this->state[i] = v & (1 << i);
        }

        this->sendData();
    }

    uint8_t getMask() const {
        uint8_t v = 0;

        for (int i = 0; i < 8; i++) {
            if (this->state[i] == 1) {
                v |= (1 << i);
            }
        }

        return v;
    }

    void setState(uint8_t pos, uint8_t value) {
        if (pos >= 8) {
            return;
        }

        this->state[pos] = value;
    }

    uint8_t getState(uint8_t pos) const {
        if (pos >= 8) {
            return 0;
        }

        return this->state[pos];
    }

    void allLow() {
        for (int i = 0; i < 8; i++) {
            setState(i, 0);
        }
        sendData();
    }

    void allHigh() {
        for (int i = 0; i < 8; i++) {
            setState(i, 1);
        }
        sendData();
    }

   private:
    uint8_t dataPin;
    uint8_t latchPin;
    uint8_t clockPin;
    uint8_t state[8];
};

#endif