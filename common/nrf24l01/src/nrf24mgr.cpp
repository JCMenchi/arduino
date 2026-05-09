/**
 * @file nrf24mgr.cpp
 * @brief NRF24L01+ 2.4GHz RF Transceiver Manager Implementation
 * 
 * This file contains the implementation of the NRF24Manager class, providing
 * low-level hardware control and high-level packet communication interface
 * for NRF24L01+ wireless modules on AVR microcontrollers.
 * 
 * Key features:
 * - SPI command interface for hardware register access
 * - GPIO-based CE (Chip Enable) control
 * - FIFO packet buffering (TX/RX)
 * - Auto-acknowledgment with payload feedback
 * - Dynamic or fixed-size payload modes
 * - RX pipe multiplexing (6 receive pipes)
 * 
 * Hardware Communication:
 * - SPI: Command/data transfer to NRF24 registers and FIFOs
 * - GPIO: CE pin for RF activation, CSN for SPI chip select
 * 
 * Status Register:
 * - Bit 6: RX_DR (Data Ready interrupt)
 * - Bit 5: TX_DS (Data Sent interrupt)
 * - Bit 4: MAX_RT (Max Retransmits interrupt)
 * - Bits 3-1: RX_P_NO (RX pipe number with data)
 * - Bit 0: TX_FULL (TX FIFO full flag)
 * 
 * 
 * @see nrf24mgr.h for public interface documentation
 */

// AVR
#include <avr/interrupt.h>
#include <avr/io.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <util/delay.h>
#include <avr/pgmspace.h>

// external lib
#include <tinyspi.h>
#include <gpio.h>

//#define HAS_SERIAL
//#define HAS_INT0_SERIAL

#ifdef HAS_SERIAL
#include <usart_serial.h>
#endif

#ifdef HAS_INT0_SERIAL
#include <int0_serial.h>
#endif

// include files
#include "nrf24mgr.h"

//=========================================================================================
// HARDWARE REGISTER DEFINITIONS
// Memory-mapped register addresses and bit positions for NRF24L01+ control
// Communication is via SPI: Read command (R_REGISTER | addr) or Write command (W_REGISTER | addr)
//=========================================================================================
// Register Addresses

// CONFIG: configuration register
#define CONFIG_REG 0x00
#define CONFIG_REG_MASK_RX_DR 6
#define CONFIG_REG_MASK_TX_DS 5
#define CONFIG_REG_MASK_MAX_RT 4
#define CONFIG_REG_EN_CRC 3
#define CONFIG_REG_CRC0 2
#define CONFIG_REG_PWR_UP 1
#define CONFIG_REG_PRIM_RX 0

// EN_AA: Enhanced ShockBurst
// Enable 'Auto Acknowledgment' function.  Disable this functionality
// to be compatible with nRF2401.
#define EN_AA_REG 0x01
#define EN_AA_REG_ENAA_P5 5
#define EN_AA_REG_ENAA_P4 4
#define EN_AA_REG_ENAA_P3 3
#define EN_AA_REG_ENAA_P2 2
#define EN_AA_REG_ENAA_P1 1
#define EN_AA_REG_ENAA_P0 0

// EN_RXADDR: enable RX addresses
#define EN_RXADDR_REG 0x02

// SETUP_AW: setup of address widths
#define SETUP_AW_REG 0x03
#define SETUP_AW_REG_AW_3BYTES 0x01
#define SETUP_AW_REG_AW_4BYTES 0x02
#define SETUP_AW_REG_AW_5BYTES 0x03

// SETUP_RETR: setup of automatic retransmission
#define SETUP_RETR_REG 0x04

// RF Channel
#define RF_CH_REG 0x05

// RF_SETUP: RF setup register
#define RF_SETUP_REG 0x06
#define RF_SETUP_REG_CONT_WAVE 7
#define RF_SETUP_REG_RF_DR_LOW 5
#define RF_SETUP_REG_PLL_LOCK 4
#define RF_SETUP_REG_RF_DR_HIGH 3

#define RF_SETUP_REG_RF_PWR_0dBm 0x06
#define RF_SETUP_REG_RF_PWR_6dBm 0x04
#define RF_SETUP_REG_RF_PWR_12dBm 0x02
#define RF_SETUP_REG_RF_PWR_18dBm 0x00



#define OBSERVE_TX_REG 0x08
#define RPD_REG 0x09

