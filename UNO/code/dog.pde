#include <IRremote.h>
#include <IRremoteInt.h>
// PIN
int IR_RECV_PIN = 11;
int IR_SEND_PIN = 10;

int LED_RED1_PIN = 8;
int LED_RED2_PIN = 6;
int LED_YEL1_PIN = 7;
int LED_YEL2_PIN = 5;

int IC_DATA_PIN = 2;
int IC_CLK_PIN = 9;
int IC_LATCH_PIN = 4;

// IR
IRrecv irrecv(IR_RECV_PIN);
decode_results results;

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

// State
byte isOn = 0;
int lastValue = 0;
unsigned long lastValueTime = 0;
unsigned long sendCodeTime = 0;
byte isDumping = 0;
int curMotorConf = 0;
byte rightMotorState = 0;
byte leftMotorState = 0;
byte sendStep = 0;

void setup()
{
    Serial.begin(19600);
    
    pinMode(IR_SEND_PIN, OUTPUT); 
    
    pinMode(LED_RED1_PIN, OUTPUT); 
    pinMode(LED_RED2_PIN, OUTPUT);   
    pinMode(LED_YEL1_PIN, OUTPUT);   
    pinMode(LED_YEL2_PIN, OUTPUT); 

    pinMode(IC_DATA_PIN, OUTPUT);   
    pinMode(IC_CLK_PIN, OUTPUT);   
    pinMode(IC_LATCH_PIN, OUTPUT);   
   
    // set shift register to 0
    digitalWrite(IC_LATCH_PIN, LOW);
    shiftOut(IC_DATA_PIN, IC_CLK_PIN, MSBFIRST, curMotorConf);
    digitalWrite(IC_LATCH_PIN, HIGH);
    
    // Start the receiver
    irrecv.enableIRIn();
    
    // blink RED1 to say we are ready
    digitalWrite(LED_RED1_PIN, HIGH);
    delay(500);
    digitalWrite(LED_RED1_PIN, LOW);
    delay(500);
    digitalWrite(LED_RED1_PIN, HIGH);
    delay(500);
    digitalWrite(LED_RED1_PIN, LOW);
}

// Dumps out the decode_results structure.
// Call this after IRrecv::decode()
// void * to work around compiler issue
//void dump(void *v) {
//  decode_results *results = (decode_results *)v
void dump(decode_results *results) {
    int count = results->rawlen;
    if (results->decode_type == UNKNOWN) {
        Serial.println("Could not decode message");
        Serial.print("Raw (");
        Serial.print(count, DEC);
        Serial.print("): ");

        for (int i = 0; i < count; i++) {
            if ((i % 2) == 1) {
                Serial.print(results->rawbuf[i]*USECPERTICK, DEC);
            } else {
                Serial.print(-(int)results->rawbuf[i]*USECPERTICK, DEC);
            }
            Serial.print(" ");
        } 
    } else {
        if (results->decode_type == NEC) {
            Serial.print("Decoded NEC: ");
        } else if (results->decode_type == SONY) {
            Serial.print("Decoded SONY: ");
        } else if (results->decode_type == RC5) {
            Serial.print("Decoded RC5: ");
        } else if (results->decode_type == RC6) {
            Serial.print("Decoded RC6: ");
        }
        Serial.print(results->value, HEX);
        Serial.print(" (");
        Serial.print(results->bits, DEC);
        Serial.println(" bits)");
    }

    Serial.println("");
}

void calculateMotorState() {
    int newState = 0;
    
    if (rightMotorState == 1) {
        newState = 1 + 2;
    } else if (rightMotorState == 255) {
        newState = 4 + 8;
    }
    
    if (leftMotorState == 1) {
        newState += 16 + 32;
    } else if (leftMotorState == 255) {
        newState += 64 + 128;
    }
    
    if (newState != curMotorConf) {
        curMotorConf = newState;
        digitalWrite(IC_LATCH_PIN, LOW);
        shiftOut(IC_DATA_PIN, IC_CLK_PIN, MSBFIRST, curMotorConf);
        digitalWrite(IC_LATCH_PIN, HIGH);
    }
        
}

