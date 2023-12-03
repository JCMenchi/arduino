#include <Arduino.h>

const int LED_PIN = PB4; // PB4 leg 3 on DIP8 chip

int ledState = LOW;  // ledState used to set the LED

unsigned long previousMillis = 0;  // will store last time LED was updated

const long interval = 25;  // interval at which to blink (milliseconds)

void setup() {
  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, LOW);  
}

void loop() {
  unsigned long currentMillis = millis();

  if (currentMillis - previousMillis >= interval) {
    // save the last time you blinked the LED
    previousMillis = currentMillis;

    // if the LED is off turn it on and vice-versa:
    if (ledState == LOW) {
      ledState = HIGH;
    } else {
      ledState = LOW;
    }

    // set the LED with the ledState of the variable:
    digitalWrite(LED_PIN, ledState);
  }
}
