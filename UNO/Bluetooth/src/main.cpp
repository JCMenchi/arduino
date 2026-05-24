#include <Arduino.h>

#include <SoftwareSerial.h>

// pin
const uint8_t PIN_RXBLUE = PIN7;
const uint8_t PIN_TXBLUE = PIN6;

// receive on arduino is transmit on bluetooth
SoftwareSerial btserial(PIN_TXBLUE, PIN_RXBLUE);

void setup() {
  Serial.begin(115200);
  Serial.println("Welcome");

  btserial.begin(9600);

  btserial.println("AT+VERSION");
}

void loop() {
  int i = 0;
  char someChar[32] = {0};

  // when characters arrive over the serial port...
  if (Serial.available()) {
    do {
      someChar[i++] = Serial.read();
      delay(3);
    } while (Serial.available() > 0);

    btserial.println(someChar);
    Serial.println(someChar);
  }

  while (btserial.available())
    Serial.print((char)btserial.read());
}