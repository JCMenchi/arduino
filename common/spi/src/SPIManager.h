/*! \file SPIManager.h
 *  \brief SPI Master/Slave manager for AVR microcontrollers
 *
 *  Provides a flexible interface for Serial Peripheral Interface (SPI) communication
 *  on AVR microcontrollers. Supports both master and slave modes with optional
 *  hardware or software implementation.
 *
 */

#ifndef __TINY_SPI_H__
#define __TINY_SPI_H__

#include <avr/common.h>

/*! \defgroup SPIState SPI State Constants
 *  @{
 */
const uint8_t SPI_INIT = 1;          //!< Initial state
const uint8_t SPI_WAIT_MASTER = 2;   //!< Waiting for master
const uint8_t SPI_WAIT_COMMAND = 3;  //!< Waiting for command
const uint8_t SPI_WAIT_DATA = 4;     //!< Waiting for data
/*! @} */

/*! \defgroup SPIMode SPI Mode Masks and Definitions
 *  @{
 */
#define SPI_MODE_MASK 0x03   //!< Bitmask to extract mode bits
#define SPI_MODE_MASTER 0x03 //!< Master mode identifier
#define SPI_MODE_SLAVE 0x01  //!< Slave mode identifier

#define SPI_COMMAND_MASK 0xE0       //!< Bitmask for command bits
#define SPI_COMMAND_SIZE_MASK 0x1F  //!< Bitmask for command size
/*! @} */

/*! \class SPIManager
 *  \brief Serial Peripheral Interface (SPI) Manager
 *
 *  Manages SPI communication for both master and slave devices.
 *  Supports hardware SPI and software SPI implementations.
 *
 *  \par Configuration Macros:
 *  - Define `SOFTWARE_SPI` before compiling to use software SPI
 *  - Define `SLOW_SPI` to reduce clock speed to CPU_FREQ/64
 *
 *  \par Supported MCUs:
 *  - ATmega328P (Arduino UNO)
 *  - ATmega1284P
 *  - ATmega8535
 */
class SPIManager {
   public:
    /*! \brief Constructor
     *
     *  Initializes the SPI manager with initial state and zero status register.
     */
    SPIManager() : _state(SPI_INIT), _statusRegister(0) {}

    /*! \brief Get current SPI state
     *  \return Current state value (SPI_INIT, SPI_WAIT_MASTER, etc.)
     */
    uint8_t getState() const { return this->_state; }

    /*! \brief Set SPI state
     *  \param s New state value
     */
    void setState(uint8_t s) { this->_state = s; }

    /*! \brief Get SPI status register
     *  \return Status register value indicating mode and state
     */
    uint8_t getStatusRegister() const { return this->_statusRegister; }

    /*! \defgroup SlaveMethods Slave Mode Methods
     *  @{
     */

    /*! \brief Initialize SPI as slave
     *
     *  Configures the SPI module for slave mode. Sets MISO as output,
     *  and MOSI, SCK, and CS as inputs. Enables pull-up on CS.
     */
    void startSlave();

    /*! \brief Check if in slave mode
     *  \return true if currently in slave mode, false otherwise
     */
    bool isSlave() const { return ((this->_statusRegister & SPI_MODE_MASK) == SPI_MODE_SLAVE); }

    /*! \brief Receive command byte from master
     *
     *  Waits for a complete byte transfer from the master device.
     *  The function blocks until either a complete byte is received or
     *  the chip select is released (deasserted high).
     *
     *  \param[out] command Reference to store received command byte
     *  \return true if command received successfully, false if CS was released prematurely
     */
    bool receiveCommand(uint8_t& command);

    /*! \brief Execute bidirectional command with data exchange
     *
     *  Simultaneously sends response data while receiving command data from master.
     *  Each byte transfer completes before the next begins.
     *
     *  \param[in] size Number of bytes to exchange
     *  \param[in] outbuffer Pointer to data to send to master
     *  \param[out] inbuffer Pointer to buffer for received data from master
     *  \return true if all bytes exchanged successfully, false if CS was released prematurely
     */
    bool execCommand(uint8_t size, uint8_t* outbuffer, uint8_t* inbuffer);

    /*! @} */

    /*! \defgroup MasterMethods Master Mode Methods
     *  @{
     */

