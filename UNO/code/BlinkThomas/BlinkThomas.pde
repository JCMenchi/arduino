/*
  Blink
  Turns on an LED on for one second, then off for one second, repeatedly.
 
  This example code is in the public domain.
 */

byte on12;
 
void setup() {                
  // initialize the digital pin as an output.
  // Pin 13 has an LED connected on most Arduino boards:
  Serial.begin(9600);
  pinMode(13, OUTPUT);
  pinMode(12, OUTPUT);
  pinMode(11,OUTPUT);
  pinMode(10, OUTPUT);
  pinMode(9, OUTPUT);
  pinMode(8, OUTPUT);
  pinMode(7,OUTPUT);
  pinMode(6, OUTPUT);
  pinMode(5, OUTPUT);
  pinMode(4, OUTPUT);
  pinMode(3, OUTPUT);
  pinMode(2, OUTPUT); 
  on12 = 0;
}

void loop() {
  byte brightness;
 
  
   
 
  digitalWrite(8,HIGH);
  digitalWrite(9, LOW);
  digitalWrite(10,HIGH);
  digitalWrite(11,LOW);  
  digitalWrite(12,HIGH);
  digitalWrite(13,LOW);  
  delay(500);
  
   
  
  digitalWrite(8,LOW);
  digitalWrite(9, HIGH);
  digitalWrite(10,LOW);
  digitalWrite(11,HIGH);  
  digitalWrite(12,LOW);
  digitalWrite(13,HIGH);  
  delay(500);
  
} 
