#include <avr/io.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <util/delay.h>
#include <avr/pgmspace.h>

#include "usart_serial.h"
#include "millisec.h"
#include "SPIManager.h"
#include "nrf24mgr.h"
#include "nunchuk.h"
#include "SSD1306Display.h"

SPIManager spi;
NRF24Manager radio(1);
SSD1306Display display(128, 32);

Nunchuk nunchuk;

const uint8_t RADIO_CE_PIN = 1;

void setup() {
  // init serial com
  USART_Init(BAUD_RATE_115200, SerialCommandMgr::serialInput);

  // init SPI bus to control NRF24
  spi.startMaster();

  // init NRF24
  _delay_ms(100); // give some time to NRF24 module to start
  radio.init(&spi, RADIO_CE_PIN);
  radio.summary();

  // init nunchuk
  bool res = nunchuk.initialize();
  if (res) {
    USART_WritePString(PSTR("# Wii nunchuk ready.\n"));
  } else {
    USART_WritePString(PSTR("# Wii nunchuk error.\n"));
  }

  // init OLED display
  display.init(0x20);
  display.flip(SSD1306_OFF);
  display.drawScreen(0x00);
  display.drawPString(0, SSD1306_LINE0, PSTR("ATmega328P"));

  // ready to enter main loop
  USART_WritePString(PSTR("# UNO Ready\n"));
}

void show_joystick_position(uint32_t now, Nunchuk &nunchuk) {
  USART_WriteUInt(now);
  USART_WriteString(" ");
  USART_WriteChar(nunchuk.get_joystick_position());
  USART_WriteString(" ");
  USART_WriteInt(nunchuk.joystick_x());
  USART_WriteString(" ");
  USART_WriteInt(nunchuk.joystick_y());
  if (nunchuk.z_button()) {
    USART_WriteString(" Z");
  }
  if (nunchuk.c_button()) {
    USART_WriteString(" C");
  }
  USART_WriteString("\n");
  display.drawChar(0, SSD1306_LINE1, nunchuk.get_joystick_position());

  uint8_t pos = 10;
  if (nunchuk.joystick_x() < 10) {
    pos = display.drawString(10, SSD1306_LINE1, "  ");
  } else if (nunchuk.joystick_x() < 100) {
    pos = display.drawString(10, SSD1306_LINE1, " ");
  }
  display.drawInt(pos, SSD1306_LINE1, nunchuk.joystick_x(), 10);

  pos = 50;
  if (nunchuk.joystick_y() < 10) {
    pos = display.drawString(10, SSD1306_LINE1, "  ");
  } else if (nunchuk.joystick_y() < 100) {
    pos = display.drawString(10, SSD1306_LINE1, " ");
  }
  display.drawInt(pos, SSD1306_LINE1, nunchuk.joystick_y(), 10);
}

void show_nunchuk_orientation(uint32_t now, Nunchuk &nunchuk) {
  USART_WriteUInt(now);
  USART_WriteString(", ");
  USART_WriteInt(nunchuk.x_tilt());
  USART_WriteString(", ");
  USART_WriteInt(nunchuk.y_tilt());
  USART_WriteString(", ");
  USART_WriteInt(nunchuk.z_tilt());
  USART_WriteString(", ");

  float accSquare = nunchuk.x_g() * nunchuk.x_g() + nunchuk.y_g() * nunchuk.y_g() + nunchuk.z_g() * nunchuk.z_g();
  USART_WriteFloat(accSquare, 5, 2);
  USART_WriteString("\n");
}

void radio_message(uint32_t now, NRF24Manager &radio) {
  USART_WriteString("now ");
  USART_WriteUInt(now/1000);
  USART_WriteString("s: ");
  const char *msg = radio.read_message();
  USART_WriteString(msg);
  USART_WriteString("\n");   
}

void send_to_remote(uint32_t now, NRF24Manager& radio, Nunchuk& nunchuk) {
  uint8_t data[NRF24_MAX_MESSAGE_SIZE];

  data[0] = 1 + 1 + 3 * 2 + 1 + 2; // message size
  
  data[1] = nunchuk.get_joystick_position();

  // copy orientation
  int16_t x = nunchuk.x_tilt();
  int16_t y = nunchuk.y_tilt();
  int16_t z = nunchuk.z_tilt();
  memcpy(data + 2, &x, 2);
  memcpy(data + 4, &y, 2);
  memcpy(data + 6, &z, 2);
  data[8] = nunchuk.joystick_strength();
  data[9] = nunchuk.joystick_x();
  data[10] = nunchuk.joystick_y();

  radio.send_binary(data, 11);
  radio.listen();
}

void execCommand(uint32_t now, const char *cmd) {
  // check if command is defined
  if (cmd == NULL || strlen(cmd) == 0)
    return;

  if (strcmp(cmd, "status") == 0) {
    show_joystick_position(now, nunchuk);
    show_nunchuk_orientation(now, nunchuk);
    radio.info();
  } else if (strcmp(cmd, "calib") == 0) {
    nunchuk.display_calibration();
  }
}

void loop() {
  _delay_ms(50);
  uint32_t now = milliseconds();

  // update devices
  bool changed = nunchuk.update();
  
  if (radio.dataAvailable()) {
    radio_message(now, radio);
  }

  // send info to remote
  if (nunchuk.c_button()) { // press C to send data to remote
    send_to_remote(now, radio, nunchuk);
  }

  // exec user commands
  if (SerialCommandMgr::hasCommand()) {
    execCommand(now, SerialCommandMgr::command());
  }

  // debug logs
  if (nunchuk.c_button()) {
    show_nunchuk_orientation(now, nunchuk);
  } 

  if (changed) {
    show_joystick_position(now, nunchuk);
  }
}

#include <main.cpp.h>