// pipe address
#define RX_ADDR_P0_REG 0x0A
#define RX_ADDR_P1_REG 0x0B
#define RX_ADDR_P2_REG 0x0C
#define RX_ADDR_P3_REG 0x0D
#define RX_ADDR_P4_REG 0x0E
#define RX_ADDR_P5_REG 0x0F
#define TX_ADDR_REG 0x10

// receive buffer
#define RX_PW_P0_REG 0x11
#define RX_PW_P1_REG 0x12
#define RX_PW_P2_REG 0x13
#define RX_PW_P3_REG 0x14
#define RX_PW_P4_REG 0x15
#define RX_PW_P5_REG 0x16


// DYNPD: enable dynamic payload length
#define DYNPD_REG 0x1C
#define DYNPD_REG_DPL_P5 5
#define DYNPD_REG_DPL_P4 4
#define DYNPD_REG_DPL_P3 3
#define DYNPD_REG_DPL_P2 2
#define DYNPD_REG_DPL_P1 1
#define DYNPD_REG_DPL_P0 0

// FEATURE:
#define FEATURE_REG 0x1D
#define FEATURE_REG_EN_DPL 2
#define FEATURE_REG_EN_ACK_PAY 1
#define FEATURE_REG_EN_DYN_ACK 0

//=========================================================================================
// NRF24L01+ COMMAND DEFINITIONS
// Low-level commands sent over SPI to interact with NRF24L01+ hardware
//=========================================================================================
#define NRF24CMD_ADDRESS_MASK 0x1F /* 000A AAAA */
#define NRF24CMD_R_REGISTER 0x00
#define NRF24CMD_W_REGISTER 0b00100000
#define NRF24CMD_R_RX_PAYLOAD 0b01100001
#define NRF24CMD_W_TX_PAYLOAD 0b10100000
#define NRF24CMD_FLUSH_TX 0b11100001
#define NRF24CMD_FLUSH_RX 0b11100010
#define NRF24CMD_REUSE_TX_PL 0b11100011
#define NRF24CMD_ACTIVATE 0b01010000 // only on non + variants, not needed for our purposes
#define NRF24CMD_R_RX_PL_WID 0b01100000
#define NRF24CMD_ACK_PAYLOAD_MASK 0b00000111
#define NRF24CMD_W_ACK_PAYLOAD 0b10101000 /* 1010 1PPP | PPP = pipe number */
#define NRF24CMD_W_TX_PAYLOAD_NOACK 0b10110000
#define NRF24CMD_NOP 0xFF

//=========================================================================================
// RADIO ADDRESS CONFIGURATION
// These addresses determine which node can communicate with this module.
// For auto-acknowledgment to work, RX_ADDR_P0 must match the transmitter's TX_ADDR.
//=========================================================================================
// RADIO address (5 bytes) - addresses must match between sender and receiver
// tx_radio_address is used for RX_ADDR_P0 and TX_ADDR
// radio_address is used for RX_ADDR_P1
static uint8_t radio_address[5] = {'1', 'N', 'o', 'd', 'e'}; // 5-byte address for both TX and RX (must match for auto-ack to work)
static uint8_t tx_radio_address[5] = {'2', 'N', 'o', 'd', 'e'};

/**
 * @brief Set CE (Chip Enable) pin LOW
 * 
 * Disables RF receiver and stops transmission. Used internally to
 * control module operating state.
 */
void NRF24Manager::celow() { GPIO_SET_LOW(NRF24_CX_PIN_PORT, this->_ce_pin); }

/**
 * @brief Set CE (Chip Enable) pin HIGH
 * 
 * Activates RF receiver (if in RX mode) or initiates transmission (if in TX mode).
 * Used internally to control module operating state.
 */
void NRF24Manager::cehigh() { GPIO_SET_HIGH(NRF24_CX_PIN_PORT, this->_ce_pin); }

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
uint8_t NRF24Manager::send_spi(uint8_t cmd, uint8_t *data, uint8_t size) {
  uint8_t exchange = cmd;

  this->_spi->begin(this->_cs_pin);
  this->_spi->sendCommand(exchange);
  
  if (size > 0) {
    this->_spi->sendCommandData(size, data);
  }
  this->_spi->end(this->_cs_pin);

  return exchange; // nRF24L01 STATUS register value is returned during the command exchange (first byte)
}

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
uint8_t NRF24Manager::writeRegister(uint8_t reg, uint8_t *data, uint8_t size) {
  uint8_t status = this->send_spi(NRF24CMD_W_REGISTER | reg, data, size);
  return status;  // Return statement was after delay
}

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
uint8_t NRF24Manager::readRegister(uint8_t reg, uint8_t *data, uint8_t size) {
  return this->send_spi(NRF24CMD_R_REGISTER | reg, data, size);
}

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
void NRF24Manager::setDestinationAddress(const char address[5]) {
  memcpy(tx_radio_address, address, 5);
}

