#ifndef ARDUINO
#include <avr/io.h>
#include <util/delay.h>
#include <millisec.h>
#else
#include <Arduino.h>
#endif

#include <gpio.h>

int ledState = GPIO_LOW;  // ledState used to set the LED

unsigned long previousMillis = 0;  // will store last time LED was updated

const long interval = 500;  // interval at which to blink (milliseconds)

void setup() {
    GPIO_OUTPUT(A, 0);
    GPIO_OUTPUT(A, 1);
    GPIO_OUTPUT(A, 2);
    GPIO_OUTPUT(A, 3);
    GPIO_OUTPUT(A, 4);
    GPIO_OUTPUT(A, 5);
    GPIO_OUTPUT(A, 6);
    GPIO_OUTPUT(A, 7);
    GPIO_OUTPUT(B, 0);
    GPIO_OUTPUT(B, 1);
    GPIO_OUTPUT(B, 2);
    GPIO_OUTPUT(B, 3);

    GPIO_SET_LOW(A, 0);
    GPIO_SET_LOW(A, 1);
    GPIO_SET_LOW(A, 2);
    GPIO_SET_LOW(A, 3);
    GPIO_SET_LOW(A, 4);
    GPIO_SET_LOW(A, 5);
    GPIO_SET_LOW(A, 6);
    GPIO_SET_LOW(A, 7);
    GPIO_SET_LOW(B, 0);
    GPIO_SET_LOW(B, 1);
    GPIO_SET_LOW(B, 2);

}

void loop() {
    unsigned long currentMillis = milliseconds();

    if (currentMillis - previousMillis >= interval) {
        // save the last time you blinked the LED
        previousMillis = currentMillis;

        // if the LED is off turn it on and vice-versa:
        if (ledState == GPIO_LOW) {
            ledState = GPIO_HIGH;
            GPIO_SET_HIGH(A, 0);
            GPIO_SET_HIGH(A, 1);
            GPIO_SET_HIGH(A, 2);
            GPIO_SET_HIGH(A, 3);
            GPIO_SET_HIGH(A, 4);
            GPIO_SET_HIGH(A, 5);
            GPIO_SET_HIGH(A, 6);
            GPIO_SET_HIGH(A, 7);
            GPIO_SET_HIGH(B, 0);
            GPIO_SET_HIGH(B, 1);
            GPIO_SET_HIGH(B, 2);

        } else {
            ledState = GPIO_LOW;
            GPIO_SET_LOW(A, 0);
            GPIO_SET_LOW(A, 1);
            GPIO_SET_LOW(A, 2);
            GPIO_SET_LOW(A, 3);
            GPIO_SET_LOW(A, 4);
            GPIO_SET_LOW(A, 5);
            GPIO_SET_LOW(A, 6);
            GPIO_SET_LOW(A, 7);
            GPIO_SET_LOW(B, 0);
            GPIO_SET_LOW(B, 1);
            GPIO_SET_LOW(B, 2);
   
        }
    }
    _delay_ms(10);  // Small delay to prevent excessive CPU usage
}

#ifndef ARDUINO
#include <main.cpp.h>
#endif