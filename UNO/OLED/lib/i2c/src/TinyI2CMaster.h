/* TinyI2C v2.0.1

   David Johnson-Davies - www.technoblogy.com - 5th June 2022

   CC BY 4.0
   Licensed under a Creative Commons Attribution 4.0 International license:
   http://creativecommons.org/licenses/by/4.0/
*/

#ifndef TinyI2CMaster_h
#define TinyI2CMaster_h

#include <stdint.h>

class TinyI2CMaster {

public:
  TinyI2CMaster();
  void init(void);
  uint8_t read(void);
  uint8_t readLast(void);
  bool write(uint8_t data);
  bool start(uint8_t address, uint8_t readcount);
  bool restart(uint8_t address, uint8_t readcount);
  void stop(void);

private:
  uint8_t I2Ccount;
  uint8_t transfer(uint8_t data);
};

extern TinyI2CMaster TinyI2C;

#endif
