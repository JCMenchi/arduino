/*
  Use XBee for remote com
 */

#include <EEPROM.h>

// SoftwareSerial is used to communicate with the XBee
#include <SoftwareSerial.h>

SoftwareSerial XBee(2, 3); // Arduino RX, TX (XBee Dout, Din)

class LEDBlinker {
public:
  LEDBlinker(uint8_t p) : m_pin(p), m_state(LOW) {}

  void setup() {
    uint8_t v = EEPROM.read(m_pin);
    if (v == 1) {
      m_state = HIGH;
    } else {
      m_state = LOW;
    }
    pinMode(m_pin, OUTPUT);
    digitalWrite(m_pin, m_state);
  }

  void on() {
    m_state = HIGH;
    EEPROM.update(m_pin, m_state);
    digitalWrite(m_pin, m_state);
  }

  void off() {
    m_state = LOW;
    EEPROM.update(m_pin, m_state);
    digitalWrite(m_pin, m_state);
  }

  void setPWM(int v) {
    Serial.print("Set PWM to ");
    Serial.println(v);
    if (v < 0) v = 0;
    if (v > 255) v = 255;
    
    analogWrite(m_pin, v);
  }

  boolean isOn() const { return m_state != LOW; }

  uint8_t toggle() {
    if (isOn()) {
      off();
    } else {
      on();
    }
    return m_state;
  }

  uint8_t m_pin;
  uint8_t m_state;
};

const uint8_t RED_LED = 5;
const uint8_t GREEN_LED = 10;
const uint8_t BLUE_LED = 3;

LEDBlinker red(RED_LED);
LEDBlinker green(GREEN_LED);
LEDBlinker blue(BLUE_LED);

uint8_t debug = 0;

void setup() {                
  debug = EEPROM.read(0);
 
  XBee.begin(9600);
  Serial.begin(115200);
  if (debug) {
    Serial.print("Bee DEBUG ON");
  } else {
    Serial.print("Bee DEBUG OFF");
  }
  
  analogReference(DEFAULT);

  pinMode(A0, INPUT);
  pinMode(A1, INPUT);
  pinMode(A2, INPUT);
  pinMode(A3, INPUT);
  pinMode(A4, INPUT);
  pinMode(A5, INPUT);

  pinMode(13, OUTPUT);
  digitalWrite(13, LOW);

  red.setup();
  green.setup();
  blue.setup();
}

void loop() {

  delay(1000);
  int val0 = analogRead(A0);
  int val1 = analogRead(A1);
  int val2 = analogRead(A2);
  int val3 = analogRead(A3);
  int val4 = analogRead(A4);
  int val5 = analogRead(A5);

  // red.setPWM(val1/4);

  if (debug) {
    Serial.print("Value A0: ");
    Serial.print(val0);
    Serial.print(" A1: ");
    Serial.print(val1);
    Serial.print(" A2: ");
    Serial.print(val2);
    Serial.print(" A3: ");
    Serial.print(val3);
    Serial.print(" A4: ");
    Serial.print(val4);
    Serial.print(" A5: ");
    Serial.println(val5);

    XBee.print("Value A0: ");
    XBee.print(val0);
    XBee.println(val5);
  }

  if (XBee.available()) { 
    // If data comes in from XBee, send it out to serial monitor
    Serial.write(XBee.read());
  }

  if (Serial.available()) {
    // read the most recent byte (which will be from 0 to 255):
    int brightness = Serial.read();
    //Serial.println(brightness);
    if (brightness == '1') {
      Serial.println("on");
      digitalWrite(13, HIGH);
    } else if (brightness == '0') {
      Serial.println("off");
      digitalWrite(13, LOW);
      red.off();
      green.off();
      blue.off();
    } else if (brightness == 'b') {
      blue.toggle();
    } else if (brightness == 'g') {
      green.toggle();
    } else if (brightness == 'r') {
      red.toggle();
    } else if (brightness == 'D') {
      if (debug) {
        debug = 0;
        Serial.print("DEBUG OFF");
      } else {
        debug = 1;
        Serial.print("DEBUG ON");
      }
      EEPROM.update(0, debug);
    }
  }
}
      
