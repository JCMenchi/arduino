
// for infra red remote
#include <IRremoteJC.h>
#include <IRremoteIntJC.h>
int IR_RECV_PIN = 11;
int IR_SEND_PIN = 10;



// SONY remote code
#define IR_POWER    0xA81
#define IR_SLEEP    0x61
#define IR_CD       0xA41
#define IR_PLAY     0x4D1
#define IR_PAUSE    0x9D1
#define IR_STOP     0x1D1
#define IR_EJECT    0x691
#define IR_PREVIOUS 0xD1
#define IR_NEXT     0x8D1
#define IR_TAPE     0xC41

#define IR_TUNER    0x841
#define IR_SHIFT    0xCD6
#define IR_PHONO    0x41
#define IR_VIDAUX   0x441
#define IR_VOLPLUS  0x481
#define IR_VOLMINUS 0xC81
#define IR_PREPLUS  0x96
#define IR_PREMINUS 0x896

IRrecvJC irrecv(IR_RECV_PIN);
decode_results results;

// State
int lastValue = 0;
unsigned long sendCodeTime = 0;
byte isDumping = 0;
byte sendStep = 0;

void setupIRfunc()
{

  pinMode(IR_SEND_PIN, OUTPUT); 
  pinMode(13, OUTPUT);
  
  // Start the receiver
  irrecv.enableIRIn();
  irrecv.blink13(1);
}

// Dumps out the decode_results structure.
// Call this after IRrecv::decode()
// void * to work around compiler issue
//void dump(void *v) {
//  decode_results *results = (decode_results *)v
void dumpIR(decode_results *results) {
  int count = results->rawlen;
  if (results->decode_type == UNKNOWN) {
    Serial.println("Could not decode message");
    Serial.print("Raw (");
    Serial.print(count, DEC);
    Serial.print("): ");

    for (int i = 0; i < count; i++) {
      if ((i % 2) == 1) {
        Serial.print(results->rawbuf[i]*USECPERTICK, DEC);
      } 
      else {
        Serial.print(-(int)results->rawbuf[i]*USECPERTICK, DEC);
      }
      Serial.print(" ");
    } 
  } 
  else {
    if (results->decode_type == SONY) {
      Serial.print("Decoded SONY: ");
    } 
    
    Serial.print(results->value, HEX);
    Serial.print(" (");
    Serial.print(results->bits, DEC);
    Serial.println(" bits)");
  }

  Serial.println("");
}

void execIRCommand(byte& rightMotorState, byte& leftMotorState, decode_results *results) {
  
  if (results->value == IR_PLAY) {
    rightMotorState = 1;
    leftMotorState = 1;
  } else if (results->value == IR_STOP) {
    rightMotorState = 0;
    leftMotorState = 0;
  } else if (results->value == IR_VOLPLUS) {
    rightMotorState = 1;
  } else if (results->value == IR_VOLMINUS) {
    rightMotorState = 255;
  } else if (results->value == IR_PREPLUS) {
    leftMotorState = 1;
  } else if (results->value == IR_PREMINUS) {
    leftMotorState = 255;
  }

  calculateMotorState();

  if (results->value == IR_SLEEP) {
    sendCodeTime = micros() + 20000;
    Serial.print(" send TAPE code in ");
    Serial.println(sendCodeTime, DEC);
    digitalWrite(LED_RED1_PIN, HIGH);
  }

  if (results->value == IR_TAPE) {
    digitalWrite(LED_RED1_PIN, HIGH);
  }

  if (results->value == IR_CD) {
    digitalWrite(LED_RED1_PIN, LOW);
    sendCodeTime = 0;
  }
}

void showIRState(int curValueTime, char butvalue) {
  Serial.print("curValueTime= ");
  Serial.print(curValueTime);
  Serial.print(" lastValueTime= ");
  Serial.print(lastValueTime);
  Serial.print(" current Button= ");
  Serial.print(butvalue);
  Serial.print(" last Button= ");
  Serial.println(lastValue, HEX);
}

unsigned long sendIR(int triggerPin)
{
  int halfPeriod = 12; //one period at 40khZ is  25 microseconds
  int cycles = 24; //25 microseconds * 24 is more or less 600 microsecond
  unsigned long t = 600;

  cycles = 13 + sendStep%2 * 13;

  if (sendStep == 0) {
    cycles = 52;
  } 
  else if (sendStep == 12) {
    t = 1000000;
  }

  //Serial.print("send step ");
  //Serial.print(sendStep, DEC);
  //Serial.print(" nb cycles ");
  //Serial.println(cycles);
  int i;
  for (i=0; i <cycles; ++i)
  {
    digitalWrite(triggerPin, HIGH); 
    delayMicroseconds(halfPeriod);
    digitalWrite(triggerPin, LOW); 
    delayMicroseconds(halfPeriod); 
  }

  sendStep++;
  if (sendStep == 13) {
    sendStep = 0;  
  }

  return micros() + t;
}

void loopIR() {
  unsigned long curValueTime = millis();
  if (irrecv.decode(&results)) {
    dumpIR(&results);

    if (results.decode_type == SONY) {

      if (results.value == IR_SHIFT && lastValue != IR_SHIFT) {
        isDumping = !isDumping;
      }

      if (curValueTime - lastValueTime > 2000) {
        lastValue = 0;
      }

      if (isDumping) {
        showIRState(curValueTime, results.value);
        dumpIR(&results);
      } 

      if (results.value == IR_POWER && lastValue != results.value) {
        if (isOn == 1) {
          isOn = 0;
          digitalWrite(LED_RED1_PIN, LOW);
        } 
        else {
          isOn = 1;
          digitalWrite(LED_RED1_PIN, HIGH);
        }
        lastValueTime = curValueTime;
      }

      if (isOn) {
        execIRCommand(results.value);
      }
      lastValue = results.value;
    }

    irrecv.resume(); // Receive the next value
  }
  unsigned long t = micros();

  if (sendCodeTime != 0 && t >= sendCodeTime) {

    // send TAPE code
    //Serial.print("send code...");
    sendCodeTime = sendIR(IR_SEND_PIN);

    //Serial.print("done. next ");
    //Serial.println(sendCodeTime, DEC);

  }
}
