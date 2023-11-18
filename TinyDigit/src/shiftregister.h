#include <Arduino.h>

class SimpleShiftRegister
{
public:
    SimpleShiftRegister(uint8_t data, uint8_t latch, uint8_t clock)
        : dataPin(data), latchPin(latch), clockPin(clock), printer(NULL)
    {
        memset(state, 0, 8);
    }

    void setup(Print *p = NULL)
    {
        this->printer = p;

        pinMode(this->dataPin, OUTPUT);
        pinMode(this->latchPin, OUTPUT);
        pinMode(this->clockPin, OUTPUT);

        digitalWrite(this->latchPin, HIGH);
        digitalWrite(this->dataPin, LOW);
        digitalWrite(this->clockPin, LOW);
    }

    void toggleBit(uint8_t pos)
    {
        if (pos >= 8)
        {
            if (printer)
            {
                printer->print("SimpleShiftRegister:toggleBit out of range error: ");
                printer->println(pos);
            }
            return;
        }

        if (printer)
        {
            printer->print("Toggle ");
            printer->println(pos);
        }

        if (state[pos] == 0)
        {
            state[pos] = 1;
        }
        else
        {
            state[pos] = 0;
        }
    }

    void sendData()
    {
        uint8_t v = this->getMask();

        // take the latchPin low so
        // the LEDs don't change while we are sending in bits:
        digitalWrite(this->latchPin, LOW);
        // shift out the bits:
        shiftOut(this->dataPin, this->clockPin, MSBFIRST, v);
        // take the latch pin high so the LEDs will light up:
        digitalWrite(this->latchPin, HIGH);
    }

    void setMask(uint8_t v)
    {

        for (int i = 0; i < 8; i++)
        {
            this->state[i] = bitRead(v, i);
        }

        this->sendData();
    }

    uint8_t getMask() const
    {
        uint8_t v = 0;

        for (int i = 0; i < 8; i++)
        {
            if (this->state[i] == 1)
            {
                bitSet(v, i);
            }
        }

        return v;
    }

    void setState(uint8_t pos, uint8_t value)
    {
        if (pos >= 8)
        {
            if (printer)
            {
                printer->print("SimpleShiftRegister:setState out of range error: ");
                printer->println(pos);
            }
            return;
        }

        this->state[pos] = value;
    }

    uint8_t getState(uint8_t pos) const
    {
        if (pos >= 8)
        {
            if (printer)
            {
                printer->print("SimpleShiftRegister:getState out of range error: ");
                printer->println(pos);
            }
            return 0;
        }

        return this->state[pos];
    }

    void print() const
    {
        if (printer)
        {
            printer->print("ShiftRegister state: ");
            for (int i = 7; i >= 0; i--)
            {
                printer->print(this->getState(i));
                printer->print(" ");
            }
            printer->print(" DEC ");
            printer->print(this->getMask());
            printer->print(" HEX ");
            printer->print(this->getMask(), 16);
            printer->println("");
        }
    }

    void allLow()
    {
        for (int i = 0; i < 8; i++)
        {
            setState(i, 0);
        }
        sendData();
    }

    void allHigh()
    {
        for (int i = 0; i < 8; i++)
        {
            setState(i, 1);
        }
        sendData();
    }

private:
    uint8_t dataPin;
    uint8_t latchPin;
    uint8_t clockPin;
    uint8_t state[8];
    Print *printer;
};