/**
 * @brief Set this module's RX listening address
 * 
 * Configures the address this module listens on for incoming packets
 * (RX_ADDR_P1). This address is used for data reception.
 * 
 * @param address Pointer to 5-byte local address array
 * 
 * @note Call it before init() to set the address during initialization.
 */
void NRF24Manager::setMyAddress(const char address[5]) {
  memcpy(radio_address, address, 5);
}

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
void NRF24Manager::init(SPIManager *s, uint8_t ce_pin, uint8_t cs_pin) {
  this->_ce_pin = ce_pin;
  this->_cs_pin = cs_pin;
  this->_spi = s;

#ifdef HAS_INT0
  cli(); // Disable interrupts for critical section
#endif

  // Configure CE pin as output and set LOW (disable transmit/receive)
  GPIO_OUTPUT(NRF24_CX_PIN_PORT, this->_ce_pin);
  this->celow();

  // CRITICAL: NRF24 requires >100ms after power-on to be stable
  // Extended delay recommended for slower AVR designs
  _delay_ms(200);

  // local variable for command construction,
  // data are modifiied in-place for SPI transfer, so we use this variable to not override the original data buffers
  uint8_t cmd = 0;

  // ===== Auto-Acknowledgment Setup =====
  // Enhanced ShockBurst: automatic ACK on received packets
  cmd = (0 << EN_AA_REG_ENAA_P5) | (0 << EN_AA_REG_ENAA_P4) |
        (0 << EN_AA_REG_ENAA_P3) | (0 << EN_AA_REG_ENAA_P2) |
        (1 << EN_AA_REG_ENAA_P1) | (1 << EN_AA_REG_ENAA_P0);
  this->writeRegister(EN_AA_REG, &cmd, 1);
  _delay_ms(2);
  
  // ===== Address Setup =====
  cmd = SETUP_AW_REG_AW_5BYTES;
  this->writeRegister(SETUP_AW_REG, &cmd, 1);
  _delay_ms(2);

  // ===== Automatic Retransmission Setup =====
  // ARD (Auto Retransmit Delay) and ARC (Auto Retransmit Count)
  cmd = 0x41; // ARD=0011 (ARD*250µs delay), ARC=0001 (1 retry attempt)
  this->writeRegister(SETUP_RETR_REG, &cmd, 1);
  _delay_ms(2);

  // ===== RF Channel Configuration =====
  // Channel frequency = 2400 + RF_CH (MHz)
  // Valid range: 0-125 (2400-2525 MHz) with 25 channels available in most regions
  cmd = 1; // Channel 100 (2500 MHz, center of 2.4GHz band)
  this->writeRegister(RF_CH_REG, &cmd, 1);
  _delay_ms(2);

  // ===== RF Setup (Data Rate and TX Power) =====
  // Default configuration for 1 Mbps (00) (01 for 2 Mbps, 10 for 250 kbps)
  cmd = (0 << RF_SETUP_REG_RF_DR_LOW) | (1 << RF_SETUP_REG_RF_DR_HIGH) | RF_SETUP_REG_RF_PWR_0dBm;
  this->writeRegister(RF_SETUP_REG, &cmd, 1);
  _delay_ms(2);

  // ===== Set RX/TX Addresses =====
  // P0 is used for both RX (when listening) and TX acks
  // TX_ADDR must match RX_ADDR_P0 for auto-ack to work correctly
  this->writeRegister(RX_ADDR_P1_REG, radio_address, 5);
  uint8_t poaddr[5];
  memcpy(poaddr, tx_radio_address, 5);
  this->writeRegister(RX_ADDR_P0_REG, poaddr, 5);
  this->writeRegister(TX_ADDR_REG, tx_radio_address, 5);
  _delay_ms(2);

  // ===== Enable RX Pipes =====
  // Only enable pipe 0 & 1 (bits correspond to pipes P0-P5)
  cmd = 0x03;
  this->writeRegister(EN_RXADDR_REG, &cmd, 1);
  _delay_ms(2);

  // ===== Set Payload Size =====
  if (this->_payloadSize > NRF24_MAX_MESSAGE_SIZE) {
    this->_payloadSize = NRF24_MAX_MESSAGE_SIZE;
  }

  if (this->_payloadSize == 0) {
    this->_payloadSize = 1;
  }

  if (this->isDynamicPayload()) {
    // Enable dynamic payload length in FEATURE register
    cmd = (1 << FEATURE_REG_EN_DPL | 1 << FEATURE_REG_EN_ACK_PAY); // Enable DPL and ACK payloads
    this->writeRegister(FEATURE_REG, &cmd, 1);
    _delay_ms(2);

    // Enable dynamic payload on pipes 0 and 1
    cmd = (1 << DYNPD_REG_DPL_P0) | (1 << DYNPD_REG_DPL_P1);
    this->writeRegister(DYNPD_REG, &cmd, 1);
    _delay_ms(2);
  } else {
    // set size
    cmd = this->_payloadSize;
    this->writeRegister(RX_PW_P0_REG, &cmd, 1);
    _delay_ms(2);
    cmd = this->_payloadSize;
    this->writeRegister(RX_PW_P1_REG, &cmd, 1);
    _delay_ms(2);
    
    // ===== Disable Dynamic Payload on Pipes =====
    cmd = 0;
    this->writeRegister(DYNPD_REG, &cmd, 1);
    _delay_ms(2);
    cmd = 0;
    this->writeRegister(FEATURE_REG, &cmd, 1);
    _delay_ms(2);
  }

  // ===== Initial CONFIG register setup =====
  // Disable interrupt, enable CRC mode, and power up the module in RX mode by default
  uint8_t config =
      (1 << CONFIG_REG_MASK_RX_DR) |  
      (1 << CONFIG_REG_MASK_TX_DS) |  
      (1 << CONFIG_REG_MASK_MAX_RT) | 
      (1 << CONFIG_REG_EN_CRC) |      
      (1 << CONFIG_REG_CRC0) |        
      (1 << CONFIG_REG_PWR_UP) |      
      (1 << CONFIG_REG_PRIM_RX);
      
  cmd = config;
  uint8_t status = this->writeRegister(CONFIG_REG, &cmd, 1);
  // For nRF24L01+ to go from power down mode to TX or RX mode it must first pass through stand-by mode.
  // There must be a delay of Tpd2stby (see Table 16.) after the nRF24L01+ leaves power down mode before
  // the CE is set high. - Tpd2stby can be up to 5ms per the 1.0 datasheet
  _delay_ms(5);

  // check status for IRQ flags that may indicate issues with SPI communication
  if (status & ((1 << STATUS_REG_RX_DR) | (1 << STATUS_REG_TX_DS) | (1 << STATUS_REG_MAX_RT))) {
     // Write 1 to STATUS bits to clear any pending interrupts
    cmd = (1 << STATUS_REG_RX_DR) |  // Clear RX_DR flag
          (1 << STATUS_REG_TX_DS) |  // Clear TX_DS flag
          (1 << STATUS_REG_MAX_RT);  // Clear MAX_RT flag
    this->writeRegister(STATUS_REG, &cmd, 1);
    _delay_ms(2);
  }

  // Read back CONFIG register to verify it was written correctly
  uint8_t config_register;
  this->readRegister(CONFIG_REG, &config_register, 1);

  // If mismatch detected, add delay and retry (handles unstable modules)
  if (config != config_register) {
    _delay_ms(500);
    cmd = config;
    this->writeRegister(CONFIG_REG, &cmd, 1);
    // NRF24 needs some time after power up to be stable
    _delay_ms(5);
  }
  this->flushRX(); // Clear RX FIFO in case it contains any stale data from power-up
  this->flushTX(); // Clear TX FIFO in case it contains any stale data from power-up
 
  this->cehigh(); // Ensure CE is HIGH to enable RF receiver (module is now actively listening for packets)

  #ifdef HAS_INT0
  sei(); // Re-enable interrupts
  #endif
}

