/**
 * @file nrf24mgr.h
 * @brief NRF24L01+ 2.4GHz RF Transceiver Manager for AVR Microcontrollers
 * 
 * This header defines the NRF24Manager class, which provides a high-level interface
 * for controlling NRF24L01+ wireless transceiver modules. The module uses SPI for
 * communication and GPIO pins for control (CE - Chip Enable, CSN - Chip Select).
 * 
 * Features:
 * - Fixed or dynamic payload sizes (0-32 bytes)
 * - Auto-acknowledgment with payload feedback in option
 * - 1 RX pipe + 1 TX pipe
 * - Use Enhanced ShockBurst features for reliable communication
 * 
 * @note Requires SPIManager and GPIO libraries for underlying communication
 */

#ifndef _NRF24Manager_H
#define _NRF24Manager_H

#include <stdint.h>
#include <stdlib.h>

/**
 * @defgroup NRF24_State Module Operating States
 * @brief States for the NRF24Manager radio module
 * @{
 */
//================================================================================
// NRF24L01+ State Definitions
// Used with changeState() to manage module operating modes
//================================================================================
#define NRF24_INIT 0      /**< Initial state (not initialized) */
#define NRF24_RECEIVE 3   /**< RX mode (PRIM_RX=1, listening for packets) */
#define NRF24_TRANSMIT 4  /**< TX mode (PRIM_RX=0, ready to transmit) */
/** @} */

/** @defgroup NRF24_Constants NRF24L01+ Constants and Limits */
/** @{ */
/** Maximum payload size per NRF24L01+ specification (32 bytes) */
#define NRF24_MAX_MESSAGE_SIZE 32

/**
 * GPIO port for CE and CSN control pins
 * @note Default: PORT A. Override on command line if using different port
 */
#ifndef NRF24_CX_PIN_PORT
#define NRF24_CX_PIN_PORT A
#endif

/**
 * @defgroup NRF24_Register_FIFO_Status FIFO_STATUS Register (0x17)
 * @brief FIFO (First-In-First-Out) status register bit positions
 * @{
 */
#define FIFO_STATUS_REG 0x17           /**< FIFO Status register address */
#define FIFO_STATUS_REG_TX_REUSE 6     /**< TX FIFO reuse flag */
#define FIFO_STATUS_REG_TX_FULL 5      /**< TX FIFO full flag */
#define FIFO_STATUS_REG_TX_EMPTY 4     /**< TX FIFO empty flag */
#define FIFO_STATUS_REG_RX_FULL 1      /**< RX FIFO full flag */
#define FIFO_STATUS_REG_RX_EMPTY 0     /**< RX FIFO empty flag */
/** @} */

/**
 * @defgroup NRF24_Register_Status STATUS Register (0x07)
 * @brief Radio status register bit positions
 * @{
 */
#define STATUS_REG 0x07                /**< STATUS register address */
#define STATUS_REG_RX_DR 6             /**< RX Data Ready interrupt flag */
#define STATUS_REG_TX_DS 5             /**< TX Data Sent interrupt flag */
#define STATUS_REG_MAX_RT 4            /**< Maximum Retransmits interrupt flag */
#define STATUS_REG_RX_P_NO_START_BIT 1 /**< RX pipe number start bit */
#define STATUS_REG_TX_FULL 0           /**< TX FIFO full flag */
/** @} */

/** @} */

class SPIManager;

/**
 * @def NRF24_DYNAMIC_PAYLOAD_SIZE
 * @brief Special payload size constant indicating dynamic payload mode
 * 
 * When NRF24Manager is initialized with this value (-1), the module operates
 * in dynamic payload mode where packets can be 0-32 bytes in length. The actual
 * packet size is determined per-packet rather than fixed at initialization.
 * With this mode it is possible to use ACK payloads (enhanced acknowledgment with data feedback).
 */
const int8_t NRF24_DYNAMIC_PAYLOAD_SIZE = -1;

/**
 * @class NRF24Manager
 * @brief High-level manager for NRF24L01+ 2.4GHz RF Transceiver
 * 
 * This class provides a complete interface for NRF24L01+ operations including:
 * - Module initialization and configuration
 * - RX/TX mode switching
 * - Packet transmission and reception
 * - ACK payload handling (enhanced acknowledgment with data feedback)
 * - FIFO management (flush operations)
 * - Register access for advanced configuration
 * 
 * The module uses:
 * - SPI for high-speed command/data transfer
 * - CE (Chip Enable) GPIO for RX/TX activation
 * - CSN (Chip Select) GPIO for SPI chip selection
 * 
 * @note Supports both fixed-size and dynamic payload modes
 * @note Use only pipe 0 and pipe 1. 1 to receive, 0 to transmit. This simplifies auto-ack and ACK payload handling.
 */
