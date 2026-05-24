/* Melody
 * (cleft) 2005 D. Cuartielles for K3
 *
 * This example uses a piezo speaker to play melodies.  It sends
 * a square wave of the appropriate frequency to the piezo, generating
 * the corresponding tone.
 *
 * The calculation of the tones is made following the mathematical
 * operation:
 *
 *       timeHigh = period / 2 = 1 / (2 * toneFrequency)
 *
 * where the different tones are described as in the table:
 *
 * note 	frequency 	period 	timeHigh
 * c 	        261 Hz 	        3830 	1915 	
 * d 	        294 Hz 	        3400 	1700 	
 * e 	        329 Hz 	        3038 	1519 	
 * f 	        349 Hz 	        2864 	1432 	
 * g 	        392 Hz 	        2550 	1275 	
 * a 	        440 Hz 	        2272 	1136 	
 * b 	        493 Hz 	        2028	1014	
 * C	        523 Hz	        1912 	956
 *
 * http://www.arduino.cc/en/Tutorial/Melody
 */
  
int speakerPin = 10;
int sensorPin=0;
int ledPin=9;
int sensorValue=0;

int length = 15; // the number of notes
char notes[] = "ccggaagffeeddc "; // a space represents a rest
int beats[] = { 1, 1, 1, 1, 1, 1, 2, 1, 1, 1, 1, 1, 1, 2, 4 };
int tempo = 300;

void playTone(int tone, int duration) {
  for (long i = 0; i < duration * 1000L; i += tone * 2) {
    digitalWrite(speakerPin, HIGH);
    delayMicroseconds(tone);
    digitalWrite(speakerPin, LOW);
    delayMicroseconds(tone);
  }
}

void playNote(char note, int duration) {
  char names[] = { 'c', 'd', 'e', 'f', 'g', 'a', 'b', 'C' };
  int tones[] = { 1915, 1700, 1519, 1432, 1275, 1136, 1014, 956 };
  
  // play the tone corresponding to the note name
  for (int i = 0; i < 8; i++) {
    if (names[i] == note) {
      playTone(tones[i], duration);
    }
  }
}

char getnote(char r, int& beat) {
  if (r == 'w') {
    beat = 1;
    return 'a';
  } else if (r == 'x') {
    beat = 1;
    return 'b';
   } else if (r == 'c') {
    beat = 1;
    return 'c'; 
   } else if (r == 'v') {
    beat = 1;
    return 'd'; 
   } else if (r == 'b') {
    beat = 1;
    return 'e';
   } else if (r == 'n') {
    beat = 1;
    return 'f'; 
   } else if (r == ',') {
    beat = 1;
    return 'g';
   } else if (r == 'W') {
    beat = 2;
    return 'a';
  } else if (r == 'X') {
    beat = 2;
    return 'b';
   } else if (r == 'C') {
    beat = 2;
    return 'c'; 
   } else if (r == 'V') {
    beat = 2;
    return 'd'; 
   } else if (r == 'B') {
    beat = 2;
    return 'e';
   } else if (r == 'N') {
    beat = 2;
    return 'f'; 
   } else if (r == '?') {
    beat = 2;
    return 'g';
   }
   beat = 4;
   return ' ';
}

void setup() {
  Serial.begin(9600);
  Serial.println("hello");
  pinMode (ledPin, OUTPUT);
  pinMode(speakerPin, OUTPUT);
}

void loop() {
  char mynote = ' ';
  int curbeat = 1;
  tempo = analogRead(sensorPin);
  if (Serial.available()) {
    char r = Serial.read();
    mynote = getnote(r, curbeat);
    Serial.println(mynote);
  } 
  
  if (mynote == ' ') {
    delay(curbeat * tempo); // rest
  } else {
    playNote(mynote, curbeat * tempo);
  }
  // pause between notes
  delay(tempo / 2); 
  analogWrite(ledPin, tempo%255);

}
