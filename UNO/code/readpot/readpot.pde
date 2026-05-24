int sensorPin=0;
int ledPin=9;
int sensorValue=0;

void setup () {
  Serial.begin(9600);
  Serial.println("hello");
  pinMode (ledPin, OUTPUT);
}
void loop () {
  sensorValue = analogRead(sensorPin);
  digitalWrite(ledPin, HIGH);
  delay(sensorValue);
  digitalWrite(ledPin, LOW);
  delay(sensorValue);
  if (Serial.available()) {
    byte r = Serial.read();
    Serial.print("sensorValue = ");
    Serial.println(sensorValue, DEC);
  }
}
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
