#ifndef ARDUINO
#include <millisec.h>
#else
#include <Arduino.h>
#endif

#include <string.h>

#include <usart_serial.h>
#include "SPIManager.h"
#include <gpio.h>

#define CS_PORT B 
#define CS_PIN PB4

#define LED_PORT B
#define LED_PIN PB3

SPIManager spi;

// buffer
const uint8_t BUF_SIZE = 32;
char inbuf[BUF_SIZE];
char outbuf[BUF_SIZE];
uint8_t outpos = 0;

void clearInBuf() {
  memset(inbuf, 0, BUF_SIZE);
}

void clearOutBuf() {
  outpos = 0;
  memset(outbuf, 0, BUF_SIZE);
}

void displayState() {
  USART_WriteString("Received on serial '");
  USART_WriteString(outbuf);
  USART_WriteString("' state: 0x");
  USART_WriteInt(spi.getState(), 16);
  USART_WriteString(" sreg: 0x");
  USART_WriteInt(spi.getStatusRegister(), 16);
  USART_WriteString("\n");
}

volatile void serial_rec(uint8_t data, bool error) {
  if (!error) {
    if (data == '\n') {
      outbuf[outpos] = 0;
      outpos = 0;
      displayState();
    } else {
      outbuf[outpos] = data;
      outpos++;
    }
  }

  if (outpos >= BUF_SIZE) {
    outpos = 0;
  }
}

void setup() {
  spi.startSlave();

  GPIO_OUTPUT(LED_PORT, LED_PIN);
  GPIO_SET_LOW(LED_PORT, LED_PIN);

  clearInBuf();
  clearOutBuf();

  USART_Init(BAUD_RATE_9600, serial_rec);
  USART_WriteString("ATmega8535 ready.\n");
}

void loop() {
  uint8_t cs = GPIO_READ(CS_PORT, CS_PIN);

  if (cs == GPIO_LOW) {
    if (spi.getState() == SPI_INIT) {
      USART_WriteString("Init LOW\n"); // wait for first HIGH to finish init
    } else if (spi.getState() == SPI_WAIT_MASTER) {
      // master as set CS LOW to start communication
      USART_WriteString("Start communication\n");
      GPIO_SET_HIGH(LED_PORT, LED_PIN);
      spi.setState(SPI_WAIT_COMMAND);
    }
  }
  
  if (cs == GPIO_HIGH) {
    if (spi.getState() == SPI_WAIT_COMMAND) {
      // master as released line
      USART_WriteString("End communication: without command\n");
    } else if (spi.getState() == SPI_WAIT_DATA) {
      // master as released line
      USART_WriteString("End communication:\n");
    } else if (spi.getState() == SPI_INIT) {
      USART_WriteString("Init HIGH\n");
    }

    spi.setState(SPI_WAIT_MASTER);
    GPIO_SET_LOW(LED_PORT, LED_PIN);
  }

  if (spi.getState() == SPI_WAIT_COMMAND) {
    uint8_t cmd = 0;
    bool success = spi.receiveCommand(cmd);

    if (success) {
      spi.setState(SPI_WAIT_DATA);
      uint8_t size = cmd & SPI_COMMAND_SIZE_MASK;
      cmd = cmd & SPI_COMMAND_MASK;

      success = spi.execCommand(size, (uint8_t*)outbuf, (uint8_t*)inbuf);
      if (success) {
        USART_WriteString("Exec command: ");
        USART_WriteInt(cmd, 16);
        USART_WriteString(" size ");
        USART_WriteInt(size);
        USART_WriteString("\n");
        USART_WriteString("Receive: ");
        USART_WriteString(inbuf);
        USART_WriteString("\n");
      } else {
        USART_WriteString("Error while executing command: ");
        USART_WriteInt(cmd, 16);
        USART_WriteString("\n");
      }
    } else {
      USART_WriteString("Error while receiving command: ");
      USART_WriteInt(cmd, 16);
      USART_WriteString("\n");
    }
    
    clearInBuf();
    clearOutBuf();
  }

  /*if (Serial.available()) {
    size_t s = Serial.readBytesUntil('\n', outbuf, BUF_SIZE);
    outbuf[s] = '\0';
    USART_WriteString("Received on serial '");
    USART_WriteString(outbuf);
    USART_WriteString("' state: 0x");
    USART_WriteInt(spi.getState(), 16);
    USART_WriteString(" sreg: 0x");
    USART_WriteInt(spi.getStatusRegister(), 16);
    USART_WriteString(cs==GPIO_HIGH?" cs: HIGH":" cs: LOW");
    USART_WriteString("\n");
  }*/
}

#ifndef ARDUINO
#include <main.cpp.h>
#endif