void execCommand(int cmd) {
    if (results.value == IR_PLAY) {
        
        rightMotorState = 1;
        leftMotorState = 1;
    
    } else if (results.value == IR_STOP) {
        
        rightMotorState = 0;
        leftMotorState = 0;
        
    } else if (results.value == IR_VOLPLUS) {
        
        rightMotorState = 1;
    
    } else if (results.value == IR_VOLMINUS) {
        
        rightMotorState = 255;
    
    } else if (results.value == IR_PREPLUS) {
        
        leftMotorState = 1;
        
    } else if (results.value == IR_PREMINUS) {
        
        leftMotorState = 255;
        
    }
  
    calculateMotorState();
    
    if (results.value == IR_SLEEP) {
        sendCodeTime = micros() + 20000;
        Serial.print(" send TAPE code in ");
        Serial.println(sendCodeTime, DEC);
        digitalWrite(LED_RED2_PIN, HIGH);
    }
    
    if (results.value == IR_TAPE) {
        digitalWrite(LED_RED2_PIN, HIGH);
    }
    
    if (results.value == IR_CD) {
        digitalWrite(LED_RED2_PIN, LOW);
        sendCodeTime = 0;
    }
}

void showState(int curValueTime, int butvalue) {
    Serial.print("curValueTime= ");
    Serial.print(curValueTime);
    Serial.print(" lastValueTime= ");
    Serial.print(lastValueTime);
    Serial.print(" current Button= ");
    Serial.print(butvalue);
    Serial.print(" last Button= ");
    Serial.println(lastValue, HEX);
    Serial.print(" Left Motor = ");
    Serial.println(leftMotorState, DEC);
    Serial.print(" Right Motor = ");
    Serial.println(rightMotorState, DEC);
    Serial.print(" Motor Conf = ");
    Serial.println(curMotorConf);
}

int sendIR(int triggerPin, unsigned long t )
{
  int halfPeriod = 12; //one period at 40khZ is  25 microseconds
  int cycles = 24; //25 microseconds * 24 is more or less 600 microsecond
  
  cycles = 24 + sendStep%2 * 24;
  t += 600;
  if (sendStep == 0) {
    cycles = 100;
    sendStep = 1;
  } else if (sendStep == 12) {
    sendStep = 0;
    t += 1000000;
  }
  
  int i;
  for (i=0; i <=cycles; i++)
  {
    digitalWrite(triggerPin, HIGH); 
    delayMicroseconds(halfPeriod);
    digitalWrite(triggerPin, LOW); 
    delayMicroseconds(halfPeriod); 
  }
  
  return t;
}

void loop() {
    unsigned long curValueTime = millis();
    if (irrecv.decode(&results)) {
        dump(&results);

        if (results.decode_type == SONY) {
            
            if (results.value == IR_SHIFT && lastValue != IR_SHIFT) {
                isDumping = !isDumping;
            }

            if (curValueTime - lastValueTime > 2000) {
                lastValue = 0;
            }

            if (isDumping) {
                showState(curValueTime, results.value);
                dump(&results);
            } 

            if (results.value == IR_POWER && lastValue != results.value) {
                if (isOn == 1) {
                    isOn = 0;
                    digitalWrite(LED_RED1_PIN, LOW);
                } else {
                    isOn = 1;
                    digitalWrite(LED_RED1_PIN, HIGH);
                }
                lastValueTime = curValueTime;
            }

            if (isOn) {
                execCommand(results.value);
            }
            lastValue = results.value;
        }

        irrecv.resume(); // Receive the next value
    }
    unsigned long t = micros();
    
    if (sendCodeTime != 0 && t >= sendCodeTime) {
        
        // send TAPE code
        //Serial.print("send code...");
        sendCodeTime = sendIR(IR_SEND_PIN, t);
        
        //Serial.print("done. next ");
        //Serial.println(sendCodeTime, DEC);
         
    }
}



