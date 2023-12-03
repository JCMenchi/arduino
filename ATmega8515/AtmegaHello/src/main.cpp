#include <Arduino.h>

const int LED_PIN = 31; // PA7 leg 32 on chip
const uint8_t BUTTON_PIN = 16; // PC0 leg 21
int ledState = LOW;  // ledState used to set the LED

unsigned long previousMillis = 0;  // will store last time LED was updated

const long interval = 1000;  // interval at which to blink (milliseconds)

void setup() {
  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, LOW);

  pinMode(BUTTON_PIN, INPUT_PULLUP);

  Serial.begin(9600);
}

void loop() {
  
  unsigned long currentMillis = millis();

  uint8_t v = digitalRead(BUTTON_PIN);
  //Serial.printf("Button %d\n", v);

  if (v == 0) {
    ledState = HIGH;
  } else if (currentMillis - previousMillis >= interval) {
    // save the last time you blinked the LED
    previousMillis = currentMillis;

    // if the LED is off turn it on and vice-versa:
    if (ledState == LOW) {
      ledState = HIGH;
    } else {
      ledState = LOW;
    }
  }

  // set the LED with the ledState of the variable:
    digitalWrite(LED_PIN, ledState);
}
