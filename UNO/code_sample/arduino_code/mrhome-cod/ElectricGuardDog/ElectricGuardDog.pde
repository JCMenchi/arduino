//START:includes
#include <avr/pgmspace.h>
#include "util.h"
#include "MediaPlayer.h"
#include <ServoTimer2.h> 
//END:includes 

//START:variables
int ledPin        = 13;  // on-board LED
int inputPin      = 12;  // input pin for the PIR sensor
int pirStatus     = LOW; // set to LOW (no motion detected)
int pirValue      = 0;   // variable for reading inputPin status
int servoposition = 0;   // starting position of the servo
//END:variables

//START:objects
ServoTimer2 theservo;    // create servo object from the ServoTimer2 class
MediaPlayer mediaPlayer; // create mediaplayer object from the MediaPlayer class
//END:objects

//START:setup
void setup() {
  pinMode(ledPin, OUTPUT);   // set pinMode of the onboard LED to OUTPUT
  pinMode(inputPin, INPUT);  // set the PIR inputPin and listen to it as INPUT
  theservo.attach(7);        // attach the servo motor digital output to pin 7
  randomSeed(analogRead(0)); // seed the Arduino random number generator
   Serial.begin(9600);
}
//END:setup
 
//START:main_loop
void loop(){
  pirValue = digitalRead(inputPin); // poll the value of the PIR
  if (pirValue == HIGH) {           // If motion is detected
    digitalWrite(ledPin, HIGH);  	// turn the onboard LED on
    if (pirStatus == LOW) {			// Trigger motion
      Serial.println("Motion detected");
      
      // Generate a random number between 1 and 5 to match file names
	  // and playback the file and move the servo varying degrees
      switch (random(1,6)) {
        case 1: 
          Serial.println("Playing back 1.WAV");
          theservo.write(1250);
          mediaPlayer.play("1.WAV");
          break;
        case 2: 
          Serial.println("Playing back 2.WAV");
          theservo.write(1400);
          mediaPlayer.play("2.WAV");
          break;
        case 3: 
          Serial.println("Playing back 3.WAV");
          theservo.write(1600);
          mediaPlayer.play("3.WAV");
          break;
        case 4: 
          Serial.println("Playing back 4.WAV");
          theservo.write(1850);
          mediaPlayer.play("4.WAV");
          break;
        case 5: 
          Serial.println("Playing back 5.WAV");
          theservo.write(2100);
          mediaPlayer.play("5.WAV");
          break;
      }        

      delay(1000);          // wait a second
      theservo.write(1000); // return the servo to the start position
      pirStatus = HIGH;     // set the pirStatus flag to HIGH to stop
                            // repeating motion
    }
  } else {
    digitalWrite(ledPin, LOW); // turn the onboard LED off
    if (pirStatus == HIGH){
      Serial.println("No motion");
      mediaPlayer.stop();
      pirStatus = LOW;      // set the pirStatus flag to LOW to 
                            // prepare it for a motion event
    }
  }
}
//END:main_loop