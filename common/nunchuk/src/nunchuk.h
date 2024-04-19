#ifndef __NUNCHUK_H__
#define __NUNCHUK_H__

#include <stdint.h>

#define NUNCHUK_BUFFER_SIZE 6

class Nunchuk {
public:
  void initialize();
  bool update();
  
  uint8_t joystick_x() const { return _buffer[0]; }
  uint8_t joystick_y() const { return _buffer[1]; }

  uint16_t x_acceleration() const {
    return ((uint16_t)(_buffer[2]) << 2) | ((_buffer[5] >> 2) & 0x03);
  }
  
  uint16_t y_acceleration() const {
    return ((uint16_t)(_buffer[3]) << 2) | ((_buffer[5] >> 4) & 0x03);
  }
  
  uint16_t z_acceleration() const {
    return ((uint16_t)(_buffer[4]) << 2) | ((_buffer[5] >> 6) & 0x03);
  }
  
  bool z_button() const { return !(_buffer[5] & 0x01); }
  bool c_button() const { return !(_buffer[5] & 0x02); }
  
  void display();

private:
  void request_data();
  uint8_t decode_byte(uint8_t b) { return (b ^ 0x17) + 0x17; }
  
  uint8_t _buffer[NUNCHUK_BUFFER_SIZE];
};

#endif

