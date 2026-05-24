/*
  Blink
  Turns on an LED on for one second, then off for one second, repeatedly.
 
  This example code is in the public domain.
 */

byte on12;

byte v9;
byte v8;

void setup() {                
  // initialize the digital pin as an output.
  // Pin 13 has an LED connected on most Arduino boards:
  Serial.begin(9600);
  pinMode(2, OUTPUT);
  pinMode(3, OUTPUT);
  pinMode(4, OUTPUT);  
  pinMode(5, OUTPUT);
  pinMode(6, OUTPUT);  
  pinMode(7, OUTPUT);
  pinMode(8, OUTPUT);
  pinMode(9, OUTPUT);  
  pinMode(10, OUTPUT);
  pinMode(13, OUTPUT);
  Serial.println("hello");
  on12 = 0;
  v9 = 0;
  v8 = 125;
}

void loop() {
  byte brightness;
 
  analogWrite(8, v8);
  analogWrite(9, v9);
  //~ digitalWrite(10, LOW);
  digitalWrite(13, HIGH);   // set the LED on
  //~ if (on12 == 1) {
    //~ digitalWrite(12, HIGH);
  //~ } else {
    //~ digitalWrite(12, LOW);
  //~ }
  delay(400);              // wait for a second
  //~ digitalWrite(10, HIGH);
  digitalWrite(13, LOW);    // set the LED off
  //~ if (on12 == 1) {
    //~ digitalWrite(12, LOW);
  //~ } else {
    //~ digitalWrite(12, HIGH);
  //~ }
  delay(200);              // wait for a second
  
  if (Serial.available()) {
    // read the most recent byte (which will be from 0 to 255):
    brightness = Serial.read();
    Serial.println(brightness);
    if (brightness == '1') {
      on12 = 1;
      Serial.println("on");
    } else if (brightness == '2') {
      Serial.println("off");
      on12 = 0;
    } else if (brightness == '+') {
      v9 = (v9+10) % 256;
      v8 = (v8+10) % 256;  
      Serial.print(v9, DEC);
      Serial.print(" ");
      Serial.println(v8, DEC);
    } else if (brightness == '-') {
        v9 = (v9-10) % 256;
      v8 = (v8-10) % 256;  
        Serial.print(v9, DEC);
      Serial.print(" ");
      Serial.println(v8, DEC);
    }
  }
}
      
