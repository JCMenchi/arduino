#ifndef ARDUINO
#include <millisec.h>
#else
#include <Arduino.h>
#endif

#include <tinyspi.h>

//#define AS_MASTER

const int SS_PIN = PB4; 

const int LED_PIN = PB3;
uint8_t ledState = LOW;

unsigned long previousMillis = 0;  // will store last time LED was updated
const long interval = 1000;  // interval at which to blink (milliseconds)

const uint8_t INIT = 1;
const uint8_t WAIT = 2;
uint8_t state = INIT;

void setup() {

#ifdef AS_MASTER
  spi_master_init();

  pinMode(SS_PIN, OUTPUT);
  digitalWrite(SS_PIN, HIGH);
#else
  spi_slave_init();
  pinMode(SS_PIN, INPUT_PULLUP);
  state = INIT;
  spi_send(0);
#endif

  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, ledState);
}

uint8_t count = 0;

uint8_t last = 0;

const char* hello = "Hello World";

void loop() {
  
#ifdef AS_MASTER
  unsigned long currentMillis = millis();

  if (currentMillis - previousMillis >= interval) {
    // save the last time you blinked the LED
    previousMillis = currentMillis;

    digitalWrite(LED_PIN, HIGH);
    delay(100);


    digitalWrite(SS_PIN, LOW);
    delay(5);
    uint8_t d = spi_exchange(count++);
    spi_wait();
    delay(5);
    spi_send(d);
    spi_wait();
    //delay(5);
    digitalWrite(SS_PIN, HIGH);

    delay(50);
    //digitalWrite(SS_PIN, LOW);
    //spi_bulk_send((uint8_t*)hello, strlen(hello));
    //digitalWrite(SS_PIN, HIGH);

    digitalWrite(LED_PIN, ledState);
  }

  #else

  uint8_t cs = digitalRead(SS_PIN);

  if (cs == LOW && state == INIT) {
    digitalWrite(LED_PIN, HIGH);
    state = WAIT;
    spi_send(42);
  } else if (cs == HIGH && state == WAIT) {
    last = spi_get();
    state = INIT;
    digitalWrite(LED_PIN, LOW);
  }

  #endif
}

#ifndef ARDUINO
#include <main.cpp.h>
#endif