#include "usart_serial.h"
#include <avr/io.h>
#include <stddef.h>
#include <util/delay.h>
#include <stdlib.h>
#include <string.h>

#include "millisec.h"

#include "SPIManager.h"
#include "nrf24mgr.h"
#include <gpio.h>


const uint8_t RADIO_CE_PIN = 1;
const uint8_t RADIO_CS_PIN = 2;

SPIManager spi;
NRF24Manager radio(1);

bool loopmsg = false;

void setup() {
  // init serial com
  USART_Init(BAUD_RATE_115200, SerialCommandMgr::serialInput);

  USART_WriteString("Init SPI\n");
  // init SPI bus to control NRF24
  spi.startMaster();

  // init NRF24
  USART_WriteString("Wait for radio\n");
  _delay_ms(1000); // give some time to NRF24 module to start
  USART_WriteString("Init radio\n");
  radio.init(&spi, RADIO_CE_PIN, RADIO_CS_PIN);

  // ready to enter main loop
  USART_WriteString("UNO Ready\n");

  radio.changeState(NRF24_POWERDOWN);
  radio.summary();
}

void execCommand(const char* cmd) {
  // check if command is defined
  if (cmd == NULL || strlen(cmd) ==0) return;

  USART_WriteString("Exec command: ");
  USART_WriteString(cmd);
  USART_WriteString("\n\n");
  if (strcmp(cmd, "status") == 0) {
    radio.summary();
  } else if (strcmp(cmd, "info") == 0) {
    radio.info();
  } else if (strcmp(cmd, "reset") == 0) {
    radio.reset(0);
  } else if (strcmp(cmd, "auto") == 0) {
    radio.reset(1);
  } else if (strcmp(cmd, "loop") == 0) {
    loopmsg = true;
  } else if (strcmp(cmd, "noloop") == 0) {
    loopmsg = false;
  } else if (strcmp(cmd, "listen") == 0) {
    radio.listen();
  } else if (strcmp(cmd, "on") == 0) {
    radio.changeState(NRF24_POWERUP);
  } else if (strcmp(cmd, "off") == 0) {
    radio.changeState(NRF24_POWERDOWN);
  } else if (strlen(cmd) > 0) {
    radio.send(cmd);
    _delay_ms(100);
  }
}

const uint16_t PERIOD_MS = 2000;
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
    USART_WriteString("now ");
    USART_WriteUInt(now/1000);
    USART_WriteString("s: ");
    const char *msg = radio.read_message();
    USART_WriteString(msg);
    
    if (strncmp(msg, "ping", 4) == 0) {
        USART_WriteString(" => send pong");
        radio.send("pong");
        radio.listen();
    }
    USART_WriteString("\n");   
  }
  

  if (now - prevTime > PERIOD_MS && loopmsg) {
    ultoa(count, number, 16);
    ping[4] = number[0];
    if (count > 15) {
      ping[5] = number[1];
    } else {
      ping[5] = '\0';
    }

    USART_WriteString("ping at ");
    USART_WriteUInt(now/1000);
    USART_WriteString("s '");
    USART_WriteString(ping);
    USART_WriteString("'\n");
    //time to ping
    radio.send(ping);
    radio.listen();
    count++;
    prevTime = now;
  }

}

#include <main.cpp.h>