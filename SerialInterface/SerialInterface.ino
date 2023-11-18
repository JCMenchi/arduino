/*
  Test serial interface
 */

void setup() {                

  Serial.begin(9600);
  Serial.println("hello");

  pinMode(13, OUTPUT);
  digitalWrite(13, LOW);
}

void loop() {

  if (Serial.available()) {
    // read the most recent byte (which will be from 0 to 255):
    int brightness = Serial.read();
    Serial.println(brightness);
    if (brightness == '1') {
      Serial.println("on");
      digitalWrite(13, HIGH);
    } else if (brightness == '0') {
      Serial.println("off");
      digitalWrite(13, LOW);
    }
  }
}
      
