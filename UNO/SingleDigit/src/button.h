#include <Arduino.h>

class AbstractButton 
{
    public:
    virtual void interrupt() {}
};

static AbstractButton* ButtonArray[6] = { NULL, NULL, NULL, NULL, NULL, NULL };

#define BUTTON_ISR_IMPL(num) \
  void buttonISR##num() { \
    if (ButtonArray[num] != NULL) ButtonArray[num]->interrupt(); \
}

#define BUTTON_ISR_NAME(num) buttonISR##num

BUTTON_ISR_IMPL(0)
BUTTON_ISR_IMPL(1)
BUTTON_ISR_IMPL(2)
BUTTON_ISR_IMPL(3)
BUTTON_ISR_IMPL(4)
BUTTON_ISR_IMPL(5)

typedef void (*voidFuncPtr)(void);
static voidFuncPtr buttonISR[6] = {
    BUTTON_ISR_NAME(0),
    BUTTON_ISR_NAME(1),
    BUTTON_ISR_NAME(2),
    BUTTON_ISR_NAME(3),
    BUTTON_ISR_NAME(4),
    BUTTON_ISR_NAME(5)
};

const int DEBOUNCE_DELAY = 200;

class SimpleButton : AbstractButton
{
public:
    explicit SimpleButton(uint8_t p) : pin(p), printer(NULL) {
        buttonPressed = false;
    }

    void setup(Print *p)
    {
        this->printer = p;

        // setup button interrupt
        pinMode(this->pin, INPUT_PULLUP);

        ButtonArray[this->pin] = this;
        attachInterrupt(digitalPinToInterrupt(this->pin), buttonISR[this->pin], FALLING);
    }

    bool isPressed() const {
        return buttonPressed;
    }

    void reset() {
        buttonPressed = false;
    }

    void interrupt() override {
        unsigned long now = millis();
        if (now - lastInterrupt > DEBOUNCE_DELAY) {
            buttonPressed = true;
            lastInterrupt = now;
        }
    }

private:
    unsigned long lastInterrupt = 0;
    uint8_t pin;
    volatile uint8_t buttonPressed;
    Print *printer;
};