/**
 * @brief Clear all pending RX packets from FIFO
 * 
 * Flushes the RX FIFO, discarding all received packets that haven't been
 * read yet. Useful for clearing stale data before switching modes.
 * 
 * @note Should be called before entering RX mode or after changeState()
 */
void NRF24Manager::flushRX() {
  this->send_spi(NRF24CMD_FLUSH_RX, 0, 0);
}

/**
 * @brief Clear all pending TX packets from FIFO
 * 
 * Flushes the TX FIFO, discarding all unsent packets. Useful for clearing
 * queued transmissions before changing modes or recovering from errors.
 */
void NRF24Manager::flushTX() {
  this->send_spi(NRF24CMD_FLUSH_TX, 0, 0);
}

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
void NRF24Manager::changeState(uint8_t state) {
  uint8_t config_register, data;
  this->readRegister(CONFIG_REG, &config_register, 1);

  switch (state) {
  case NRF24_RECEIVE:
    if (config_register & (1 << CONFIG_REG_PRIM_RX)) {
      return; // Already in receive mode
    }
    this->celow();  // Ensure CE is LOW before changing mode
    // Set PRIM_RX=1 for receive mode
    data = config_register | (1 << CONFIG_REG_PRIM_RX);
    this->writeRegister(CONFIG_REG, &data, 1);
    _delay_ms(1);
    // Clear all status flags before entering RX
    data = (1 << STATUS_REG_RX_DR) | (1 << STATUS_REG_TX_DS) |
           (1 << STATUS_REG_MAX_RT);
    this->writeRegister(STATUS_REG, &data, 1);
    this->cehigh();  // Enable RF receiver
    _delay_us(150);  // wait 130 us before using
    break;
  case NRF24_TRANSMIT:
    if (!(config_register & (1 << CONFIG_REG_PRIM_RX))) {
      return; // Already in transmit mode
    }
    this->celow();  // Ensure CE is LOW for transmit mode
    // Set PRIM_RX=0 for transmit mode
    data = config_register & ~(1 << CONFIG_REG_PRIM_RX);
    this->writeRegister(CONFIG_REG, &data, 1);
    
    _delay_ms(1);
    break;
  }
}

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
int8_t NRF24Manager::dataAvailable(void) {
  uint8_t fifo;
  uint8_t status = this->readRegister(FIFO_STATUS_REG, &fifo, 1);
  // RX_EMPTY bit (bit 0): 1=empty, 0=has data
  bool dataAvailable = ((fifo & (1 << FIFO_STATUS_REG_RX_EMPTY)) == 0); // Also check RX_DR flag in STATUS for pending data
  if (dataAvailable) {
    int8_t p = (status >> STATUS_REG_RX_P_NO_START_BIT) & 0x07; // Extract pipe number (bits 3-1)
    return p;
  }

  return -1;
}

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
uint8_t* NRF24Manager::read_binary_message(uint8_t& length) {
  // Message placeholder - STATIC (persists between calls, gets overwritten)
  static uint8_t rx_message[NRF24_MAX_MESSAGE_SIZE];
  this->celow(); // Ensure CE is LOW to read from FIFO (CE must be LOW for SPI access to RX FIFO)

  // Get length of incoming message using R_RX_PL_WID command
  // This reads the actual payload width from the next packet in FIFO
  uint8_t l = 0; // Default to 0 if read fails
  this->send_spi(NRF24CMD_R_RX_PL_WID, &l, 1);
  length = l; // Output the length to caller
  // Read message from RX FIFO
  if (length > 0) {
    this->send_spi(NRF24CMD_R_RX_PAYLOAD, rx_message, length);
  } else {
    // If length is 0, it may indicate a FIFO corruption issue. Flush RX FIFO to clear it.
    this->flushRX();
  }
  
  // Clear RX_DR interrupt flag by writing 1 to it in STATUS register
  // This signals to the module that we've acknowledged the data
  uint8_t data = (1 << STATUS_REG_RX_DR);
  this->writeRegister(STATUS_REG, &data, 1);
  _delay_ms(2);

  this->cehigh(); // Return to listening mode (CE HIGH)

  // Return data pointer if received, otherwise NULL
  if (length > 0) {
    return rx_message;
  }

  return NULL;
}

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
void NRF24Manager::set_ack_buffer(uint8_t *msg, uint8_t length) {
  if (length > NRF24_MAX_MESSAGE_SIZE) {
    length = NRF24_MAX_MESSAGE_SIZE; // Truncate if message exceeds maximum size
  }
  if (length == 0) {
    length = 1; // Minimum payload size is 1 byte
  }

  this->celow(); // Ensure CE is LOW to access SPI and set ACK payload
  // Load ACK payload into nRF24L01+ for pipe 1 (used for auto-ack)
  uint8_t cmd = NRF24CMD_W_ACK_PAYLOAD | 0x01; // Pipe 1
  this->_spi->begin(this->_cs_pin);
  this->_spi->sendCommand(cmd);
  
  // send data
  while (length--)
    this->_spi->send(*(uint8_t *)msg++);  // Send raw bytes
  this->_spi->end(this->_cs_pin);

  this->cehigh(); // Return to listening mode (CE HIGH)

}

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
uint8_t* NRF24Manager::send_binary(uint8_t *msg, uint8_t &length) {
  // Transmit mode: CE low, enter TX mode
  this->celow();  // Stop any RX activity
  this->changeState(NRF24_TRANSMIT);
  uint8_t* result = NULL;

  // clear status flags to ensure clean state before transmission
  uint8_t cmd = (1 << STATUS_REG_RX_DR) | (1 << STATUS_REG_TX_DS) |
                (1 << STATUS_REG_MAX_RT); // Clear all status flags
  this->writeRegister(STATUS_REG, &cmd, 1);

  if (!this->isDynamicPayload() && length > this->_payloadSize) {
    length = this->_payloadSize; // Truncate if message exceeds payload size
  }
  if (length > NRF24_MAX_MESSAGE_SIZE) {
    length = NRF24_MAX_MESSAGE_SIZE; // Truncate if message exceeds maximum size
  }

  // Load binary message to TX_PAYLOAD register
  this->_spi->begin(this->_cs_pin);
  this->_spi->send(NRF24CMD_W_TX_PAYLOAD);
  
  int8_t padding = (this->_payloadSize > 0) ? this->_payloadSize - length : 0;

  // send data
  while (length--)
    this->_spi->send(*(uint8_t *)msg++);  // Send raw bytes
  // send padding
  while (padding--)
    this->_spi->send(0);  // Send padding bytes
  
  this->_spi->end(this->_cs_pin);

  // Send message by pulsing CE high (≥10µs)
  this->cehigh();
  _delay_us(15);  // Pulse CE for ~15µs
  this->celow();

  // Wait for transmission to complete by monitoring STATUS register flags
  uint8_t status = 0;
  while(!(status & ((1 << STATUS_REG_TX_DS) | (1 << STATUS_REG_MAX_RT)))) {
    this->readRegister(STATUS_REG, &status, 1);
  }
  // clear bits after transmission completes
  cmd = (1 << STATUS_REG_TX_DS) | (1 << STATUS_REG_MAX_RT); // Clear all status flags
  this->writeRegister(STATUS_REG, &cmd, 1);
  _delay_us(10);
  this->flushTX(); // Clear TX FIFO to prepare for next transmission

  // switch back to receive mode after transmission
  this->changeState(NRF24_RECEIVE);

  // Check for transmission success or failure
  if (status & (1 << STATUS_REG_MAX_RT)) {
    // Transmission failed
    length = 33; // Indicate failure with special length value (greater than max payload)
    return NULL;
  }

  return result;
}

