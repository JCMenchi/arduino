#ifndef __NUNCHUK_H__
#define __NUNCHUK_H__

#include <stdint.h>

/*
  For details go to http://wiibrew.org/wiki/Wiimote/Extension_Controllers/Nunchuck
*/

const char NUNCHUK_JOYSTICK_NORTH = '^';
const char NUNCHUK_JOYSTICK_NORTH_WEST = '\\';
const char NUNCHUK_JOYSTICK_NORTH_EAST = '/';
const char NUNCHUK_JOYSTICK_WEST = '<';
const char NUNCHUK_JOYSTICK_EAST = '>';
const char NUNCHUK_JOYSTICK_SOUTH = 'v';
const char NUNCHUK_JOYSTICK_SOUTH_WEST = ',';
const char NUNCHUK_JOYSTICK_SOUTH_EAST = '`';
const char NUNCHUK_JOYSTICK_CENTER = 'x';

const char NUNCHUK_JOYSTICK_UNKNOWN = '?';

#define NUNCHUK_BUFFER_SIZE 6
#define NUNCHUK_CALIBRATION_BUFFER_SIZE 16

class Nunchuk {
public:

  Nunchuk() : _joystick_prev_position(NUNCHUK_JOYSTICK_UNKNOWN), _buttons(0) {}

  bool initialize();

  bool update();
  
  char get_joystick_position();

  inline uint8_t joystick_x() const { return _buffer[0]; }
  inline uint8_t joystick_y() const { return _buffer[1]; }

  uint8_t joystick_strength();

  inline float x_g() const {
    return ((float)x_acceleration() - (float)_ax_0g) * _ax_res;
  }

  inline float y_g() const {
    return ((float)y_acceleration() - (float)_ay_0g) * _ay_res;
  }

  inline float z_g() const {
    return ((float)z_acceleration() - (float)_az_0g) * _az_res;
  }

  inline int16_t x_tilt() const {
    float d = sqrt(y_g() * y_g() + z_g() * z_g() );
    return atan2(x_g(), d)*360/M_PI;
  }
  //
  inline int16_t y_tilt() const {
    float d = sqrt(x_g() * x_g() + z_g() * z_g() );
    return atan2(y_g(), d)*360/M_PI;
  }
  //
  inline int16_t z_tilt() const {
    float d = sqrt(y_g() * y_g() + x_g() * x_g() );
    return atan2(d, z_g())*360/M_PI;
  }
  
  inline int16_t x_acceleration() const {
    return ((uint16_t)(_buffer[2]) << 2) | ((_buffer[5] >> 2) & 0x03);
  }

  inline int16_t y_acceleration() const {
    return ((uint16_t)(_buffer[3]) << 2) | ((_buffer[5] >> 4) & 0x03);
  }
  
  inline int16_t z_acceleration() const {
    return ((uint16_t)(_buffer[4]) << 2) | ((_buffer[5] >> 6) & 0x03);
  }
  
  inline bool z_button() const { return !(_buffer[5] & 0x01); }
  inline bool c_button() const { return !(_buffer[5] & 0x02); }
  
  void display();
  void display_calibration();

private:
  bool get_calibration();
  uint8_t decode_byte(uint8_t b) { return (b ^ 0x17) + 0x17; }
  
  // last read
  uint8_t _buffer[NUNCHUK_BUFFER_SIZE];

  // previous state
  char _joystick_prev_position;
  uint8_t _buttons;

  // calibration data
  uint8_t _jx_max;
  uint8_t _jx_min;
  uint8_t _jx_center;
  uint8_t _jy_max;
  uint8_t _jy_min;
  uint8_t _jy_center;
  uint16_t _ax_0g;
  uint16_t _ay_0g;
  uint16_t _az_0g;
  uint16_t _ax_1g;
  uint16_t _ay_1g;
  uint16_t _az_1g;
  float _ax_res;
  float _ay_res;
  float _az_res;
};

#endif

