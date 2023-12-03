void setup()
{
  Serial.begin(9600);
  pinMode(13, OUTPUT); 
  digitalWrite(13, HIGH);
}

char r;

void loop()
{
  if (Serial.available() > 0) {
    r = Serial.read();
    if (r == 'H') {
      digitalWrite(13, HIGH);
      Serial.print("  13 ON "); 
    }
    if (r == 'L') {
      digitalWrite(13, LOW);
      Serial.print(" 13 OFF "); 
    }
  }
}
