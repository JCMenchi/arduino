#include <Arduino.h>

//#define USE_SPI

const uint8_t BUF_SIZE = 32;

#ifdef USE_SPI
#include <SPI.h>

void setup() {
  Serial.begin(115200);
  Serial.println("Welcome");

  pinMode(MISO, INPUT);
  pinMode(MOSI, OUTPUT);
  pinMode(SS, OUTPUT);
  pinMode(SCK, OUTPUT);

  SPI.begin();
  digitalWrite(SS, HIGH);
}

volatile int i = 0;
uint8_t spi_data = 0;

void client_loop(void) {
  if (i) {
    i = 0;
    Serial.print("Received data item from Master: ");
    Serial.print(spi_data, HEX);
    Serial.print(" ");
    Serial.println((char)spi_data);
    Serial.println("=============================================");
    SPDR = 42;
  }
}


void master_loop1() {
  uint8_t readchar = 0;
  while (Serial.available()) {
    char buf[BUF_SIZE];
    size_t s = Serial.readBytesUntil('\n', buf, BUF_SIZE);
    buf[s] = '\0';
    Serial.print("Send data ");
    Serial.println(buf);

    SPI.beginTransaction(SPISettings(100000, MSBFIRST, SPI_MODE0));
    digitalWrite(SS, LOW);
    // send command
    uint8_t cmd = 0x20;
    cmd |= (uint8_t)s;
    uint8_t sr = SPI.transfer(cmd);
    Serial.print("StatusReg: ");
    Serial.println(sr, HEX);
    delay(100);
    // send data
    SPI.transfer(buf, strlen(buf));
    delay(100);
    digitalWrite(SS, HIGH);

    SPI.endTransaction();

    digitalWrite(SCK, LOW);
    digitalWrite(MOSI, LOW);
    digitalWrite(MISO, LOW);
    
    Serial.print(" received ");
    Serial.println(buf);
    Serial.println("=============================================");
  }
}

void loop(void) {
#ifdef AS_MASTER
  master_loop1();
#else
  client_loop();
#endif
}

#else

#include "SPIManager.h"

// buffer
char inbuf[BUF_SIZE];

void clearInBuf() {
  memset(inbuf, 0, BUF_SIZE);
}

SPIManager spi;

void setup() {
  Serial.begin(115200);
  Serial.println("Welcome");

  spi.startMaster();
  clearInBuf();
}

void loop(void) {
  
  while (Serial.available()) {
 
    char buf[BUF_SIZE];
    size_t s = Serial.readBytesUntil('\n', buf, BUF_SIZE);
    buf[s] = '\0';
    Serial.print("Send data ");
    Serial.println(buf);

    digitalWrite(SS, LOW);
    // send command
    uint8_t cmd = 0x20;
    cmd |= (uint8_t)s;
    bool success = spi.sendCommand(cmd);

    Serial.print("StatusReg: ");
    Serial.println(cmd, HEX);
    if (success) {
      delay(100);
      // send data
      spi.sendCommandData(strlen(buf), (uint8_t*) buf, (uint8_t*) inbuf);
      delay(100);
    } else {
      Serial.println("Error sending command.");
    }
    
    digitalWrite(SS, HIGH);
    digitalWrite(SCK, LOW);
    digitalWrite(MOSI, LOW);
    digitalWrite(MISO, LOW);
    
    Serial.print(" received ");
    Serial.println(inbuf);
    Serial.println("=============================================");

    clearInBuf();
  }
}

#endif
