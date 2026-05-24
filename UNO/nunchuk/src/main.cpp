#include "usart_serial.h"
#include <avr/io.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <util/delay.h>

#include "millisec.h"

#include "nunchuk.h"

Nunchuk nunchuk;

bool loopmsg = false;

void setup() {
  // init serial com
  USART_Init(BAUD_RATE_115200, SerialCommandMgr::serialInput);

  // init nunchuk
  bool res = nunchuk.initialize();
  if (res) {
    USART_WriteString("# Wii nunchuk ready.\n");
  } else {
    USART_WriteString("# Wii nunchuk error.\n");
  }

  // ready to enter main loop
  USART_WriteString("# UNO Ready\n");

  USART_WriteString("time, accx, accy, accz, daccx, daccy, daccz, tiltx, "
                    "tilty, tiltz, acc\n");
}

void execCommand(const char *cmd) {
  // check if command is defined
  if (cmd == NULL || strlen(cmd) == 0)
    return;

  if (strcmp(cmd, "status") == 0) {
    nunchuk.update();
    nunchuk.display();
  } else if (strcmp(cmd, "calib") == 0) {
    nunchuk.display_calibration();
  } else if (strcmp(cmd, "loop") == 0) {
    loopmsg = true;
  } else if (strcmp(cmd, "noloop") == 0) {
    loopmsg = false;
  }
}

void show_joystick_position(uint32_t now, Nunchuk &nunchuk) {
  USART_WriteUInt(now);
  USART_WriteString(" ");
  USART_WriteChar(nunchuk.get_joystick_position());
  USART_WriteString(" JX:");
  USART_WriteInt(nunchuk.joystick_x());
  USART_WriteString(" JY:");
  USART_WriteInt(nunchuk.joystick_y());
  if (nunchuk.z_button()) {
    USART_WriteString(" Z");
  }
  if (nunchuk.c_button()) {
    USART_WriteString(" C");
  }
  USART_WriteString("\n");
}

const uint16_t PERIOD_MS = 1000;
uint32_t prevTime = 0;

void show_nunchuk_orientation(uint32_t now, Nunchuk &nunchuk) {
  USART_WriteUInt(now);
  USART_WriteString(", XT:");
  USART_WriteInt(nunchuk.x_tilt());
  USART_WriteString(", YT");
  USART_WriteInt(nunchuk.y_tilt());
  USART_WriteString(", ZT");
  USART_WriteInt(nunchuk.z_tilt());
  USART_WriteString(", ACC:");

  float accSquare = nunchuk.x_g() * nunchuk.x_g() + nunchuk.y_g() * nunchuk.y_g() + nunchuk.z_g() * nunchuk.z_g();
  USART_WriteFloat(accSquare, 5, 2);
  USART_WriteString("\n");
}

void loop() {
  uint32_t now = milliseconds();
  if (SerialCommandMgr::hasCommand()) {
    execCommand(SerialCommandMgr::command());
  }

  nunchuk.update();

  //if (nunchuk.c_button()) {
  //  show_nunchuk_orientation(now, nunchuk);
  //} 
  //
  //if (nunchuk.z_button()) {
  //  show_joystick_position(now, nunchuk);
  //} 
  //
  if (now - prevTime > PERIOD_MS && loopmsg) {
    show_joystick_position(now, nunchuk);
    show_nunchuk_orientation(now, nunchuk);
    prevTime = now;
  }
}

#include <main.cpp.h>