#ifdef HAS_SERIAL

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
void NRF24Manager::info() {

  uint8_t buffer[5];
  this->summary();

  this->readRegister(EN_AA_REG, buffer, 1);
  USART_WritePString(PSTR("        EN_AA: "));
  USART_WriteUInt(buffer[0], 2);
  USART_WritePString(PSTR("\n"));

  this->readRegister(EN_RXADDR_REG, buffer, 1);
  USART_WritePString(PSTR("    EN_RXADDR: "));
  USART_WriteUInt(buffer[0], 2);
  USART_WritePString(PSTR("\n"));

  this->readRegister(SETUP_AW_REG, buffer, 1);
  USART_WritePString(PSTR("     SETUP_AW: "));
  USART_WriteUInt(buffer[0], 2);
  USART_WritePString(PSTR("\n"));

  this->readRegister(SETUP_RETR_REG, buffer, 1);
  USART_WritePString(PSTR("   SETUP_RETR: "));
  USART_WriteUInt(buffer[0], 2);
  USART_WritePString(PSTR("\n"));

  this->readRegister(RF_CH_REG, buffer, 1);
  USART_WritePString(PSTR("        RF_CH: "));
  USART_WriteUInt(buffer[0], 2);
  USART_WritePString(PSTR("\n"));

  this->readRegister(RF_SETUP_REG, buffer, 1);
  USART_WritePString(PSTR("     RF_SETUP: "));
  USART_WriteUInt(buffer[0], 2);
  USART_WritePString(PSTR("\n"));

  this->readRegister(DYNPD_REG, buffer, 1);
  USART_WritePString(PSTR("        DYNPD: "));
  USART_WriteUInt(buffer[0], 2);
  USART_WritePString(PSTR("\n"));

  this->readRegister(FEATURE_REG, buffer, 1);
  USART_WritePString(PSTR("      FEATURE: "));
  USART_WriteUInt(buffer[0], 2);
  USART_WritePString(PSTR("\n"));

  this->readRegister(RX_ADDR_P0_REG, buffer, 5);
  USART_WritePString(PSTR("   RX_ADDR_P0: "));
  for (uint8_t i = 0; i < 5; i++) {
    USART_WriteChar(buffer[i]);
  }
  USART_WritePString(PSTR("\n"));

  this->readRegister(RX_PW_P0_REG, buffer, 1);
  USART_WritePString(PSTR("     RX_PW_P0: "));
  USART_WriteUInt(buffer[0]);
  USART_WritePString(PSTR("\n"));

  this->readRegister(RX_ADDR_P1_REG, buffer, 5);
  USART_WritePString(PSTR("   RX_ADDR_P1: "));
  for (uint8_t i = 0; i < 5; i++) {
    USART_WriteChar(buffer[i]);
  }
  USART_WritePString(PSTR("\n"));

  this->readRegister(RX_PW_P1_REG, buffer, 1);
  USART_WritePString(PSTR("     RX_PW_P1: "));
  USART_WriteUInt(buffer[0]);
  USART_WritePString(PSTR("\n"));

  this->readRegister(TX_ADDR_REG, buffer, 5);
  USART_WritePString(PSTR("      TX_ADDR: "));
  for (uint8_t i = 0; i < 5; i++) {
    USART_WriteChar(buffer[i]);
  }
  USART_WritePString(PSTR("\n"));
}

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
void NRF24Manager::summary() {
  uint8_t buffer[1];

  USART_WritePString(PSTR("NRF24 info:\n"));

  this->readRegister(CONFIG_REG, buffer, 1);
  USART_WritePString(PSTR("       CONFIG: "));
  USART_WriteUInt(buffer[0], 2);
  USART_WritePString(PSTR("\n"));

  this->readRegister(STATUS_REG, buffer, 1);
  USART_WritePString(PSTR("       STATUS: "));
  USART_WriteUInt(buffer[0], 2);
  USART_WritePString(PSTR("\n"));

  this->readRegister(OBSERVE_TX_REG, buffer, 1);
  USART_WritePString(PSTR("   OBSERVE_TX: "));
  USART_WriteUInt(buffer[0], 2);
  USART_WritePString(PSTR("\n"));

  this->readRegister(RPD_REG, buffer, 1);
  USART_WritePString(PSTR("          RPD: "));
  USART_WriteUInt(buffer[0], 2);
  USART_WritePString(PSTR("\n"));

  this->readRegister(FIFO_STATUS_REG, buffer, 1);
  USART_WritePString(PSTR("  FIFO_STATUS: "));
  USART_WriteUInt(buffer[0], 2);
  USART_WritePString(PSTR("\n"));
}

