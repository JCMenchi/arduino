#ifndef _NRF24Manager_H
#define _NRF24Manager_H

#include <stdint.h>
#include <stdlib.h>

//================================================================================
// NRF24L01+ State Definitions
// Used with changeState() to manage module operating modes
//================================================================================
#define NRF24_INIT 0      // Initial state (not initialized)
#define NRF24_POWERUP 1   // Power-on state (enables internal oscillator)
#define NRF24_POWERDOWN 2 // Power-down state (low current, ~22µA)
#define NRF24_RECEIVE 3   // RX mode (PRIM_RX=1, listening for packets)
#define NRF24_TRANSMIT 4  // TX mode (PRIM_RX=0, ready to transmit)
#define NRF24_STANDBY1 5  // Standby mode 1 (CE low, minimal current)
#define NRF24_STANDBY2 6  // Standby mode 2 (CE high, TX standby ~340µA)

// Maximum payload size per NRF24L01+ specification
#define NRF24_MAX_MESSAGE_SIZE 32

// Port for CE and CSN GPIO pins (default: PORT A, override in platform config)
#ifndef NRF24_CX_PIN_PORT
#define NRF24_CX_PIN_PORT A
#endif

class SPIManager;

/**
 * @class NRF24Manager
 * @brief Driver for NRF24L01+ 2.4GHz wireless transceiver module
 * 
 * Key Features:
 *  - SPI-based communication with AVR microcontrollers
 *  - Supports both transmit and receive modes
 *  - Auto-acknowledgment mode with configurable retries
 *  - Dynamic payload length support (up to 32 bytes)
 *  - Power management (full power, standby, or power-down)
 *  - Status register monitoring and interrupt handling
 * 
 * Hardware Requirements:
 *  - 3.3V stable power supply with 10µF+ bypass capacitor
 *  - Level shifting for 5V AVR logic (required on pins: CE, CSN, SCK, MOSI)
 *  - Standard 6-pin SPI interface (SCK, MOSI, MISO)
 *  - 2 GPIO pins for CE (Chip Enable) and CSN (Chip Select)
 * 
 * Usage Example:
 *  SPIManager spi;
 *  NRF24Manager radio(1);  // 1 = AutoACK enabled
 *  
 *  spi.startMaster();
 *  radio.init(&spi, CE_PIN, CSN_PIN);
 *  radio.changeState(NRF24_POWERUP);
 *  radio.listen();
 *  
 *  if (radio.dataAvailable()) {
 *    const char *msg = radio.read_message();
 *    // Process msg...
 *  }
 */
class NRF24Manager {
public:
  /**
   * @brief Constructor
   * @param autoack Enable (1) or disable (0) auto-acknowledgment feature
   */
  NRF24Manager(uint8_t autoack) : _state(NRF24_INIT), _ce_pin(1), _cs_pin(1), _spi(NULL), _autoack(autoack) {}

  /**
   * @brief Initialize the NRF24L01+ module
   * @param s Pointer to initialized SPIManager instance
   * @param ce_pin GPIO pin number for CE control
   * @param cs_pin GPIO pin number for CS (SPI slave select)
   */
  void init(SPIManager *s, uint8_t ce_pin, uint8_t cs_pin);

  /**
   * @brief Change module operating state (power, RX/TX mode, etc.)
   * @param state Target state (see NRF24_* defines above)
   */
  void changeState(uint8_t state);
  
  /**
   * @brief Get current operating state
   * @return Current state (NRF24_INIT, NRF24_POWERUP, etc.)
   */
  uint8_t state() const { return this->_state; }

  /**
   * @brief Activate receive mode and start listening for packets
   */
  void listen();
  
  /**
   * @brief Check if data is available in RX FIFO
   * @return 1 if data available, 0 if empty
   * 
   * Non-blocking check; call read_message() or read_binary_message() to retrieve data
   */
  uint8_t dataAvailable();

  /**
   * @brief Read text message from RX FIFO
   * @return Pointer to NUL-terminated message string, or NULL if empty
   * 
   * WARNING: Returns pointer to static buffer that gets overwritten on next call!
   * Copy the message immediately if you need to preserve it.
   */
  const char *read_message();
  
  /**
   * @brief Read binary message from RX FIFO
   * @param length OUT parameter: size of received data
   * @return Pointer to binary data buffer, or NULL if empty
   * 
   * WARNING: Same as read_message() - returns static buffer overwritten on next call!
   * The 'length' parameter is set to the actual bytes received.
   */
  uint8_t* read_binary_message(uint8_t& length);
  
  /**
   * @brief Send text message (string)
   * @param msg Pointer to NUL-terminated string (max 32 bytes including NUL)
   * @return 1 on success, 0 on failure
   * 
   * Switches to TX mode, sends message, and waits for completion (if AutoACK disabled)
   */
  uint8_t send(const char *msg);
  
  /**
   * @brief Send binary data
   * @param msg Pointer to binary data buffer
   * @param length Number of bytes to send (max 32)
   * @return 1 on success
   * 
   * Like send() but accepts arbitrary binary payload without NUL termination
   */
  uint8_t send_binary(uint8_t *msg, uint8_t length);

  /**
   * @brief Print status register values (requires USART)
   * 
   * Prints: CONFIG, STATUS, OBSERVE_TX, RPD, FIFO_STATUS
   */
  void summary();
  
  /**
   * @brief Print detailed register contents (requires USART)
   * 
   * Prints all configuration registers and addresses
   */
  void info();

  /**
   * @brief Reset module configuration
   * @param autoack Enable (1) or disable (0) auto-acknowledgment
   * 
   * Re-initializes key registers without power cycle
   */
  void reset(uint8_t autoack);

private:
  // CE (Chip Enable) control - activates receiver or starts transmission
  void celow();   // CE = 0 (disable transmit/receive)
  void cehigh();  // CE = 1 (enable transmit/receive)
  
  // ACK payload (for AutoACK mode)
  void ack();
  
  // Low-level SPI communication
  uint8_t send_spi(uint8_t cmd, uint8_t *data, uint8_t size);
  uint8_t writeRegister(uint8_t reg, uint8_t *data, uint8_t size);
  uint8_t readRegister(uint8_t reg, uint8_t *data, uint8_t size);

  // Internal state
  uint8_t _state;        // Current operating state
  uint8_t _ce_pin;       // GPIO pin for CE control
  uint8_t _cs_pin;       // GPIO pin for SPI CS (chip select)
  SPIManager *_spi;      // Pointer to SPI manager
  uint8_t _autoack;      // AutoACK enabled/disabled flag
};

#endif
