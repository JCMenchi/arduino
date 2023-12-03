#ifndef modeselector_h
#define modeselector_h

#include <Arduino.h>

class ModeSelector
{
public:
    explicit ModeSelector(uint8_t p) : pin(p), currentMode(-1)
    {
    }

    void setup()
    {
        // set as input to read potentiometer
        pinMode(this->pin, INPUT);
    }

    int8_t update()
    {
        int v = analogRead(this->pin);
        int8_t idx = v * 9 / 1020;
        if (this->currentMode != idx)
        {
            this->currentMode = idx;
            return this->currentMode;
        }
        return -1;
    }

    int8_t mode()
    {
        return this->currentMode;
    }

private:
    uint8_t pin;
    int8_t currentMode;
};

#endif