#include "usart_serial.h"
#include <stddef.h>
#include <util/delay.h>
#include <avr/io.h>
#include <avr/wdt.h>
#include <stdlib.h>
#include <string.h>
#include <avr/pgmspace.h>

#include "millisec.h"
#include "nrf24mgr.h"
#include "SPIManager.h"
#include <gpio.h>
#include <SSD1306Display.h>

const uint8_t ON_LED_PIN = 0;
const uint8_t RADIO_COM_LED_PIN = 1;

const uint8_t RADIO_CE_PIN = 2;

SSD1306Display display(128, 32);
SPIManager spi;
NRF24Manager radio;

// init MCU
void setup() {

  // init debug LED
  GPIO_OUTPUT(A, ON_LED_PIN);
  GPIO_OUTPUT(A, RADIO_COM_LED_PIN);
  GPIO_SET_LOW(A, RADIO_COM_LED_PIN);

  // startup blinking of ON LED
  GPIO_SET_HIGH(A, ON_LED_PIN);
  _delay_ms(200);
  GPIO_SET_LOW(A, ON_LED_PIN);
  _delay_ms(500);
  GPIO_SET_HIGH(A, ON_LED_PIN);
  _delay_ms(200);
  GPIO_SET_LOW(A, ON_LED_PIN);
  _delay_ms(500);
  GPIO_SET_HIGH(A, ON_LED_PIN);

  // init serial com
  USART_Init(BAUD_RATE_57600, SerialCommandMgr::serialInput);

  // init SPI bus to control NRF24
  spi.startMaster();

  // init NRF24
  _delay_ms(100); // give some time to NRF24 module to start
  radio.init(&spi, RADIO_CE_PIN, 0);

  // init OLED display
  display.init(0x20);
  display.flip(SSD1306_OFF);
  display.drawScreen(0x00);
  display.drawPString(0, SSD1306_LINE0, PSTR("ATmega1284P"));

  // ready to enter main loop
  USART_WriteString("ATmega1284P Ready\n");

  radio.summary();
}

void sendMsg(const char* msg) {
  display.clearPage(3);
  display.drawString(0, SSD1306_LINE3, "SND:");
  display.drawString(26, SSD1306_LINE3, msg);
  GPIO_SET_HIGH(A, RADIO_COM_LED_PIN);
  radio.send(msg);
  radio.listen();
  GPIO_SET_LOW(A, RADIO_COM_LED_PIN);
}

void execCommand(const char* cmd) {
  // check if command is defined
  if (cmd == NULL || strlen(cmd) ==0) return;
  display.clearPage(1);
  display.drawString(0, SSD1306_LINE1, "EXE:");
  display.drawString(26, SSD1306_LINE1, cmd);
  // USART_WriteString("Exec command: ");
  // USART_WriteString(cmd);
  // USART_WriteString("\n\n");
  if (strcmp(cmd, "status") == 0) {
    radio.summary();
    //chuk.initialize();
    //chuk.display();
  } else if (strcmp(cmd, "info") == 0) {
    radio.info();
  } else if (strcmp(cmd, "reset") == 0) {
    radio.reset();
  } else if (strcmp(cmd, "on") == 0) {
    radio.changeState(NRF24_POWERUP);
    radio.listen();
  } else if (strcmp(cmd, "off") == 0) {
    radio.changeState(NRF24_POWERDOWN);
  } else if (strlen(cmd) > 0) {
    sendMsg(cmd);
  }
}

const uint16_t PERIOD_MS = 15000;
uint32_t prevTime = 0;
uint8_t count = 0;

static char number[5];

char ping[7] = "pingXX";

void loop() {
  uint32_t now = milliseconds();

  if (SerialCommandMgr::hasCommand()) {
    execCommand(SerialCommandMgr::command());
  }

  if (radio.dataAvailable()) {
    // get current time
    const char* msg = radio.read_message();
    if (msg && strlen(msg) > 0) {
      USART_WriteString("now ");
      USART_WriteUInt(now/1000);
      USART_WriteString("s: ");
      USART_WriteString(msg);
      
      if (strncmp(msg, "pong", 4) == 0) {
        display.drawString(90, SSD1306_LINE3, msg);
      } else {
        display.clearPage(2);
        display.drawString(0, SSD1306_LINE2, "REC:");
        display.drawString(26, SSD1306_LINE2, msg);
      }

      if (strncmp(msg, "ping", 4) == 0) {
        USART_WriteString(" => send pong");
        const char* counter = msg+4;
        display.drawString(100, SSD1306_LINE2,counter);
        radio.send("pong");
        radio.listen();
      }
      USART_WriteString("\n");
    }
  }

  if (now - prevTime > PERIOD_MS) {
    ultoa(count, number, 16);
    ping[4] = number[0];
    if (count > 15) {
      ping[5] = number[1];
    } else {
      ping[5] = '\0';
    }
    sendMsg(ping);
    count++;
    prevTime = now;
  }

}

#include <main.cpp.h>