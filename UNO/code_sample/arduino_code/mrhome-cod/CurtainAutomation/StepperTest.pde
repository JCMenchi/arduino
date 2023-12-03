#include <AFMotor.h>

AF_Stepper motor(48, 2);

void setup() {
  Serial.begin(9600);
  Serial.println("Starting stepper motor test...");
  // Use setSpeed to alter speed of rotation
  motor.setSpeed(20);
}

void loop() {
  // step() function 
  motor.step(100, FORWARD, DOUBLE); 
  motor.step(100, BACKWARD, DOUBLE);

}