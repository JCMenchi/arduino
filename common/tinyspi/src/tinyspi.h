/**
 * @file tinyspi.h
 * @brief SPI Manager for AVR microcontrollers
 * 
 * Unified SPI interface supporting both hardware SPI and USI-based implementations.
 * Provides master and slave modes with command and data exchange capabilities.
 * 
 * Device Support:
 * - Hardware SPI: ATmega8535, ATmega32, ATmega328P
 * - USI-based SPI: ATtiny84, ATtiny45 (and other ATtiny with USI)
 * 
 * @note Pin mappings vary by device architecture. See device-specific defines in .cpp
 */
#ifndef __TINY_SPI_H__
#define __TINY_SPI_H__

#include <avr/common.h>

/** @brief SPI manager initialization state */
const uint8_t SPI_INIT = 1;
/** @brief Waiting for master acknowledgment */
const uint8_t SPI_WAIT_MASTER = 2;
/** @brief Waiting for command reception */
const uint8_t SPI_WAIT_COMMAND = 3;
/** @brief Waiting for data reception */
const uint8_t SPI_WAIT_DATA = 4;

/** @brief Status register mode mask (2-bit field) */
#define SPI_MODE_MASK 0x03
/** @brief Master mode status value */
#define SPI_MODE_MASTER 0x03
/** @brief Slave mode status value */
#define SPI_MODE_SLAVE 0x01

/** @brief Command header mask (upper 3 bits contain command ID) */
#define SPI_COMMAND_MASK 0xE0
/** @brief Command size mask (lower 5 bits contain payload size in bytes) */
#define SPI_COMMAND_SIZE_MASK 0x1F

/** @brief Default chip select port (A for ATtiny84, B for others) */
#ifndef SPI_SELECT_PORT
#define SPI_SELECT_PORT A
#endif


/**
 * @class SPIManager
 * @brief Unified SPI interface for master and slave operations
 * 
 * Provides a consistent API across different AVR hardware implementations.
 * Supports both full-duplex (send/receive) and command-based transactions.
 */
class SPIManager {
public:
    /**
     * @brief Default constructor
     * 
     * Initializes SPI manager in uninitialized state.
     */
    SPIManager() : _state(SPI_INIT), _statusRegister(0) {}

    /**
     * @brief Assert chip select (pull low)
     * @param cspin GPIO pin number for chip select
     */
    void begin(uint8_t cspin);
    
    /**
     * @brief Deassert chip select (pull high)
     * @param cspin GPIO pin number for chip select
     */
    void end(uint8_t cspin);

    /**
     * @brief Get current manager state
     * @return Current state (SPI_INIT, SPI_WAIT_MASTER, etc.)
     */
    uint8_t getState() const { return this->_state; }
    
    /**
     * @brief Set manager state
     * @param s New state value
     */
    void setState(uint8_t s) { this->_state = s; }
    
    /**
     * @brief Get status register
     * @return Status register containing mode and flags
     */
    uint8_t getStatusRegister() const { return this->_statusRegister; }

    /**
     * @brief Initialize SPI as slave
     * 
     * Configures SPI hardware for slave mode, sets appropriate pin directions.
     */
    void startSlave();
    
    /**
     * @brief Check if in slave mode
     * @return true if configured as slave
     */
    bool isSlave() const { return ((this->_statusRegister & SPI_MODE_MASK) == SPI_MODE_SLAVE); }
    
    /**
     * @brief Wait for and receive command byte from master
     * @param command Reference to store received command byte
     * @return true if command received, false if CS deasserted before reception
     */
    bool receiveCommand(uint8_t& command);
    
    /**
     * @brief Exchange command data with master
     * 
     * Performs full-duplex SPI transfer, sending data from outbuffer
     * while receiving into inbuffer.
     * 
     * @param size Number of bytes to exchange
     * @param outbuffer Pointer to output data
     * @param inbuffer Pointer to input data buffer
     * @return true if transfer completed, false if CS deasserted early
     */
    bool execCommand(uint8_t size, uint8_t* outbuffer,  uint8_t* inbuffer);

    /**
     * @brief Initialize SPI as master
     * 
     * Configures SPI hardware for master mode, sets appropriate pin directions,
     * and configures clock speed.
     */
    void startMaster();
    
    /**
     * @brief Check if in master mode
     * @return true if configured as master
     */
    bool isMaster() const { return ((this->_statusRegister & SPI_MODE_MASK) == SPI_MODE_MASTER); }
    
    /**
     * @brief Send and receive command byte
     * @param command Byte to send; receives response byte on return
     * @return true if successful
     */
    bool sendCommand(uint8_t& command);
    
    /**
     * @brief Exchange command data with slave
     * 
     * Performs full-duplex SPI transfer with separate output and input buffers.
     * 
     * @param size Number of bytes to exchange
     * @param outbuffer Pointer to output data
     * @param inbuffer Pointer to input data buffer
     * @return true if successful
     */
    bool sendCommandData(uint8_t size, uint8_t* outbuffer,  uint8_t* inbuffer);

    /**
     * @brief Send single byte
     * @param data Byte to transmit
     * @return true if successful
     */
    bool send(uint8_t data);
    
    /**
     * @brief Send data array (not used in hardware SPI mode)
     * @param size Number of bytes to send
     * @param inbuffer Pointer to data buffer
     * @return true if successful
     */
    bool sendData(uint8_t size, uint8_t* inbuffer);
    
    /**
     * @brief Exchange data in-place (both send and receive buffer same)
     * 
     * Performs full-duplex SPI transfer where each byte sent is replaced
     * with the corresponding byte received.
     * 
     * @param size Number of bytes to exchange
     * @param inoutbuffer Pointer to combined send/receive buffer
     * @return true if successful
     */
    bool sendCommandData(uint8_t size, uint8_t* inoutbuffer);

private:
    /** @brief Internal state tracker */
    uint8_t _state;
    /** @brief SPI status register (volatile for hardware updates) */
    volatile uint8_t _statusRegister;
};



#endif /* __TINY_SPI_H__ */

