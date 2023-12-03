// <callout id="code.curtainautomation.includes"/>
#include <AFMotor.h>

//START:defines
// <callout id="code.curtainautomation.defines"/>
  #define LIGHT_PIN         0
  #define LIGHT_THRESHOLD 800
  #define TEMP_PIN          5
  #define TEMP_THRESHOLD   72
  #define TEMP_VOLTAGE    5.0
  #define ONBOARD_LED      13
//END:defines

//START:variables
// <callout id="code.curtainautomation.variables"/>
int curtain_state  = 1;
int light_status   = 0;
double temp_status = 0;

boolean daylight   = true;
boolean warm       = false;

AF_Stepper motor(100, 2);
//END: variables

//START:setup
// <callout id="code.curtainautomation.setup"/>
void setup() {
  Serial.begin(9600);
  Serial.println("Setting up Curtain Automation...");
  // Set stepper motor rotation speed to 10 RPM's
  motor.setSpeed(100);
  // Initialize motor
  // motor.step(100, FORWARD, SINGLE); 
  // motor.release();
  delay(1000);
}
//END:setup

//START:curtain
// <callout id="code.curtainautomation.curtain"/>
void Curtain(boolean curtain_state) {
  digitalWrite(ONBOARD_LED, curtain_state ? HIGH : LOW);
  if (curtain_state) {
    Serial.println("Opening curtain...");
    // Try SINGLE, DOUBLE, INTERLEAVE or MICROSTOP
    motor.step(800, FORWARD, SINGLE);
  } else {
    Serial.println("Closing curtain...");
    motor.step(800, BACKWARD, SINGLE); 
  }
}
//END:curtain

//START:main_loop
// <callout id="code.curtainautomation.mainloop"/>
void loop() {

  // poll photocell value
  light_status = analogRead(LIGHT_PIN);
  delay(500);

  // print light_status value to the serial port
  Serial.print("Photocell value = ");
  Serial.println(light_status);
  Serial.println("");

  // poll temperature
  int temp_reading = analogRead(TEMP_PIN);
  delay(500);
  
  // convert voltage to temp in Celsius and Fahrenheit
  float voltage = temp_reading * TEMP_VOLTAGE / 1024.0;  
  float temp_Celsius = (voltage - 0.5) * 100 ;
  float temp_Fahrenheit = (temp_Celsius * 9 / 5) + 32;
  // print temp_status value to the serial port
  Serial.print("Temperature value (Celsius) = ");
  Serial.println(temp_Celsius);
  Serial.print("Temperature value (Fahrenheit) = ");
  Serial.println(temp_Fahrenheit);
  Serial.println("");
  
  if (light_status > LIGHT_THRESHOLD)
      daylight = true;
  else
      daylight = false; 
    
  if (temp_Fahrenheit > TEMP_THRESHOLD)
      warm = true;
  else
      warm = false;

  switch (curtain_state)
  {
  case 0:
      if (daylight && !warm)
      // open curtain
      {     
        curtain_state = 1;
        Curtain(curtain_state);
      }
      break;

  case 1:
      if (!daylight || warm)
      // close curtain
      {
        curtain_state = 0;
        Curtain(curtain_state);
      }
      break;
  }
}
//END:main_loop