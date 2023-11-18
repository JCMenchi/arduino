#include <Arduino.h>

#include "shiftregister.h"
#include "button.h"
#include "frame.h"

const uint8_t numberToRegister[10] = {
  0x7E, // ZERO
  0x28, // ONE
  0xDA, // TWO
  0xEA, // THREE
  0xAC, // FOUR
  0xE6, // FIVE
  0xF6, // SIX
  0x2A, // SEVEN
  0xFE, // HEIGHT
  0xEE  // NINE
};

uint8_t count = 0;

// declare a button
const uint8_t BUTTON_PIN = 2; // blue
SimpleButton button(BUTTON_PIN);

// declare a shift register
const uint8_t SHIFT_REG_DATA_PIN = 7; // purple
const uint8_t SHIFT_REG_LATCH_PIN = 8; // yellow
const uint8_t SHIFT_REG_CLOCK_PIN = 10; // green

SimpleShiftRegister shiftreg(SHIFT_REG_DATA_PIN, SHIFT_REG_LATCH_PIN, SHIFT_REG_CLOCK_PIN);

// declare pin to read analog
const uint8_t POT_PIN = A0;

// Set up environment
void setup() {
  // setup serial com
  Serial.begin(115200);
  Serial.println("Welcome");

  // setup shiftregister
  shiftreg.setup(&Serial);

  // setup button interrupt
  button.setup(&Serial);

  // if analog input pin 0 is unconnected, random analog
  // noise will cause the call to randomSeed() to generate
  // different seed numbers each time the sketch runs.
  // randomSeed() will then shuffle the random function.
  randomSeed(analogRead(1));

  shiftreg.setMask(0);

  // setup analog
  pinMode(POT_PIN, INPUT);

}

void countButtonPress() {

  shiftreg.setMask(numberToRegister[count]);
  shiftreg.sendData();

  count += 1;
  if (count >= 10) {
    count = 0;
  }
}

void rolldice() {
    // blink red LED to sim rolling
    shiftreg.setMask(1);
    delay(250);
    shiftreg.setMask(0);
    delay(250);
    shiftreg.setMask(1);
    delay(250);
    shiftreg.setMask(0);
    delay(250);
    shiftreg.setMask(1);
    delay(250);
    shiftreg.setMask(0);
    delay(250);
    shiftreg.setMask(1);
    delay(250);
    shiftreg.setMask(0);
    delay(250);
    shiftreg.setMask(1);
    delay(250);
    shiftreg.setMask(0);
    delay(250);

    int num = random(10);
    shiftreg.setMask(numberToRegister[num]);
}

void runFrame(const uint16_t* frame) {
  int i = 0;

  while (frame[i] != 256) {
    Serial.print("Display frame[");
    Serial.print(i);
    Serial.print("]=");
    Serial.println(frame[i], 16);
    shiftreg.setMask(frame[i]);
    delay(200);
    i++;
    if (frame[i] == 256) i = 0;
  }
  shiftreg.setMask(0);
}

int previousValue = -1;
int hyst = 2;

// Main loop
void loop() {
  int v = analogRead(POT_PIN);

  if (v < previousValue - hyst || v > previousValue + hyst) {
    Serial.print("Read value: ");
    Serial.print(v);
    Serial.print(" => ");
    int idx = v * 9 / 1020;
    Serial.println(idx);
    shiftreg.setMask(numberToRegister[v * 9 / 1020]);
    previousValue = v;
  }

  if (button.isPressed()) {
    Serial.println("Button pressed.");
    
    //runFrame(circleFrames);
    button.reset();

    
    Serial.print("Read value: ");
    Serial.println(v);
  }
}

void processCommand(int cmd) {
  if (cmd == 'O') {
    Serial.println("all high");
    shiftreg.allHigh();
  } else if (cmd == 'o') {
    Serial.println("all low");
    shiftreg.allLow();
  } else if (cmd == 'r') {
    rolldice();
  } else if (cmd == 'D') {
    shiftreg.print();
  } else if (cmd >= '0' && cmd <= '9') {
    uint8_t digit = cmd - '0';
    shiftreg.setMask(numberToRegister[digit]);
  }
}

// cppcheck-suppress unusedFunction
void serialEvent() {
  while (Serial.available()) {
    // read the most recent byte (which will be from 0 to 255):
    int cmd = Serial.read();
    processCommand(cmd);
  }
}
