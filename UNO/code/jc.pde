
// constants won't change. They're used here to 
// set pin numbers:
const int buttonPin = 2;     // the number of the pushbutton pin

// variables will change:
int buttonState = 0;         // variable for reading the pushbutton status

void setup() {
  Serial.begin(9600);
  Serial.println("hello");
  // initialize the LED pin as an output:
  pinMode(9, OUTPUT);      
  pinMode(10, OUTPUT);      
  pinMode(11, OUTPUT);      
  // initialize the pushbutton pin as an input:
  pinMode(buttonPin, INPUT);     
}

 byte red = 0;
  byte green = 0;
  byte blue = 0;

void loop(){
  // read the state of the pushbutton value:
  buttonState = digitalRead(buttonPin);
  int pot = analogRead(0);
  int temp = analogRead(1);
 
  green = int(255.0 * pot * 1.0 / 1024.0);
  //~ blue =   int(255.0 * temp * 1.0 / 1024.0);
  // check if the pushbutton is pressed.
  // if it is, the buttonState is HIGH:
  if (buttonState == HIGH) {     
    red =(red + 1)%255;
  }  else {
    red = 0;
  }
  
  digitalWrite(9, red);
  digitalWrite(10, green);
  digitalWrite(11, blue);
  if (Serial.available()) {
    char r = Serial.read();
    blue = r;
    Serial.print(pot, DEC);
    Serial.print(" ");
    Serial.print(temp, DEC);
    Serial.print(" ");
    Serial.print(red, DEC);
    Serial.print(" ");
    Serial.print(green, DEC);
    Serial.print(" ");
    Serial.print(blue, DEC);
    Serial.println(" ");
    
  } 
}