#include <Arduino.h>
#include <RCSwitch.h>

//#include "tune.h"

uint8_t debug = 0;

const int SOUND_PIN = 32;      // PA6 leg 33 on chip
const int LED_PIN = 33;        // PA7 leg 32 on chip
const uint8_t BUTTON_PIN = 16; // PC0 leg 21
int ledState = LOW;            // ledState used to set the LED

unsigned long previousMillis = 0; // will store last time LED was updated

const long interval = 1000; // interval at which to blink (milliseconds)

#include <NewPing.h>
#include <VirtualWire.h>
                                                                                                                                                                                             
#define TRIGGER_PIN  30  // Arduino pin tied to trigger pin on the ultrasonic sensor.
#define ECHO_PIN     31  // Arduino pin tied to echo pin on the ultrasonic sensor.
#define MAX_DISTANCE 250 // Maximum distance we want to ping for (in centimeters). Maximum sensor distance is rated at 400-500cm.

NewPing sonar(TRIGGER_PIN, ECHO_PIN, MAX_DISTANCE); // NewPing setup of pins and maximum distance.


// Radio RF433
#define TxPin 28

/*
void setup()
{
  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, LOW);

  pinMode(BUTTON_PIN, INPUT_PULLUP);

  pinMode(SOUND_PIN, OUTPUT);
  Serial.begin(9600);

  // Initialise the IO and ISR
  //pinMode(TxPin, OUTPUT);
  vw_set_tx_pin(TxPin);
  //vw_set_ptt_inverted(true); // Required for DR3100
  vw_setup(2000);  // Bits per sec
}

void loop()
{
  unsigned long currentMillis = millis();

  uint8_t v = digitalRead(BUTTON_PIN);
  // Serial.printf("Button %d\n", v);

  if (v == 0)
  {
    ledState = HIGH;
  }
  else if (currentMillis - previousMillis >= interval)
  {
    // save the last time you blinked the LED
    previousMillis = currentMillis;

    // if the LED is off turn it on and vice-versa:
    if (ledState == LOW)
    {
      ledState = HIGH;
    }
    else
    {
      ledState = LOW;
    }
  }

  // set the LED with the ledState of the variable:
  digitalWrite(LED_PIN, ledState);

  long cm = sonar.ping_cm(); // Send ping, get distance in cm and print result (0 = outside set distance range)
  Serial.printf("Sensor dist: %ld cm\n", cm);

  //tone(SOUND_PIN, cm * NOTE_DS8 / MAX_DISTANCE, 50);
  delay(500);

  char msg[20];
  snprintf(msg, 20, "%ldcm", cm);
  Serial.printf("Send msg: %s\n", msg);
  vw_send((uint8_t *)msg, strlen(msg));
  //vw_wait_tx(); // Wait until the whole message is gone
  //noTone(SOUND_PIN);
  delay(500);
}

void processCommand(int cmd)
{
  if (cmd == 'D')
  {
    debug = !debug;
    if (debug)
    {
      Serial.println("Activate debugging traces.");
    }
    else
    {
      Serial.println("Disable debugging traces.");
    }
  }
  else if (cmd == 'P')
  {
    //play(SOUND_PIN);
  }
}

void serialEvent()
{
  while (Serial.available())
  {
    // read the most recent byte (which will be from 0 to 255):
    int cmd = Serial.read();
    processCommand(cmd);
  }
}
*/

void setup()
{
    Serial.begin(9600);	  // Debugging only
    Serial.println("setup");

    pinMode(TxPin, OUTPUT);
    vw_set_tx_pin(TxPin);
    // Initialise the IO and ISR
    vw_setup(500);	 // Bits per sec
}

uint8_t count = 0;

void loop()
{
    char msg[64];
    long cm = sonar.ping_cm();

    snprintf(msg, 64, "%d: %d", count, cm);
    digitalWrite(LED_PIN, true); // Flash a light to show transmitting
    Serial.print("send: ");
    Serial.println(msg);
    vw_send((uint8_t *)msg, strlen(msg));
    vw_wait_tx(); // Wait until the whole message is gone
    delay(2000);
    count++;
}

/*

RCSwitch mySwitch = RCSwitch();

void setup() {

  Serial.begin(9600);
  
  // Transmitter is connected to Arduino Pin #10  
  mySwitch.enableTransmit(TxPin);
  
  // Optional set protocol (default is 1, will work for most outlets)
  // mySwitch.setProtocol(2);

  // Optional set pulse length.
  // mySwitch.setPulseLength(320);
  
  // Optional set number of transmission repetitions.
  // mySwitch.setRepeatTransmit(15);
  
}

void loop() {
  Serial.println("Send switch on");
  mySwitch.switchOn("11111", "00010");
  delay(1000);
  Serial.println("Send switch off");
  mySwitch.switchOff("11111", "00010");
  delay(1000);

  delay(2000);
}

*/
