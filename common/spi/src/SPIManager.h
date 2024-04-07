#ifndef __TINY_SPI_H__
#define __TINY_SPI_H__

#include <avr/common.h>

const uint8_t SPI_INIT = 1;
const uint8_t SPI_WAIT_MASTER = 2;
const uint8_t SPI_WAIT_COMMAND = 3;
const uint8_t SPI_WAIT_DATA = 4;

#define SPI_MODE_MASK 0x03          // 0000 0011
#define SPI_MODE_MASTER 0x03
#define SPI_MODE_SLAVE 0x01

#define SPI_COMMAND_MASK 0xE0       // 1110 0000
#define SPI_COMMAND_SIZE_MASK 0x1F  // 0001 1111

class SPIManager {
public:
    SPIManager() : _state(SPI_INIT), _statusRegister(0) {}

    uint8_t getState() const { return this->_state; }
    void setState(uint8_t s) { this->_state = s; }
    uint8_t getStatusRegister() const { return this->_statusRegister; }

    void startSlave();
    bool isSlave() const { return ((this->_statusRegister & SPI_MODE_MASK) == SPI_MODE_SLAVE); }
    bool receiveCommand(uint8_t& command);
    bool execCommand(uint8_t size, uint8_t* outbuffer,  uint8_t* inbuffer);

    void startMaster();
    bool isMaster() const { return ((this->_statusRegister & SPI_MODE_MASK) == SPI_MODE_MASTER); }
    void begin();
    void end();
    bool send(uint8_t data);
    bool sendCommand(uint8_t& command);
    bool sendCommandData(uint8_t size, uint8_t* outbuffer,  uint8_t* inbuffer);
    bool sendCommandData(uint8_t size, uint8_t* inoutbuffer);

private:
    uint8_t _state;
    volatile uint8_t _statusRegister;
};



#endif /* __TINY_SPI_H__ */
