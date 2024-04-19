
#include <TinyI2CMaster.h>
#include <util/delay.h>

#define HAS_SERIAL

#ifdef HAS_SERIAL
#include <usart_serial.h>
#endif

#include "nunchuk.h"

#define NUNCHUK_I2C_ID 0x52
#define WII_DEVICE_REGISTER 0xFA

void Nunchuk::initialize() {
  // Init I2C com
  TinyI2C.init();

  // normal init sequence; data is encrypted
  // for unencrypted use: START 0xF0, 0x55, STOP - START, 0xFB, 0x00, STOP
  bool con = TinyI2C.start(NUNCHUK_I2C_ID, 0);
  if (!con) {
#ifdef HAS_SERIAL
    USART_WriteString("I2C connect error\n");
#endif
    return;
  }

#ifndef NOT_ENCRYPTED
  TinyI2C.write(0x40);
  TinyI2C.write(0x00);
#else
  TinyI2C.write(0xF0);
  TinyI2C.write(0x55);
  TinyI2C.stop();
  TinyI2C.start(NUNCHUK_I2C_ID, 0);
  TinyI2C.write(0xFB);
  TinyI2C.write(0x00);
#endif
  TinyI2C.stop();

  // Read device type
  TinyI2C.start(NUNCHUK_I2C_ID, 0);
  TinyI2C.write(WII_DEVICE_REGISTER);
  TinyI2C.stop();

  TinyI2C.start(NUNCHUK_I2C_ID, NUNCHUK_BUFFER_SIZE);
  _buffer[0] = TinyI2C.read();
  _buffer[1] = TinyI2C.read();
  _buffer[2] = TinyI2C.read();
  _buffer[3] = TinyI2C.read();
  _buffer[4] = TinyI2C.read();
  _buffer[5] = TinyI2C.read();
  TinyI2C.stop();

#ifdef HAS_SERIAL
  USART_WriteString("Nunchuk ID: ");
  for (uint8_t i = 0; i < NUNCHUK_BUFFER_SIZE; ++i) {
    USART_WriteInt(_buffer[i], 16);
  }

  USART_WriteString("\n");
#endif

  update();
}

bool Nunchuk::update() {
  TinyI2C.start(NUNCHUK_I2C_ID, NUNCHUK_BUFFER_SIZE);
  _buffer[0] = TinyI2C.read();
  _buffer[1] = TinyI2C.read();
  _buffer[2] = TinyI2C.read();
  _buffer[3] = TinyI2C.read();
  _buffer[4] = TinyI2C.read();
  _buffer[5] = TinyI2C.read();
  TinyI2C.stop();

  request_data();

  return true;
}

void Nunchuk::request_data() {
  TinyI2C.start(NUNCHUK_I2C_ID, 0);
  TinyI2C.write(0x00);
  TinyI2C.stop();
}

void Nunchuk::display() {
#ifdef HAS_SERIAL
  USART_WriteString("Nunchuk info:\n");

  USART_WriteString("joystick: ");
  USART_WriteUInt(joystick_x());
  USART_WriteString(", ");
  USART_WriteUInt(joystick_y());
  USART_WriteString("\n");

  USART_WriteString("accel: ");
  USART_WriteUInt(x_acceleration());
  USART_WriteString(", ");
  USART_WriteUInt(y_acceleration());
  USART_WriteString(", ");
  USART_WriteUInt(z_acceleration());
  USART_WriteString("\n");

  USART_WriteString("Button: ");
  USART_WriteString(z_button()?"Z":"z");
  USART_WriteString(" ");
  USART_WriteString(c_button()?"C":"c");
  USART_WriteString("\n");

#endif
}