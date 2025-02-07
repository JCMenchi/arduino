#include <Arduino.h>

#include <shiftregister.h>

const uint8_t DATA_PIN = PIN4;  // DS
const uint8_t LATCH_PIN = PIN5; // ST_CP
const uint8_t CLOCK_PIN = PIN6; // SH_CP

const uint8_t MOTOR_ENABLE_PIN = PIN3; // PWM to 1,2EN 

#define SR_Q0 0x01
#define SR_Q1 0x02
#define SR_Q2 0x04
#define SR_Q3 0x08
#define SR_Q4 0x10
#define SR_Q5 0x20
#define SR_Q6 0x40
#define SR_Q7 0x80

#define ON_LED SR_Q0

#define MOTOR1_FORWARD SR_Q1
#define MOTOR1_REVERSE SR_Q2
#define MOTOR1_ON SR_Q5

#define MOTOR2_FORWARD SR_Q3
#define MOTOR2_REVERSE SR_Q4
#define MOTOR2_ON SR_Q6

const uint8_t MOTOR_STOP = 0x00;
const uint8_t MOTOR_FORWARD = 0x82;
const uint8_t MOTOR_REVERSE = 0x84;

SimpleShiftRegister shiftreg(DATA_PIN, LATCH_PIN, CLOCK_PIN);

void setup() {


  // Setup Serial Monitor
  Serial.begin(115200);
  Serial.println("Welcome.");

  pinMode(MOTOR_ENABLE_PIN, OUTPUT);

  shiftreg.setup();
  shiftreg.setMask(ON_LED);

  delay(500);
}

uint8_t speed = 100;

void loop() {
  if (Serial.available()) {
    char someChar = Serial.read();
     
    if (someChar == 'F') {
      analogWrite(MOTOR_ENABLE_PIN, speed);
      shiftreg.setMask(ON_LED | MOTOR1_FORWARD | MOTOR1_ON | MOTOR2_FORWARD | MOTOR2_ON);
      shiftreg.sendData();
    } else if (someChar == 'R') {
      analogWrite(MOTOR_ENABLE_PIN, speed);
      shiftreg.setMask(ON_LED | MOTOR1_REVERSE | MOTOR1_ON | MOTOR2_REVERSE | MOTOR2_ON);
      shiftreg.sendData();
    } else if (someChar == 'l') {
      analogWrite(MOTOR_ENABLE_PIN, speed);
      shiftreg.setMask(ON_LED | MOTOR1_FORWARD | MOTOR1_ON);
      shiftreg.sendData();
    } else if (someChar == 'r') {
      analogWrite(MOTOR_ENABLE_PIN, speed);
      shiftreg.setMask(ON_LED | MOTOR2_FORWARD | MOTOR2_ON);
      shiftreg.sendData();
    } else if (someChar == 'S') {
      analogWrite(MOTOR_ENABLE_PIN, 0);
      shiftreg.setMask(ON_LED);
      shiftreg.sendData();
    } else if (someChar == '+') {
      if (speed > 245) {
        speed = 255;
      } else {
        speed += 10;
      }
      analogWrite(MOTOR_ENABLE_PIN, speed);
    } else if (someChar == '-') {
      if (speed < 10) {
        speed = 0;
      } else {
        speed -= 10;
      }
      analogWrite(MOTOR_ENABLE_PIN, speed);
    }
    Serial.print("Register: ");
    Serial.println(shiftreg.getMask(), BIN);
    Serial.print("Speed: ");
    Serial.println(speed);
  }

  delay(500);
}
