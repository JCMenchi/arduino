#include <IRremote.h>
#include <Servo.h>  

// PIN
int RECV_PIN = 11;
int GREEN_PIN = 2;
int RED_PIN = 5;
int SERVO_PIN = 9;

// SERVO
Servo myservo;  // create servo object to control a servo 
int curServoPos = 50;

// IR
IRrecv irrecv(RECV_PIN);
decode_results results;

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
int lastValueTime = 0;
byte isDumping = 0;
void setup()
{
  Serial.begin(19200);
  irrecv.enableIRIn(); // Start the receiver
  myservo.attach(SERVO_PIN);
  pinMode(SERVO_PIN, OUTPUT); 
  pinMode(GREEN_PIN, OUTPUT);   
  pinMode(RED_PIN, OUTPUT);   
  
  digitalWrite(GREEN_PIN, LOW);
  digitalWrite(RED_PIN, LOW);
  myservo.write(curServoPos); 
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
    } 
    else if (results->decode_type == SONY) {
      Serial.print("Decoded SONY: ");
    } 
    else if (results->decode_type == RC5) {
      Serial.print("Decoded RC5: ");
    } 
    else if (results->decode_type == RC6) {
      Serial.print("Decoded RC6: ");
    }
    Serial.print(results->value, HEX);
    Serial.print(" (");
    Serial.print(results->bits, DEC);
    Serial.println(" bits)");
  }
  
  Serial.println("");
}

void execCommand(int cmd) {
  if (results.value == IR_VOLPLUS) {
     curServoPos+=10;
  }
  if (results.value == IR_VOLMINUS) {
     curServoPos-=10;
     
  }
  if (results.value == IR_PREPLUS) {
     curServoPos++;
  }
  if (results.value == IR_PREMINUS) {
     curServoPos--;
  }
  
  if (results.value == IR_NEXT) {
    curServoPos = 179;
  }
  
  if (results.value == IR_PREVIOUS) {
    curServoPos = 6;
  }
  
  if (curServoPos < 6) {
     curServoPos = 6;
  }
  if (curServoPos > 179) {
     curServoPos = 179;
  }
  //if (curServoPos != myservo.read()) {
    myservo.write(curServoPos); 
  //}
  analogWrite(RED_PIN, map(curServoPos, 6, 179, 0, 255));
  
}

void showState(int curValueTime, int butvalue) {
  Serial.print("curValueTime= ");
  Serial.print(curValueTime);
  Serial.print(" lastValueTime= ");
  Serial.print(lastValueTime);
  Serial.print(" current Button= ");
  Serial.print(butvalue);
  Serial.print(" last Button= ");
  Serial.print(lastValue, HEX);
  Serial.print(" servo pos= ");
  Serial.println(curServoPos);
}

void loop() {
  
  if (irrecv.decode(&results)) {
    //Serial.println(results.value, HEX);
    
     //dump(&results);
   
    
    if (results.decode_type == SONY) {
      int curValueTime = millis();
      
      if (results.value == IR_SHIFT && lastValue != IR_SHIFT) {
        isDumping = !isDumping;
      }
      
      if (curValueTime - lastValueTime > 2000) {
        lastValue = 0;
      }
      
     /* digitalWrite(RED_PIN, HIGH);
      delay(50);
      digitalWrite(RED_PIN, LOW);
      delay(50);
      digitalWrite(RED_PIN, HIGH);
      delay(50);
      digitalWrite(RED_PIN, LOW);*/
      
      
      
      if (isDumping) {
        showState(curValueTime, results.value);
        dump(&results);
      } 
      
      if (results.value == IR_POWER && lastValue != results.value) {
        if (isOn == 1) {
          isOn = 0;
          digitalWrite(GREEN_PIN, LOW);
        } else {
          isOn = 1;
          digitalWrite(GREEN_PIN, HIGH);
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
}


