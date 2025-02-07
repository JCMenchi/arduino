#include <Arduino.h>

#include "ST7735.h"
#include "SPIManager.h"

//#include <Servo.h>

// Rotary Encoder Inputs
#define CLK 5
#define DT 7
#define SW 6

int counter = 0;
int currentStateCLK;
int currentStateButton;
int lastStateButton;
int lastStateCLK;
String currentDir = "";
unsigned long lastButtonPress = 0;

// Servo servo;

#define TFT_CS 10
#define TFT_DC 9  // PORTB 1
#define TFT_RST 8 // PORTB 0

SPIManager spi;
TFT_ST7735 tft = TFT_ST7735(1, 0, &spi);

void setup() {

  // Set encoder pins as inputs
  pinMode(CLK, INPUT);
  pinMode(DT, INPUT);
  pinMode(SW, INPUT_PULLUP);

  // Setup Serial Monitor
  Serial.begin(115200);
  Serial.println("Welcome.");

  // servo.attach(3);
  // servo.write(0);

  // Read the initial state of CLK
  lastStateCLK = digitalRead(CLK);
  lastStateButton = digitalRead(SW);

  tft.init();
  tft.setRotation(3);
  tft.fillRect(0, 0, 160, 128, ST7735_BLACK); // set display to black
  tft.invertDisplay(false);
  tft.enableDisplay(1); // init does not enable display
  Serial.println("TFT init done.");

  uint32_t info = 0;
  info = tft.readcommand32(ST7735_RDDID);
  Serial.println(info, 16);

  info = 0;
  uint16_t info16 = tft.readcommand8(ST7735_RDDCOLMOD);
  Serial.println(info16, HEX);
  Serial.println(info16, BIN);
  info16 = tft.readcommand16(ST7735_RDDIM);
  Serial.println(info16, HEX);
  Serial.println(info16, BIN);
  info16 = tft.readcommand16(ST7735_RDDMADCTL);
  Serial.println(info16, HEX);
  Serial.println(info16, BIN);

  // draw simple form
  tft.startWrite();
  tft.fillRect(10, 20, 30, 4, ST7735_YELLOW);
  tft.fillRect(10, 100, 10, 20, ST7735_RED);
  tft.fillRect(100, 140, 10, 30, ST7735_BLUE);

  tft.drawHLine(0, 0, 160, ST7735_CYAN);
  tft.drawVLine(0, 0, 128, ST7735_CYAN);
  tft.drawVLine(159, 0, 128, ST7735_CYAN);
  tft.drawHLine(0, 127, 160, ST7735_CYAN);

  tft.drawHLine(1, 63, 158, ST7735_WHITE);
  tft.drawVLine(79, 1, 126, ST7735_WHITE);

  tft.drawLine(0, 0, 159, 127, ST7735_RED);

  tft.drawCircle(110, 100, 10, ST7735_WHITE);

  tft.fillCircle(110, 100, 9, ST7735_GREEN);

  tft.fillTriangle(20, 20, 50, 50, 10, 50, ST7735_GREEN);
  tft.drawTriangle(20, 20, 50, 50, 10, 50, ST7735_BLUE);

  tft.fillRoundRect(100, 20, 40, 20, 4, ST7735_YELLOW);
  tft.drawRoundRect(100, 20, 40, 20, 4, ST7735_RED);

  tft.endWrite();

  delay(5000);

  for(uint8_t i = 0; i < 128; ++i) {
    tft.drawPixel(i, i, ST7735_GREEN);
  }

  tft.drawText(10, 64, "BANANA", ST7735_WHITE, ST7735_BLACK, 4);
  tft.drawText(15, 101, "Texte tr�s long\nqui devrait revenir � la ligne", ST7735_BLUE, ST7735_BLUE);

  Serial.println("TFT draw text.");

	delay(500);

}



void loop() {
  /*
// Read the current state of CLK
currentStateCLK = digitalRead(CLK);

// If last and current state of CLK are different, then pulse occurred
// React to only 1 state change to avoid double count
if (currentStateCLK != lastStateCLK && currentStateCLK == 1) {

// If the DT state is different than the CLK state then
// the encoder is rotating CCW so decrement
if (digitalRead(DT) != currentStateCLK) {
counter--;
currentDir = "CCW";
} else {
// Encoder is rotating CW so increment
counter++;
currentDir = "CW";
}

//servo.write(counter * 10);
Serial.print("Direction: ");
Serial.print(currentDir);
Serial.print(" | Counter: ");
Serial.println(counter);
}

// Remember last CLK state
lastStateCLK = currentStateCLK;

// Read the button state
int btnState = digitalRead(SW);

// If we detect LOW signal, button is pressed
if (btnState == LOW) {
// if 50ms have passed since last LOW pulse, it means that the
// button has been pressed, released and pressed again
if (millis() - lastButtonPress > 50) {
Serial.println("Button pressed!");
counter = 0;
servo.write(counter);
}

// Remember last button press event
lastButtonPress = millis();
}

lastStateButton = btnState;

// Put in a slight delay to help debounce the reading
delay(1);
*/

  //tft.invertDisplay(true);
  //delay(500);
  //tft.invertDisplay(false);
  //delay(500);
}
