
#include <TinyI2CMaster.h>
#include <util/delay.h>

//#define HAS_SERIAL

#ifdef HAS_SERIAL
#include <usart_serial.h>
#endif

#include "nunchuk.h"

/*
  Classic calibration

  Nunchuk calibration:
    
        min center max
    JX   26   128  228
    JY   31   128  222
    
             X   Y   Z
    Acc 0G 504 513 516
    Acc 1G 708 710 724

*/

#define NUNCHUK_I2C_ID 0x52
#define WII_DEVICE_REGISTER 0xFA
#define WII_POSITION_REGISTER 0x00
#define WII_CALIBRATION_REGISTER 0x20

const uint32_t NUNCHUK_DEVICE_ID_PART1 = 0x0000A420;
const uint32_t NUNCHUK_DEVICE_ID_PART2 = 0x00000000; 

#define NOT_ENCRYPTED


bool Nunchuk::initialize() {

  // Init I2C com
  TinyI2C.init(true);

  // normal init sequence; data is encrypted
  // for unencrypted use: START 0xF0, 0x55, STOP - START, 0xFB, 0x00, STOP
  bool con = TinyI2C.start(NUNCHUK_I2C_ID, 0);
  if (!con) {
#ifdef HAS_SERIAL
    USART_WriteString("I2C connect error\n");
#endif
    return false;
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
  _delay_ms(10);

  // Read device type
  TinyI2C.start(NUNCHUK_I2C_ID, 0);
  TinyI2C.write(WII_DEVICE_REGISTER);
  TinyI2C.stop();
  _delay_ms(10);

  TinyI2C.start(NUNCHUK_I2C_ID, NUNCHUK_BUFFER_SIZE);
  uint32_t readID1 = 0;
  uint32_t readID2 = 0;

  readID1 = TinyI2C.read();
  readID1 <<= 8;
  uint8_t r = TinyI2C.read();
  readID1 |= r;
  readID1 <<= 8;
  r = TinyI2C.read();
  readID1 |= r;
  readID1 <<= 8;
  r = TinyI2C.read();
  readID1 |= r;

  readID2 = TinyI2C.read();
  readID2 <<= 8;
  r = TinyI2C.read();
  readID2 |= r;
  TinyI2C.stop();

  if (readID1 == NUNCHUK_DEVICE_ID_PART1 && readID2 == NUNCHUK_DEVICE_ID_PART2) {
    #ifdef HAS_SERIAL
      USART_WriteString("# Wii Nunchuk is OK.\n");
    #endif
    // read calibration data
    _delay_ms(100);
    get_calibration();

    // read current data
    _delay_ms(100);
    update();
  } else {
    #ifdef HAS_SERIAL
    USART_WriteString("Not a Nunchuk. ID found: ");
    USART_WriteUInt(readID1, 16);
    USART_WriteString(" ");
    USART_WriteUInt(readID2, 16);
    USART_WriteString("\n");
    #endif
    return false;
  }

  return true;
}

bool Nunchuk::update() {
  TinyI2C.start(NUNCHUK_I2C_ID, 0);
  TinyI2C.write(WII_POSITION_REGISTER);
  TinyI2C.stop();
  _delay_us(500);

  TinyI2C.start(NUNCHUK_I2C_ID, NUNCHUK_BUFFER_SIZE);
  _buffer[0] = TinyI2C.read();
  _buffer[1] = TinyI2C.read();
  _buffer[2] = TinyI2C.read();
  _buffer[3] = TinyI2C.read();
  _buffer[4] = TinyI2C.read();
  _buffer[5] = TinyI2C.read();
  TinyI2C.stop();

  char joystick_pos = this->get_joystick_position();
  if ((_joystick_prev_position != joystick_pos) || (_buttons != (_buffer[5] & 0x3))) {
    _joystick_prev_position = joystick_pos;
    _buttons = _buffer[5] & 0x3;
    
    return true;
  }

  return false;
}

bool Nunchuk::get_calibration() {
  TinyI2C.start(NUNCHUK_I2C_ID, 0);
  TinyI2C.write(WII_CALIBRATION_REGISTER);
  TinyI2C.stop();
  _delay_ms(10);

  uint8_t r = 0;

  TinyI2C.start(NUNCHUK_I2C_ID, NUNCHUK_CALIBRATION_BUFFER_SIZE);
  // X,Y,Z 0g
  _ax_0g = TinyI2C.read();
  _ay_0g = TinyI2C.read();
  _az_0g = TinyI2C.read();
  r = TinyI2C.read();
  _ax_0g <<= 2;
  _ax_0g |= ((r >> 2) & 0x03);
  _ay_0g <<= 2;
  _ay_0g |= ((r >> 4) & 0x03);
  _az_0g <<= 2;
  _az_0g |= ((r >> 6) & 0x03);

  // X,Y,Z 1g
  _ax_1g = TinyI2C.read();
  _ay_1g = TinyI2C.read();
  _az_1g = TinyI2C.read();
  r = TinyI2C.read();
  _ax_1g <<= 2;
  _ax_1g |= ((r >> 2) & 0x03);
  _ay_1g <<= 2;
  _ay_1g |= ((r >> 4) & 0x03);
  _az_1g <<= 2;
  _az_1g |= ((r >> 6) & 0x03);

  // Joystick X
  _jx_max = TinyI2C.read();
  _jx_min = TinyI2C.read();
  _jx_center = TinyI2C.read();
  // Joystick Y
  _jy_max = TinyI2C.read();
  _jy_min = TinyI2C.read();
  _jy_center = TinyI2C.read();
  // checksum
  r = TinyI2C.read();
  r = TinyI2C.read();

  TinyI2C.stop();

  // derive value
  _ax_res = 1.0f/((float)_ax_1g-(float)_ax_0g);
  _ay_res = 1.0f/((float)_ay_1g-(float)_ay_0g);
  _az_res = 1.0f/((float)_az_1g-(float)_az_0g);

  return true;
}

uint8_t Nunchuk::joystick_strength() {
  int16_t dx = joystick_x() - _jx_center;
  int16_t dy = joystick_y() - _jy_center;

  return (uint8_t)(sqrt((dx*dx+dy*dy)/10000.0f) * 255); // max radius is around 100
}

void Nunchuk::display_calibration() {
  #ifdef HAS_SERIAL
  USART_WriteString("Nunchuk calibration:\n");
  USART_WriteString("  JX ");
  USART_WriteUInt(_jx_min);
  USART_WriteString(" ");
  USART_WriteUInt(_jx_center);
  USART_WriteString(" ");
  USART_WriteUInt(_jx_max);
  USART_WriteString("\n");
  USART_WriteString("  JY ");
  USART_WriteUInt(_jy_min);
  USART_WriteString(" ");
  USART_WriteUInt(_jy_center);
  USART_WriteString(" ");
  USART_WriteUInt(_jy_max);
  USART_WriteString("\n");

  USART_WriteString("  Acc 0G ");
  USART_WriteUInt(_ax_0g);
  USART_WriteString(" ");
  USART_WriteUInt(_ay_0g);
  USART_WriteString(" ");
  USART_WriteUInt(_az_0g);
  USART_WriteString("\n");

  USART_WriteString("  Acc 1G ");
  USART_WriteUInt(_ax_1g);
  USART_WriteString(" ");
  USART_WriteUInt(_ay_1g);
  USART_WriteString(" ");
  USART_WriteUInt(_az_1g);
  USART_WriteString("\n");

  float ming = -1.0f * (float)_ax_0g * _ax_res;
  float maxg = (1024.0f - _ax_0g) * _ax_res;

  USART_WriteString("  AccX min=");
  USART_WriteFloat(ming, 4, 1);
  USART_WriteString(" max=");
  USART_WriteFloat(maxg, 4, 1);
  USART_WriteString(" res=");
  USART_WriteFloat(_ax_res, 6, 3);
  USART_WriteString("\n");

  ming = -1.0f * (float)_ay_0g * _ay_res;
  maxg = (1024.0f - _ay_0g) * _ay_res;

  USART_WriteString("  AccY min=");
  USART_WriteFloat(ming, 4, 1);
  USART_WriteString(" max=");
  USART_WriteFloat(maxg, 4, 1);
  USART_WriteString(" res=");
  USART_WriteFloat(_ay_res, 6, 3);
  USART_WriteString("\n");

  ming = -1.0f * (float)_az_0g * _az_res;
  maxg = (1024.0f - _az_0g) * _az_res;

  USART_WriteString("  AccZ min=");
  USART_WriteFloat(ming, 4, 1);
  USART_WriteString(" max=");
  USART_WriteFloat(maxg, 4, 1);
  USART_WriteString(" res=");
  USART_WriteFloat(_az_res, 6, 3);
  USART_WriteString("\n");

  #endif
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

char Nunchuk::get_joystick_position() {

  uint8_t resx = (_jx_max -_jx_min) / 6;
  uint8_t resy = (_jy_max -_jy_min) / 6;

  if (_jx_center - resx <= joystick_x() && joystick_x() <= _jx_center + resx) {
    // X center position
    if (_jy_center - resy <= joystick_y() && joystick_y() <= _jy_center + resy) {
      // Y center position
      return NUNCHUK_JOYSTICK_CENTER;
    } else if (_jy_center + resy < joystick_y()) {
      // Y north position
      return NUNCHUK_JOYSTICK_NORTH;
    } else if (joystick_y() < _jy_center - resy) {
      // Y south position
      return NUNCHUK_JOYSTICK_SOUTH;
    }
  } else if (_jx_center + resx < joystick_x()) {
    // X east position
    if (_jy_center - resy <= joystick_y() && joystick_y() <= _jy_center + resy) {
      // Y center position
      return NUNCHUK_JOYSTICK_EAST;
    } else if (_jy_center + resy < joystick_y()) {
      // Y north position
      return NUNCHUK_JOYSTICK_NORTH_EAST;
    } else if (joystick_y() < _jy_center - resy) {
      // Y south position
      return NUNCHUK_JOYSTICK_SOUTH_EAST;
    }
  } else if (joystick_x() < _jx_center - resx) {
    // X west position
    if (_jy_center - resy <= joystick_y() && joystick_y() <= _jy_center + resy) {
      // Y center position
      return NUNCHUK_JOYSTICK_WEST;
    } else if (_jy_center + resy < joystick_y()) {
      // Y north position
      return NUNCHUK_JOYSTICK_NORTH_WEST;
    } else if (joystick_y() < _jy_center - resy) {
      // Y south position
      return NUNCHUK_JOYSTICK_SOUTH_WEST;
    }
  }

  return NUNCHUK_JOYSTICK_UNKNOWN;
}