class NRF24Manager {
public:
  /**
   * @brief Constructor for NRF24Manager
   * 
   * Initializes the manager with specified payload size. Default is 32-byte
   * fixed payloads. Use NRF24_DYNAMIC_PAYLOAD_SIZE (-1) for variable-length packets.
   * 
   * @param payloadSize Size of fixed payloads in bytes (1-32), or NRF24_DYNAMIC_PAYLOAD_SIZE
   *                    for variable-length packets. Default: 32
   * 
   * @note This constructor does not initialize the hardware. Call init() after
   *       object creation to configure the RF module.
   */
  NRF24Manager(int8_t payloadSize) : _payloadSize(payloadSize), _ce_pin(1), _cs_pin(1), _spi(NULL) {}

  /**
   * @brief Set the destination address for TX operations
   * 
   * Configures the TX address where packets will be sent. For auto-acknowledgment
   * to function correctly, this address must match the receiver's RX_ADDR_P0.
   * 
   * @param address Pointer to 5-byte destination address array
   * 
   * @note Call it before init() to set the address during initialization.
   * @note RX_ADDR_P0 is automatically set to match TX address for ACK reception
   */
  void setDestinationAddress(const char address[5]);

  /**
   * @brief Set this module's RX listening address
   * 
   * Configures the address this module listens on for incoming packets
   * (RX_ADDR_P1). This address is used for data reception.
   * 
   * @param address Pointer to 5-byte local address array
   * 
   * @note Call it before init() to set the address during initialization.
   *
   */
  void setMyAddress(const char address[5]);

  /**
   * @brief Clear all pending RX packets from FIFO
   * 
   * Flushes the RX FIFO, discarding all received packets that haven't been
   * read yet. Useful for clearing stale data before switching modes.
   * 
   * @note Should be called before entering RX mode or after changeState()
   */
  void flushRX();

  /**
   * @brief Clear all pending TX packets from FIFO
   * 
   * Flushes the TX FIFO, discarding all unsent packets. Useful for clearing
   * queued transmissions before changing modes or recovering from errors.
   */
  void flushTX();
 
  /**
   * @brief Initialize and configure the NRF24L01+ module
   * 
   * Performs complete hardware initialization including:
   * - GPIO configuration (CE, CSN pins)
   * - SPI setup via SPIManager
   * - Module power-up and stabilization (200ms delay)
   * - Auto-acknowledgment configuration
   * - Address setup (RX/TX)
   * - FIFO configuration
   * - RF channel, data rate, and power settings
   * - Dynamic payload enablement (if configured)
   * 
   * @param s Pointer to initialized SPIManager instance for SPI communication
   * @param ce_pin GPIO pin number connected to NRF24 CE (Chip Enable)
   * @param cs_pin GPIO pin number connected to NRF24 CSN (SPI Chip Select)
   * 
   * @note This must be called before any other operations
   * @note Critical 200ms stabilization delay is applied after power-up
   * @note Module is left in RX mode (CE=HIGH) after initialization
   * 
   * @warning Ensure SPIManager is already initialized before calling this
   */
  void init(SPIManager *s, uint8_t ce_pin, uint8_t cs_pin);

  /**
   * @brief Check if data is available in RX FIFO
   * 
   * Reads the STATUS register to determine which RX pipe has data waiting.
   * Used to poll for incoming packets without blocking.
   * 
   * @return Pipe number (0-1) if data is available, -1 if FIFO is empty
   * 
   * @note Non-blocking function. Returns immediately.
   * @note Call read_binary_message() to retrieve the actual packet data
   * 
   * @see read_binary_message()
   */
  int8_t dataAvailable();

  /**
   * @brief Read a received packet from RX FIFO
   * 
   * Retrieves the oldest packet from the RX FIFO. The packet length is
   * automatically determined if dynamic payload is enabled, otherwise uses
   * the configured payload size.
   * 
   * @param[out] length Reference to byte variable that receives packet length.
   *                    Contains valid length only if return value is not NULL.
   * 
   * @return Pointer to packet data buffer, or NULL if FIFO is empty.
   *         Buffer size is determined by length parameter.
   * 
   * @note Packet buffer is valid until next read_binary_message() call
   * @note Call dataAvailable() first to verify data exists
   * 
   * @see dataAvailable(), send_binary()
   */
  uint8_t* read_binary_message(uint8_t& length);

  /**
   * @brief Transmit a binary packet
   * 
   * Sends a packet to the configured destination address and waits for
   * auto-acknowledgment. Automatically retransmits on failure (up to configured
   * retry count).
   * 
   * @param msg Pointer to packet data to transmit
   * @param length Input: packet size in bytes; Output: actual bytes sent (may differ on error)
   * 
   * @return Pointer to any ACK payload received from destination, or NULL if no ACK
   * 
   * @note Module must be in NRF24_TRANSMIT state before calling
   * @note Blocks until transmission completes or max retries exceeded
   * @note If destination sets an ACK payload, it's returned in the buffer
   * 
   * @see set_ack_buffer(), changeState()
   */
  uint8_t* send_binary(uint8_t *msg, uint8_t &length);