#elif defined(HAS_INT0_SERIAL)

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
void NRF24Manager::info() {

  uint8_t buffer[5];
  this->summary();

  this->readRegister(RX_ADDR_P0_REG, buffer, 5);
  INT0_WritePString(PSTR("   RX_ADDR_P0: "));
  for (uint8_t i = 0; i < 5; i++) {
    INT0_WriteChar(buffer[i]);
  }
  INT0_WritePString(PSTR("\n"));

  this->readRegister(RX_PW_P0_REG, buffer, 1);
  INT0_WritePString(PSTR("     RX_PW_P0: "));
  INT0_WriteUInt(buffer[0]);
  INT0_WritePString(PSTR("\n"));

  this->readRegister(RX_ADDR_P1_REG, buffer, 5);
  INT0_WritePString(PSTR("   RX_ADDR_P1: "));
  for (uint8_t i = 0; i < 5; i++) {
    INT0_WriteChar(buffer[i]);
  }
  INT0_WritePString(PSTR("\n"));
    this->readRegister(RX_PW_P1_REG, buffer, 1);
  INT0_WritePString(PSTR("     RX_PW_P1: "));
  INT0_WriteUInt(buffer[0]);
  INT0_WritePString(PSTR("\n"));

  this->readRegister(TX_ADDR_REG, buffer, 5);
  INT0_WritePString(PSTR("      TX_ADDR: "));
  for (uint8_t i = 0; i < 5; i++) {
    INT0_WriteChar(buffer[i]);
  }
  INT0_WritePString(PSTR("\n"));
  
  uint8_t l = 0; // Default to 0 if read fails
  this->send_spi(NRF24CMD_R_RX_PL_WID, &l, 1);
  INT0_WritePString(PSTR("     R_RX_PL_WID: "));
  INT0_WriteUInt(l);
  INT0_WritePString(PSTR("\n"));
}

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
void NRF24Manager::summary() {
  uint8_t buffer[1];

  INT0_WritePString(PSTR("NRF24 info:\n"));

  this->readRegister(CONFIG_REG, buffer, 1);
  INT0_WritePString(PSTR("       CONFIG: "));
  INT0_WriteUInt(buffer[0], 2);
  INT0_WritePString(PSTR("\n"));

  this->readRegister(STATUS_REG, buffer, 1);
  INT0_WritePString(PSTR("       STATUS: "));
  INT0_WriteUInt(buffer[0], 2);
  INT0_WritePString(PSTR("\n"));

  this->readRegister(OBSERVE_TX_REG, buffer, 1);
  INT0_WritePString(PSTR("   OBSERVE_TX: "));
  INT0_WriteUInt(buffer[0], 2);
  INT0_WritePString(PSTR("\n"));

  this->readRegister(RPD_REG, buffer, 1);
  INT0_WritePString(PSTR("          RPD: "));
  INT0_WriteUInt(buffer[0], 2);
  INT0_WritePString(PSTR("\n"));

  this->readRegister(FIFO_STATUS_REG, buffer, 1);
  INT0_WritePString(PSTR("  FIFO_STATUS: "));
  INT0_WriteUInt(buffer[0], 2);
  INT0_WritePString(PSTR("\n"));

  this->readRegister(DYNPD_REG, buffer, 1);
  INT0_WritePString(PSTR("        DYNPD: "));
  INT0_WriteUInt(buffer[0], 2);
  INT0_WritePString(PSTR("\n"));

  this->readRegister(FEATURE_REG, buffer, 1);
  INT0_WritePString(PSTR("      FEATURE: "));
  INT0_WriteUInt(buffer[0], 2);
  INT0_WritePString(PSTR("\n"));

  this->readRegister(EN_AA_REG, buffer, 1);
  INT0_WritePString(PSTR("        EN_AA: "));
  INT0_WriteUInt(buffer[0], 2);
  INT0_WritePString(PSTR("\n"));

  this->readRegister(EN_RXADDR_REG, buffer, 1);
  INT0_WritePString(PSTR("    EN_RXADDR: "));
  INT0_WriteUInt(buffer[0], 2);
  INT0_WritePString(PSTR("\n"));

  this->readRegister(SETUP_AW_REG, buffer, 1);
  INT0_WritePString(PSTR("     SETUP_AW: "));
  INT0_WriteUInt(buffer[0], 2);
  INT0_WritePString(PSTR("\n"));

  this->readRegister(SETUP_RETR_REG, buffer, 1);
  INT0_WritePString(PSTR("   SETUP_RETR: "));
  INT0_WriteUInt(buffer[0], 2);
  INT0_WritePString(PSTR("\n"));

  this->readRegister(RF_CH_REG, buffer, 1);
  INT0_WritePString(PSTR("        RF_CH: "));
  INT0_WriteUInt(buffer[0], 2);
  INT0_WritePString(PSTR("\n"));

  this->readRegister(RF_SETUP_REG, buffer, 1);
  INT0_WritePString(PSTR("     RF_SETUP: "));
  INT0_WriteUInt(buffer[0], 2);
  INT0_WritePString(PSTR("\n"));

}

#else
void NRF24Manager::info()  {}
void NRF24Manager::summary() {}
#endif