    /*! \brief Initialize SPI as master
     *
     *  Configures the SPI module for master mode. Sets MISO as input,
     *  MOSI, SCK, and CS as outputs. Initializes output levels.
     */
    void startMaster();

    /*! \brief Check if in master mode
     *  \return true if currently in master mode, false otherwise
     */
    bool isMaster() const { return ((this->_statusRegister & SPI_MODE_MASK) == SPI_MODE_MASTER); }

    /*! \brief Assert chip select before transmission
     *
     *  Pulls the chip select line low to begin a transaction with a slave device.
     *  Should be called before sending data.
     *
     *  \param[in] cspin Chip select pin number
     */
    void begin(uint8_t cspin);

    /*! \brief Release chip select after transmission
     *
     *  Pulls the chip select line high to end a transaction with a slave device.
     *  Should be called after sending all data.
     *
     *  \param[in] cspin Chip select pin number
     */
    void end(uint8_t cspin);

    /*! \brief Send single byte to slave
     *
     *  Transmits a single byte over SPI. Blocks until transmission is complete.
     *
     *  \param[in] data Byte to send
     *  \return true if send successful, false if not in master mode
     */
    bool send(uint8_t data);

    /*! \brief Send multiple bytes to slave
     *
     *  Transmits a sequence of bytes over SPI. Each byte transfer blocks
     *  until complete before starting the next byte.
     *
     *  \param[in] size Number of bytes to send
     *  \param[in] inbuffer Pointer to buffer containing bytes to send
     *  \return true if successful, false if not in master mode
     */
    bool sendData(uint8_t size, uint8_t* inbuffer);

    /*! \brief Send command and receive response (full-duplex)
     *
     *  Simultaneously sends a command byte and receives response data.
     *  The received byte overwrites the command variable.
     *
     *  \param[in,out] command On input, command byte to send; on output, received byte
     *  \return true if successful, false if not in master mode
     */
    bool sendCommand(uint8_t& command);

    /*! \brief Send data and receive data (full-duplex with separate buffers)
     *
     *  Simultaneously sends data from outbuffer while receiving data into inbuffer.
     *  Byte-by-byte transfer with blocking.
     *
     *  \param[in] size Number of bytes to exchange
     *  \param[in] outbuffer Pointer to bytes to send
     *  \param[out] inbuffer Pointer to buffer for received bytes
     *  \return true if successful, false if not in master mode
     */
    bool sendCommandData(uint8_t size, uint8_t* outbuffer, uint8_t* inbuffer);

    /*! \brief Send/receive data (full-duplex with single buffer)
     *
     *  Performs bidirectional SPI transfer using a single buffer.
     *  Outgoing data is overwritten with incoming data during transfer.
     *
     *  \param[in] size Number of bytes to exchange
     *  \param[in,out] inoutbuffer Pointer to buffer with outgoing data;
     *                             will contain received data after completion
     *  \return true if successful, false if not in master mode
     */
    bool sendCommandData(uint8_t size, uint8_t* inoutbuffer);

    /*! @} */

    /*! \defgroup SpecialMethods Special Purpose Methods
     *  @{
     */

    /*! \brief Configure MOSI pin as input
     *
     *  Switches MOSI from output to input mode. Disables hardware SPI.
     *  Used for reading data from devices like TFT display controllers
     *  that drive the MOSI line.
     */
    void setMosiAsInput();

    /*! \brief Configure MOSI pin as output
     *
     *  Switches MOSI back to output mode. In software SPI mode, just sets
     *  the pin as output. In hardware SPI mode, re-initializes master mode.
     */
    void setMosiAsOutput();

    /*! \brief Generate one SPI clock cycle
     *
     *  Generates a complete clock cycle (low-to-high-to-low transition)
     *  on the SCK line. Useful for clocking data into devices that
     *  require manual clock generation.
     */
    void dummyClock();

    /*! \brief Read 8 bits from MOSI pin
     *
     *  Reads 8 bits serially from the MOSI line, using SCK to clock the data.
     *  Data is sampled on the rising edge of the clock.
     *  Bit order is MSB first.
     *
     *  \return 8-bit value read from MOSI with clock generation
     */
    uint8_t readFromMosi();

    /*! @} */

   private:
    uint8_t _state;                  //!< Internal state machine state
    volatile uint8_t _statusRegister; //!< SPI status (mode bits and flags)
};

#endif /* __TINY_SPI_H__ */