  /**
   * @brief Configure custom payload to send with ACK packets
   * 
   * When this module receives a packet with auto-acknowledgment enabled,
   * it automatically sends back an ACK. This function sets custom data to
   * be included in that ACK response (max 32 bytes).
   * 
   * @param msg Pointer to data to include in ACK payload
   * @param length Number of bytes to send in ACK (0-32)
   * 
   * @note Requires EN_ACK_PAY feature to be enabled (automatic with dynamic payload)
   * @note ACK payload is queued on pipe 1 and persists until overwritten
   * @note Module must be in RX mode for ACK payload to be sent
   * 
   * @see isDynamicPayload()
   */
  void set_ack_buffer(uint8_t *msg, uint8_t length);

  /**
   * @brief Print module information/statistics
   * 
   * Outputs detailed diagnostic information including register contents,
   * configuration settings, transmission statistics, and FIFO status.
   * Output format varies depending on HAS_SERIAL compile flag.
   * 
   * @note Requires serial output support (configured via HAS_SERIAL or HAS_INT0_SERIAL)
   * @note Non-blocking function
   * 
   * @see summary()
   */
  void info();

  /**
   * @brief Print module summary/status
   * 
   * Outputs a brief status summary including current state, addresses,
   * and packet statistics.
   * 
   * @note Requires serial output support (configured via HAS_SERIAL or HAS_INT0_SERIAL)
   * @note Lighter output than info()
   * 
   * @see info()
   */
  void summary();

  /**
   * @brief Query if module uses dynamic payload mode
   * 
   * Determines whether the module is configured for variable-length packets
   * (dynamic mode, size=-1) or fixed-size packets.
   * 
   * @return true if dynamic payload is enabled, false for fixed-size mode
   * 
   * @see NRF24_DYNAMIC_PAYLOAD_SIZE
   */
  bool isDynamicPayload() const { return (this->_payloadSize == -1); }

  
private:
  /**
   * @brief Change the module operating state (RX/TX mode)
   * 
   * Switches between receive and transmit modes, or initializes the module.
   * 
   * @param state Operating state: NRF24_RECEIVE, or NRF24_TRANSMIT
   * 
   * @note NRF24_RECEIVE: Enables RF receiver, listens on configured RX addresses
   * @note NRF24_TRANSMIT: Disables receiver, prepares module for packet transmission
   * @note Module must be initialized (init) before changing states
   * @note Dynamic payload must be enabled before setting this state
   * 
   * @see NRF24_RECEIVE, NRF24_TRANSMIT
   */
  void changeState(uint8_t state);

  /**
   * @brief Set CE (Chip Enable) pin LOW
   * 
   * Disables RF receiver and stops transmission. Used internally to
   * control module operating state.
   */
  void celow();

  /**
   * @brief Set CE (Chip Enable) pin HIGH
   * 
   * Activates RF receiver (if in RX mode) or initiates transmission (if in TX mode).
   * Used internally to control module operating state.
   */
  void cehigh();

  /**
   * @brief Send SPI command to NRF24 module
   * 
   * Low-level SPI communication primitive. Handles CS selection, command
   * transmission, data transfer, and CS deselection.
   * 
   * @param cmd SPI command byte
   * @param data Pointer to data buffer (command payload)
   * @param size Number of bytes in data buffer
   * 
   * @return STATUS register value returned during SPI transaction
   */
  uint8_t send_spi(uint8_t cmd, uint8_t *data, uint8_t size);

  /**
   * @brief Read a hardware register value
   * 
   * Low-level function to read NRF24L01+ hardware registers. Useful for
   * diagnostics and advanced configuration.
   * 
   * @param reg Register address (0x00-0x1D)
   * @param data Pointer to buffer to receive register contents
   * @param size Number of bytes to read from register
   * 
   * @return STATUS register value at time of read
   * 
   * @note Public access for diagnostics only; most users should use higher-level APIs
   * 
   * @see writeRegister() (private)
   */
  uint8_t readRegister(uint8_t reg, uint8_t *data, uint8_t size);

  /**
   * @brief Write to a NRF24 hardware register
   * 
   * Sends a write command to the specified register address via SPI.
   * 
   * @param reg Register address to write to
   * @param data Pointer to bytes to write
   * @param size Number of bytes to write
   * 
   * @return STATUS register value at time of write
   */
  uint8_t writeRegister(uint8_t reg, uint8_t *data, uint8_t size);

  // ===== Internal State =====
  
  /** Payload size configuration: positive value for fixed, -1 for dynamic */
  int8_t _payloadSize;
  
  /** GPIO pin number for CE (Chip Enable) control */
  uint8_t _ce_pin;
  
  /** GPIO pin number for SPI CS (Chip Select) */
  uint8_t _cs_pin;
  
  /** Pointer to SPIManager instance for hardware communication */
  SPIManager *_spi;
};

#endif
