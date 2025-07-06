#ifndef _NRF24Manager_H
#define _NRF24Manager_H

#include <stdint.h>
#include <stdlib.h>

//	States
#define NRF24_INIT 0
#define NRF24_POWERUP 1
#define NRF24_POWERDOWN 2
#define NRF24_RECEIVE 3
#define NRF24_TRANSMIT 4
#define NRF24_STANDBY1 5
#define NRF24_STANDBY2 6


#define NRF24_MAX_MESSAGE_SIZE 32

#ifndef NRF24_CX_PIN_PORT
#define NRF24_CX_PIN_PORT A
#endif

class SPIManager;

class NRF24Manager {
public:
  NRF24Manager(uint8_t autoack) : _state(NRF24_INIT), _ce_pin(1), _cs_pin(1), _spi(NULL), _autoack(autoack) {}

  void init(SPIManager *s, uint8_t ce_pin, uint8_t cs_pin);

  void changeState(uint8_t state);
  uint8_t state() const { return this->_state; }

  void listen();
  uint8_t dataAvailable();

  const char *read_message();
  uint8_t* read_binary_message(uint8_t& length);
  uint8_t send(const char *msg);
  uint8_t send_binary(uint8_t *msg, uint8_t length);

  void summary();
  void info();

  void reset(uint8_t autoack);

private:
  void celow();
  void cehigh();
  void ack();
  uint8_t send_spi(uint8_t cmd, uint8_t *data, uint8_t size);
  uint8_t writeRegister(uint8_t reg, uint8_t *data, uint8_t size);
  uint8_t readRegister(uint8_t reg, uint8_t *data, uint8_t size);

  uint8_t _state;
  uint8_t _ce_pin;
  uint8_t _cs_pin;
  SPIManager *_spi;

  uint8_t _autoack;
};

#endif
