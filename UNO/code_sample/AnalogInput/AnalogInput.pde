
int gpuMax = 0; 
int gpuMin = 2000; 
int simpMax = 0; 
int simpMin = 2000; 
int compMax = 0; 
int compMin = 2000; 
unsigned long t = 0;
unsigned long gpuSum = 0;
unsigned long gpuNb = 0;
unsigned long simpSum = 0;
unsigned long simpNb = 0;
unsigned long compSum = 0;
unsigned long compNb = 0;

void setup() {
  Serial.begin(19600);
 
}

void loop() {
  
  int sensorValue = analogRead(A0);
  if (sensorValue > gpuMax) { gpuMax = sensorValue;}
  if (sensorValue < gpuMin) { gpuMin = sensorValue;}
  gpuNb++;
  gpuSum += sensorValue;
  
  sensorValue = analogRead(A2);
  if (sensorValue > simpMax) { simpMax = sensorValue;}
  if (sensorValue < simpMin) { simpMin = sensorValue;}
  simpNb++;
  simpSum += sensorValue;
  
  sensorValue = analogRead(A4);
  if (sensorValue > compMax) { compMax = sensorValue;}
  if (sensorValue < compMin) { compMin = sensorValue;}
  compNb++;
  compSum += sensorValue;
  
  if (Serial.available() > 0) {
    int inByte = Serial.read();
    Serial.print("Value GPU: Min=");
    Serial.print(gpuMin, DEC);
    Serial.print(" Max=");
    Serial.print(gpuMax, DEC);
    Serial.print(" Mean=");
    Serial.print(gpuSum/gpuNb, DEC);
    
    Serial.print(" SIMP: Min=");
    Serial.print(simpMin, DEC);
    Serial.print(" Max=");
    Serial.print(simpMax, DEC);
    Serial.print(" Mean=");
    Serial.print(simpSum/simpNb, DEC);
    
    Serial.print(" COMP: Min=");
    Serial.print(compMin, DEC);
    Serial.print(" Max=");
    Serial.print(compMax, DEC);
    Serial.print(" Mean=");
    Serial.println(compSum/compNb, DEC);
  }       
  delay(200);  
}
