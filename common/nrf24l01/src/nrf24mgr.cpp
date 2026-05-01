
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

//-----------------------------------------------------------------------------------------
// Register

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

// SETUP_AW: seup of address widths
#define SETUP_AW_REG 0x03

// SETUP_RETR: setup of automatic retransmission
#define SETUP_RETR_REG 0x04

// RF Channel
#define RF_CH_REG 0x05

// RF_SETUP: RF setup register
#define RF_SETUP_REG 0x06
#define RF_SETUP_REG_RF_DR_LOW 5

// STATUS: status register
#define STATUS_REG 0x07
#define STATUS_REG_RX_DR 6
#define STATUS_REG_TX_DS 5
#define STATUS_REG_MAX_RT 4
#define STATUS_REG_RX_P_NO 1
#define STATUS_REG_TX_FULL 0

#define OBSERVE_TX_REG 0x08
#define RPD_REG 0x09

// pipe address
#define RX_ADDR_P0_REG 0x0A
#define RX_ADDR_P1_REG 0x0B
#define TX_ADDR_REG 0x10

// receive buffer
#define RX_PW_P0_REG 0x11
#define RX_PW_P1_REG 0x12

// FIFO_STATUS: FIFO status register
#define FIFO_STATUS_REG 0x17
#define FIFO_STATUS_REG_TX_REUSE 6
#define FIFO_STATUS_REG_FIFO_FULL 5
#define FIFO_STATUS_REG_TX_EMPTY 4
#define FIFO_STATUS_REG_RX_FULL 1
#define FIFO_STATUS_REG_RX_EMPTY 0

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
//-----------------------------------------------------------------------------------------
// SPI commands
#define NRF24CMD_ADDRESS_MASK 0x1F /* 000A AAAA */
#define NRF24CMD_R_REGISTER 0x00
#define NRF24CMD_W_REGISTER 0b00100000
#define NRF24CMD_R_RX_PAYLOAD 0b01100001
#define NRF24CMD_W_TX_PAYLOAD 0b10100000
#define NRF24CMD_FLUSH_TX 0b11100001
#define NRF24CMD_FLUSH_RX 0b11100010
#define NRF24CMD_REUSE_TX_PL 0b11100011
//#define NRF24CMD_ACTIVATE 0b01010000
#define NRF24CMD_R_RX_PL_WID 0b01100000
#define NRF24CMD_ACK_PAYLOAD_MASK 0b00000111
#define NRF24CMD_W_ACK_PAYLOAD 0b10101000 /* 1010 1PPP | PPP = pipe number */
#define NRF24CMD_W_TX_PAYLOAD_NOACK 0b10110000
#define NRF24CMD_NOP 0xFF

//-----------------------------------------------------------------------------------------
// RADIO address
uint8_t radio_address[5] = {0xe5, 0xe6, 0xe7, 0xe8, 0xe4};
uint8_t tx_radio_address[5] = {0xe5, 0xe6, 0xe7, 0xe8, 0xe4};

void NRF24Manager::celow() { GPIO_SET_LOW(NRF24_CX_PIN_PORT, this->_ce_pin); }

void NRF24Manager::cehigh() { GPIO_SET_HIGH(NRF24_CX_PIN_PORT, this->_ce_pin); }

//-------------------------------------------------------------------------------------
// R/W Register Functions
// These low-level functions handle SPI communication with the NRF24L01 module

/**
 * @brief Send SPI command to NRF24 module
 * 
 * @param cmd Command byte to send (includes opcode and register address)
 * @param data Pointer to data buffer for read/write operations
 * @param size Number of bytes to read/write (0 for command-only operations)
 * @return Status register value from NRF24 (returned during first SPI byte)
 * 
 */
uint8_t NRF24Manager::send_spi(uint8_t cmd, uint8_t *data, uint8_t size) {
  uint8_t exchange = cmd;

  this->_spi->begin(this->_cs_pin);
  // sendCommand() returns boolean (master/slave indicator)
  bool isMaster = this->_spi->sendCommand(exchange);
  if (!isMaster) {
    // If we're a slave, we won't get the status byte back during the command exchange
    return 0;
  }
  
  if (size > 0) {
    uint8_t *indata = (uint8_t *) malloc(size);
    this->_spi->sendCommandData(size, data);
    free(indata);
  }
  this->_spi->end(this->_cs_pin);

  return exchange; // nRF24L01 STATUS register value is returned during the command exchange (first byte)
}

/**
 * @brief Write to NRF24 register
 * 
 * @param reg Register address (will be ORed with W_REGISTER command 0x20)
 * @param data Pointer to data bytes to write
 * @param size Number of bytes to write
 * @return Status register value
 */
uint8_t NRF24Manager::writeRegister(uint8_t reg, uint8_t *data, uint8_t size) {
  uint8_t status = this->send_spi(NRF24CMD_W_REGISTER | reg, data, size);
  _delay_ms(10);  // Allow register write to settle
  return status;  // Return statement was after delay
}

/**
 * @brief Read from NRF24 register
 * 
 * @param reg Register address (will be ORed with R_REGISTER command 0x00)
 * @param data Pointer to buffer where read data will be stored
 * @param size Number of bytes to read
 * @return Status register value
 */
uint8_t NRF24Manager::readRegister(uint8_t reg, uint8_t *data, uint8_t size) {
  return this->send_spi(NRF24CMD_R_REGISTER | reg, data, size);
}

//-------------------------------------------------------------------------------------
// SPI initialization and NRF24 configuration
/**
 * @brief Initialize NRF24L01 module
 * 
 * Complete initialization sequence including:
 *  - GPIO setup for CE and CSN pins
 *  - Power-up sequence with proper delays
 *  - Register configuration (data rate, CRC, address format, etc.)
 *  - RX/TX pipe setup
 *  - Dynamic payload configuration
 * 
 * @param s Pointer to SPIManager instance for SPI communication
 * @param ce_pin GPIO pin number for CE (Chip Enable) control
 * @param cs_pin GPIO pin number for CS (Chip Select) control
 * 
 * IMPORTANT: NRF24L01+ requires 3.3V supply with bypassing capacitor.
 * On 5V Arduino platforms, logic level conversion is required!
 */
void NRF24Manager::init(SPIManager *s, uint8_t ce_pin, uint8_t cs_pin) {
  this->_ce_pin = ce_pin;
  this->_cs_pin = cs_pin;
  this->_spi = s;

#ifdef HAS_INT0
  // TODO: Rewrite for PB0 PCINT0 interrupt support
  cli(); // Disable interrupts for critical section
#if defined(__AVR_ATmega8535__)
  // ATmega8535 uses GICR and different ISC bits
  // MCUCR |= (1 << ISC01); // The falling edge of INT0 generates an interrupt
  // GICR |= (1 << INT0); // INT0 is on PIN 16 PD2
#else
  // ATmega328/168 (Arduino Uno) uses EICRA and EIMSK
  EICRA |= (1 << ISC01); // INT0 on falling edge generates interrupt request
  EIMSK |= (1 << INT0);  // Enable INT0 (located on PD2/PIN2)
#endif
  sei(); // Re-enable interrupts
#endif

  // Configure CE pin as output and set LOW (disable transmit/receive)
  GPIO_OUTPUT(NRF24_CX_PIN_PORT, this->_ce_pin);
  this->celow();

  // CRITICAL: NRF24 requires >100ms after power-on to be stable
  // Extended delay recommended for slower AVR designs
  _delay_ms(200);

  // ===== PHASE 1: Initial CONFIG register setup =====
  // Disable interrupt, enable CRC mode, and power up the module in RX mode by default
  uint8_t config =
      (1 << CONFIG_REG_MASK_RX_DR) |  
      (1 << CONFIG_REG_MASK_TX_DS) |  
      (1 << CONFIG_REG_MASK_MAX_RT) | 
      (1 << CONFIG_REG_EN_CRC) |      
      (1 << CONFIG_REG_CRC0) |        
      (1 << CONFIG_REG_PWR_UP) |      
      (1 << CONFIG_REG_PRIM_RX);
      
  uint8_t cmd = config;
  uint8_t status = this->writeRegister(CONFIG_REG, &cmd, 1);
  // check status register for expected value (0x00 or 0x01 depending if TX FIFO is full) to verify communication
  if ((status & 0xE0) != 0) {
    // If any of the RX_DR, TX_DS, or MAX_RT flags are set, initialization may have failed
    // This could indicate a communication issue with the NRF24 module
    // Consider adding error handling or retry logic here

    // wait and retry
    _delay_ms(200);
    cmd = config;
    status = this->writeRegister(CONFIG_REG, &cmd, 1);
  }
  
  // Allow settling time after CONFIG write
  _delay_ms(2);

  // ===== PHASE 2: Auto-Acknowledgment Setup =====
  // Enhanced ShockBurst: automatic ACK on received packets
  if (this->_autoack) {
    // Enable AutoACK only on pipe 0 (used for both RX and TX)
    cmd = (0 << EN_AA_REG_ENAA_P5) | (0 << EN_AA_REG_ENAA_P4) |
          (0 << EN_AA_REG_ENAA_P3) | (0 << EN_AA_REG_ENAA_P2) |
          (0 << EN_AA_REG_ENAA_P1) | (1 << EN_AA_REG_ENAA_P0);
  } else {
    // Disable AutoACK on all pipes
    cmd = (0 << EN_AA_REG_ENAA_P5) | (0 << EN_AA_REG_ENAA_P4) |
          (0 << EN_AA_REG_ENAA_P3) | (0 << EN_AA_REG_ENAA_P2) |
          (0 << EN_AA_REG_ENAA_P1) | (0 << EN_AA_REG_ENAA_P0);
  }
  this->writeRegister(EN_AA_REG, &cmd, 1);

  // ===== PHASE 3: Address Setup =====
  // NOTE: Address width set to 3 bytes, but addresses defined as 5 bytes!
  // This is likely a bug - either use 5-byte addresses or change this to 0x03
  cmd = 0x01; // Address width: 00=illegal, 01=3 bytes, 10=4 bytes, 11=5 bytes
  this->writeRegister(SETUP_AW_REG, &cmd, 1);

  // ===== PHASE 4: Automatic Retransmission Setup =====
  // ARD (Auto Retransmit Delay) and ARC (Auto Retransmit Count)
  if (this->_autoack) {
    cmd = 0xF1; // ARD=1111 (4000µs delay), ARC=0001 (1 retry attempt)
  } else {
    cmd = 0xF0; // ARD=1111 (4000µs delay), ARC=0000 (no retries)
  }
  this->writeRegister(SETUP_RETR_REG, &cmd, 1);

  // ===== PHASE 5: RF Channel Configuration =====
  // Channel frequency = 2400 + RF_CH (MHz)
  // Valid range: 0-125 (2400-2525 MHz) with 25 channels available in most regions
  cmd = 0x10; // Channel 7: 2407 MHz (middle of ISM band)
  this->writeRegister(RF_CH_REG, &cmd, 1);

  // ===== PHASE 6: RF Setup (Data Rate and TX Power) =====
  // Default configuration for 250 kbps with max TX power
  cmd = (1 << RF_SETUP_REG_RF_DR_LOW) | // RF_DR_LOW=1 → 250 kbps (low power mode)
        0x06; // TX_PWR=11 → 0 dBm (maximum TX power)
  this->writeRegister(RF_SETUP_REG, &cmd, 1);

  // ===== PHASE 7: Clear STATUS Flags =====
  // Write 1 to STATUS bits to clear any pending interrupts
  cmd = (1 << STATUS_REG_RX_DR) |  // Clear RX_DR flag
        (1 << STATUS_REG_TX_DS) |  // Clear TX_DS flag
        (1 << STATUS_REG_MAX_RT);  // Clear MAX_RT flag
  this->writeRegister(STATUS_REG, &cmd, 1);

  // ===== PHASE 8: Dynamic Payload Setup =====
  // Enable variable-length payload packets
  if (this->_autoack) {
    // Enable Dynamic Payload Length, ACK Payload, and Dynamic ACK
    cmd = (1 << FEATURE_REG_EN_DPL) | (1 << FEATURE_REG_EN_ACK_PAY) |
          (1 << FEATURE_REG_EN_DYN_ACK);
  } else {
    // Only enable Dynamic Payload Length
    cmd = (1 << FEATURE_REG_EN_DPL) | (0 << FEATURE_REG_EN_ACK_PAY) |
          (0 << FEATURE_REG_EN_DYN_ACK);
  }
  this->writeRegister(FEATURE_REG, &cmd, 1);

  // ===== PHASE 9: Enable Dynamic Payload on Pipes =====
  // Only pipe 0 enabled for RX (pipe pairs: P0/P1, P2/P3, P4/P5)
  // NOTE: This limits the driver to single-pipe operation
  cmd = (1 << DYNPD_REG_DPL_P0) | (0 << DYNPD_REG_DPL_P1) |
        (0 << DYNPD_REG_DPL_P2) | (0 << DYNPD_REG_DPL_P3) |
        (0 << DYNPD_REG_DPL_P4) | (0 << DYNPD_REG_DPL_P5);
  this->writeRegister(DYNPD_REG, &cmd, 1);

  // ===== PHASE 10: Set RX/TX Addresses =====
  // P0 is used for both RX (when listening) and TX acks
  // TX_ADDR must match RX_ADDR_P0 for auto-ack to work correctly
  this->writeRegister(RX_ADDR_P0_REG, radio_address, 5);
  this->writeRegister(TX_ADDR_REG, tx_radio_address, 5);

  // ===== PHASE 11: Enable RX Pipes =====
  // Only enable pipe 0 (bits correspond to pipes P0-P5)
  cmd = 0x01; // Binary: 000001 → Enable P0 only
  this->writeRegister(EN_RXADDR_REG, &cmd, 1);

  // ===== PHASE 12: Flush FIFOs and Clear Interrupts =====
  // Remove any stale data and ensure clean state
  uint8_t flush_cmd = NRF24CMD_FLUSH_RX;
  this->writeRegister(flush_cmd, 0, 0);
  flush_cmd = NRF24CMD_FLUSH_TX;
  this->writeRegister(flush_cmd, 0, 0);

  // ===== PHASE 13: Stability Re-check and CONFIG Verification =====
  // Some nRF24 modules are unstable during initial startup; verify and retry if needed
  cmd = config;
  this->writeRegister(CONFIG_REG, &cmd, 1);

  // Allow settling time after power-up
  _delay_ms(2);

  // Read back CONFIG register to verify it was written correctly
  uint8_t config_register;
  this->readRegister(CONFIG_REG, &config_register, 1);

  // If mismatch detected, add delay and retry (handles unstable modules)
  if (config != config_register) {
    _delay_ms(500);
    this->writeRegister(CONFIG_REG, &config, 1);
  }
                                           // 0 for transmit)
  // NRF24 needs some time after power up to be stable
  _delay_ms(2);
}

//-------------------------------------------------------------------------------------
// Change NRF24 Operating State
/**
 * @brief Change the operational state of the NRF24 module
 * 
 * Manages transitions between power and operating modes.
 * 
 * States:
 *   - NRF24_POWERUP: Power on the module (1.5ms startup time)
 *   - NRF24_POWERDOWN: Power off to reduce consumption
 *   - NRF24_RECEIVE: Enable RX mode (PRIM_RX=1, CE will be set to high by listen())
 *   - NRF24_TRANSMIT: Enable TX mode (PRIM_RX=0)
 *   - NRF24_STANDBY1: CE low (no transmit/receive active)
 *   - NRF24_STANDBY2: TX standby (CE high, PRIM_RX=0)
 * 
 * @param state Target state (see NRF24Manager.h for state defines)
 * 
 * NOTE: Always allow status register to settle after state changes
 */
void NRF24Manager::changeState(uint8_t state) {
  if (this->_state == state) {
    return;  // Already in target state
  }

  uint8_t config_register, data;
  this->readRegister(CONFIG_REG, &config_register, 1);

  switch (state) {
  case NRF24_POWERUP:
    // Check if already powered up
    if (!(config_register & (1 << CONFIG_REG_PWR_UP))) {
      data = config_register | (1 << CONFIG_REG_PWR_UP);
      this->writeRegister(CONFIG_REG, &data, 1);
      // CRITICAL: NRF24 needs 1.5ms minimum from POWERDOWN to operational
      _delay_ms(2);
    }
    break;
  case NRF24_POWERDOWN:
    // Clear PWR_UP bit to reduce current consumption to ~22µA
    data = config_register & ~(1 << CONFIG_REG_PWR_UP);
    this->writeRegister(CONFIG_REG, &data, 1);
    break;
  case NRF24_RECEIVE:
    // Set PRIM_RX=1 for receive mode
    data = config_register | (1 << CONFIG_REG_PRIM_RX);
    this->writeRegister(CONFIG_REG, &data, 1);
    // Clear all status flags before entering RX
    data = (1 << STATUS_REG_RX_DR) | (1 << STATUS_REG_TX_DS) |
           (1 << STATUS_REG_MAX_RT);
    this->writeRegister(STATUS_REG, &data, 1);
    _delay_ms(1);  // Allow mode transition to complete
    break;
  case NRF24_TRANSMIT:
    // Set PRIM_RX=0 for transmit mode
    data = config_register & ~(1 << CONFIG_REG_PRIM_RX);
    this->writeRegister(CONFIG_REG, &data, 1);
    // Clear all status flags before entering TX
    data = (1 << STATUS_REG_RX_DR) | (1 << STATUS_REG_TX_DS) |
           (1 << STATUS_REG_MAX_RT);
    this->writeRegister(STATUS_REG, &data, 1);
    _delay_ms(1);  // Allow mode transition to complete
    break;
  case NRF24_STANDBY1:
    // Pull CE low to stop transmit/receive
    this->celow();
    break;
  case NRF24_STANDBY2:
    // TX standby: CE high with PRIM_RX=0
    data = config_register & ~(1 << CONFIG_REG_PRIM_RX);
    this->writeRegister(CONFIG_REG, &data, 1);
    this->cehigh();
    _delay_us(150);  // Setup time for CE transition
    break;
  }

  this->_state = state;
}

//-------------------------------------------------------------------------------------
// Receive Mode Operations

/**
 * @brief Activate receive mode and prepare to listen for incoming data
 * 
 * Configures the module to receive mode and enables the radio receiver.
 * This should be called whenever you want to start listening for packets.
 * 
 * TIMING: CE must stay high for ≥130µs to start receiving
 */
void NRF24Manager::listen(void) {
  this->changeState(NRF24_RECEIVE); // Set PRIM_RX=1, clear status flags
  // Note: If AUTO_ACK enabled, could write ACK payload here: nrf24_write_ack()
  this->cehigh();                    // Enable receiver (must stay high to receive)
  _delay_us(150);                    // Setup time (≥130µs min)
}

/**
 * @brief Check if data is available to receive
 * 
 * @return 1 if data in RX FIFO, 0 if empty
 * 
 * Reads FIFO_STATUS register and checks RX_EMPTY bit
 * Note: This check is non-blocking
 */
uint8_t NRF24Manager::dataAvailable(void) {
  uint8_t fifo;
  this->readRegister(FIFO_STATUS_REG, &fifo, 1);
  // RX_EMPTY bit (bit 0): 1=empty, 0=has data
  if (!(fifo & (1 << FIFO_STATUS_REG_RX_EMPTY))) {
    return 1;
  }
  return 0;
}

/**
 * @brief Send automatic acknowledgment payload
 * 
 * Loads an ACK with data into the TX FIFO for pipe 0
 * Used in AutoACK mode with dynamic payload
 * 
 * WARNING: This function is incomplete - hardcodes "A" as payload
 */
void NRF24Manager::ack() {
  const char *ack = "A";
  unsigned int length = 1;
  this->_spi->begin(this->_cs_pin);
  this->_spi->send(NRF24CMD_W_ACK_PAYLOAD);
  while (length--)
    this->_spi->send(*(uint8_t *)ack++);
  this->_spi->end(this->_cs_pin);
}

/**
 * @brief Read text message from RX FIFO (NUL-terminated string)
 * 
 * @return Pointer to received message (static buffer), or NULL if empty
 * 
 * IMPORTANT ISSUE: Uses static buffer that gets overwritten on next call!
 * This means:
 *   1. Data is lost after next read_message() or read_binary_message() call
 *   2. Thread-unsafe (if used in interrupt context)
 *   3. Application must copy message immediately if needed
 * 
 */
const char *NRF24Manager::read_message() {
  // Message placeholder - STATIC (persists between calls, gets overwritten)
  static char rx_message[NRF24_MAX_MESSAGE_SIZE];
  memset(rx_message, 0, NRF24_MAX_MESSAGE_SIZE);

  // BUG: Write ACK message call is commented out
  // Uncomment if using AutoACK with payloads: this->ack();

  // Get length of incoming message using R_RX_PL_WID command
  uint8_t data = 0;
  this->readRegister(NRF24CMD_R_RX_PL_WID, &data, 1);

  // Read message from RX FIFO
  if (data > 0) {
    // BUZ: data+1 might be larger than NRF24_MAX_MESSAGE_SIZE!
    this->send_spi(NRF24CMD_R_RX_PAYLOAD, (uint8_t *)&rx_message, data + 1);
  }
  
  // Clear RX_DR interrupt flag by writing 1 to it
  data = (1 << STATUS_REG_RX_DR);
  this->writeRegister(STATUS_REG, &data, 1);

  // Return message if non-empty, otherwise NULL
  if (strlen(rx_message) > 0) {
    return rx_message;
  }

  return NULL;
}

/**
 * @brief Read binary message from RX FIFO (variable length)
 * 
 * @param length OUT parameter: receives the size of data read
 * @return Pointer to received data (static buffer), or NULL if empty
 * 
 * IMPORTANT ISSUE: Same as read_message() - uses static buffer!
 * Data is lost after next call.
 */
uint8_t* NRF24Manager::read_binary_message(uint8_t& length) {
  // Message placeholder - STATIC (persists between calls, gets overwritten)
  static uint8_t rx_message[NRF24_MAX_MESSAGE_SIZE];

  // Get length of incoming message
  this->readRegister(NRF24CMD_R_RX_PL_WID, &length, 1);

  // Read message from RX FIFO
  if (length > 0) {
    this->send_spi(NRF24CMD_R_RX_PAYLOAD, rx_message, length);
  }
  
  // Clear RX_DR interrupt flag
  uint8_t data = (1 << STATUS_REG_RX_DR);
  this->writeRegister(STATUS_REG, &data, 1);

  // Return data pointer if received, otherwise NULL
  if (length > 0) {
    return rx_message;
  }

  return NULL;
}

//-------------------------------------------------------------------------------------
// Transmit Operations

/**
 * @brief Send text message (string)
 * 
 * @param msg Pointer to NUL-terminated string to send
 * @return 1 on success, 0 on failure
 * 
 * Transmit process:
 *  1. Stop RX mode (CE low)
 *  2. Switch to TX mode (PRIM_RX=0)
 *  3. Flush FIFOs to ensure clean state
 *  4. Load message to TX payload register
 *  5. Pulse CE high for ≥10µs to start transmission
 *  6. If no AutoACK: wait for TX_DS flag (transmission done)
 * 
 * TIMING: TX completes in ~1ms at 250kbps or less
 * NOTE: With AutoACK, function returns immediately after CE pulse
 */
uint8_t NRF24Manager::send(const char *msg) {
  // Message length (strlen for text, but doesn't include NUL terminator)
  uint8_t length = strlen(msg);

  // Transmit mode: CE low, enter TX mode
  this->celow();  // Stop any RX activity
  this->changeState(NRF24_TRANSMIT);

  // Flush TX/RX to remove stale data and clear any pending interrupts
  this->writeRegister(NRF24CMD_FLUSH_RX, 0, 0);
  this->writeRegister(NRF24CMD_FLUSH_TX, 0, 0);

  // Commented: Could mask RX interrupt during TX if needed
  // this->readRegister(CONFIG_REG, &data, 1);
  // data |= (1 << CONFIG_REG_MASK_RX_DR);
  // this->writeRegister(CONFIG_REG, &data, 1);

  // Load message into TX_PAYLOAD register (SPI burst mode)
  this->_spi->begin(this->_cs_pin);
  this->_spi->send(NRF24CMD_W_TX_PAYLOAD);
  while (length--)
    this->_spi->send(*(uint8_t *)msg++);  // Send each character
  this->_spi->send(0);  // Send NUL terminator
  this->_spi->end(this->_cs_pin);

  // Send message by pulsing CE high (≥10µs minimum)
  this->cehigh();
  _delay_us(15);  // Pulse duration (can go up to 4ms)
  this->celow();

  // Wait for transmission to complete (only if AutoACK disabled)
  if (!this->_autoack) {
    uint8_t data = 0;
    // Poll STATUS register until TX_DS (TX Data Sent) flag is set
    this->readRegister(STATUS_REG, &data, 1);
    while (!(data & (1 << STATUS_REG_TX_DS))) {
      this->readRegister(STATUS_REG, &data, 1);
    }
    // Caller should clear interrupts and return to RX mode
  }

  // Commented: Could re-enable RX interrupt after TX
  // this->readRegister(CONFIG_REG, &data, 1);
  // data &= ~(1 << CONFIG_REG_MASK_RX_DR);
  // this->writeRegister(CONFIG_REG, &data, 1);

  // NOTE: Function should ideally return to listen() mode if in RX/TX mode
  // nrf24_start_listening();

  return 1;
}

/**
 * @brief Send binary data (arbitrary bytes)
 * 
 * @param msg Pointer to binary data buffer
 * @param length Number of bytes to send
 * @return 1 on success
 * 
 * Similar to send() but accepts arbitrary binary data
 * (not requiring NUL termination)
 * 
 * Max payload: 32 bytes per NRF24L01+ spec
 */
uint8_t NRF24Manager::send_binary(uint8_t *msg, uint8_t length) {
  // Transmit mode: CE low, enter TX mode
  this->celow();  // Stop any RX activity
  this->changeState(NRF24_TRANSMIT);

  // Flush TX/RX to ensure clean state
  this->writeRegister(NRF24CMD_FLUSH_RX, 0, 0);
  this->writeRegister(NRF24CMD_FLUSH_TX, 0, 0);

  // Load binary message to TX_PAYLOAD register (SPI burst)
  this->_spi->begin(this->_cs_pin);
  this->_spi->send(NRF24CMD_W_TX_PAYLOAD);
  while (length--)
    this->_spi->send(*(uint8_t *)msg++);  // Send raw bytes
  this->_spi->end(this->_cs_pin);

  // Send message by pulsing CE high (≥10µs)
  this->cehigh();
  _delay_us(15);  // Pulse CE for ~15µs
  this->celow();

  // Wait for transmission if AutoACK disabled
  if (!this->_autoack) {
    uint8_t data = 0;
    // Poll STATUS register for TX_DS (transmission complete)
    this->readRegister(STATUS_REG, &data, 1);
    while (!(data & (1 << STATUS_REG_TX_DS)))
      this->readRegister(STATUS_REG, &data, 1);
  }

  return 1;
}

/**
 * @brief Reset and reconfigure the NRF24 module
 * 
 * @param autoack Enable (1) or disable (0) auto-acknowledgment
 * 
 * Reconfigures key parameters:
 *  - AutoACK setting (affects address width, retry count, etc.)
 *  - CLears FIFO buffers
 *  - Re-configures feature registers
 *  - Does NOT reconfigure power, channel, or address settings
 * 
 * Use this to quickly switch between AutoACK modes or recover from errors
 */
void NRF24Manager::reset(uint8_t autoack) {
  this->_autoack = autoack;

  // Flush TX/RX FIFOs to clear any pending data
  this->writeRegister(NRF24CMD_FLUSH_RX, 0, 0);
  this->writeRegister(NRF24CMD_FLUSH_TX, 0, 0);

  // Reconfigure CONFIG register
  uint8_t config =
      (1 << CONFIG_REG_MASK_RX_DR) |  // Interrupt on RX data received
      (1 << CONFIG_REG_MASK_TX_DS) |  // No interrupt on TX done
      (1 << CONFIG_REG_MASK_MAX_RT) | // Interrupt on max retries exceeded
      (1 << CONFIG_REG_EN_CRC) |      // Enable CRC
      (1 << CONFIG_REG_CRC0) |        // 2-byte CRC
      (1 << CONFIG_REG_PWR_UP) |      // Power up
      (1 << CONFIG_REG_PRIM_RX);      // Start in RX mode

  uint8_t cmd = config;
  this->writeRegister(CONFIG_REG, &cmd, 1);

  // Allow settling time after CONFIG change
  _delay_ms(2);

  // Reconfigure Enhanced ShockBurst (AutoACK)
  if (this->_autoack) {
    cmd = (0 << EN_AA_REG_ENAA_P5) | (0 << EN_AA_REG_ENAA_P4) |
          (0 << EN_AA_REG_ENAA_P3) | (0 << EN_AA_REG_ENAA_P2) |
          (0 << EN_AA_REG_ENAA_P1) | (1 << EN_AA_REG_ENAA_P0);  // AutoACK only on P0
  } else {
    cmd = 0x00;  // All AutoACK disabled
  }
  this->writeRegister(EN_AA_REG, &cmd, 1);

  // Reconfigure automatic retransmission
  if (this->_autoack) {
    cmd = 0xF1; // ARD=1111 (4000µs), ARC=0001 (1 retry)
  } else {
    cmd = 0xF0; // ARD=1111 (4000µs), ARC=0000 (no retries)
  }
  this->writeRegister(SETUP_RETR_REG, &cmd, 1);

  // Reconfigure feature register (Dynamic Payload Length, etc.)
  if (this->_autoack) {
    cmd = (1 << FEATURE_REG_EN_DPL) |   // Dynamic Payload Length
          (1 << FEATURE_REG_EN_ACK_PAY) | // ACK Payload
          (1 << FEATURE_REG_EN_DYN_ACK);  // Dynamic ACK
  } else {
    cmd = (1 << FEATURE_REG_EN_DPL);    // Only DPL enabled
  }
  this->writeRegister(FEATURE_REG, &cmd, 1);
}


//-------------------------------------------------------------------------------------
// Print register information for debug
#ifdef HAS_SERIAL
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
    USART_WriteUInt(buffer[i], 16);
    USART_WritePString(PSTR(","));
  }
  USART_WritePString(PSTR("\n"));

  this->readRegister(RX_PW_P0_REG, buffer, 1);
  USART_WritePString(PSTR("     RX_PW_P0: "));
  USART_WriteUInt(buffer[0]);
  USART_WritePString(PSTR("\n"));

  this->readRegister(RX_ADDR_P1_REG, buffer, 5);
  USART_WritePString(PSTR("   RX_ADDR_P1: "));
  for (uint8_t i = 0; i < 5; i++) {
    USART_WriteUInt(buffer[i], 16);
    USART_WritePString(PSTR(","));
  }
  USART_WritePString(PSTR("\n"));

  this->readRegister(RX_PW_P1_REG, buffer, 1);
  USART_WritePString(PSTR("     RX_PW_P1: "));
  USART_WriteUInt(buffer[0]);
  USART_WritePString(PSTR("\n"));

  this->readRegister(TX_ADDR_REG, buffer, 5);
  USART_WritePString(PSTR("      TX_ADDR: "));
  for (uint8_t i = 0; i < 5; i++) {
    USART_WriteUInt(buffer[i], 16);
    USART_WritePString(PSTR(","));
  }
  USART_WritePString(PSTR("\n"));
}

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

void NRF24Manager::info() {

  uint8_t buffer[5];
  this->summary();

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

  this->readRegister(DYNPD_REG, buffer, 1);
  INT0_WritePString(PSTR("        DYNPD: "));
  INT0_WriteUInt(buffer[0], 2);
  INT0_WritePString(PSTR("\n"));

  this->readRegister(FEATURE_REG, buffer, 1);
  INT0_WritePString(PSTR("      FEATURE: "));
  INT0_WriteUInt(buffer[0], 2);
  INT0_WritePString(PSTR("\n"));

  this->readRegister(RX_ADDR_P0_REG, buffer, 5);
  INT0_WritePString(PSTR("   RX_ADDR_P0: "));
  for (uint8_t i = 0; i < 5; i++) {
    INT0_WriteUInt(buffer[i], 16);
    INT0_WritePString(PSTR(","));
  }
  INT0_WritePString(PSTR("\n"));

  this->readRegister(RX_PW_P0_REG, buffer, 1);
  INT0_WritePString(PSTR("     RX_PW_P0: "));
  INT0_WriteUInt(buffer[0]);
  INT0_WritePString(PSTR("\n"));

  this->readRegister(RX_ADDR_P1_REG, buffer, 5);
  INT0_WritePString(PSTR("   RX_ADDR_P1: "));
  for (uint8_t i = 0; i < 5; i++) {
    INT0_WriteUInt(buffer[i], 16);
    INT0_WritePString(PSTR(","));
  }
  INT0_WritePString(PSTR("\n"));

  this->readRegister(RX_PW_P1_REG, buffer, 1);
  INT0_WritePString(PSTR("     RX_PW_P1: "));
  INT0_WriteUInt(buffer[0]);
  INT0_WritePString(PSTR("\n"));

  this->readRegister(TX_ADDR_REG, buffer, 5);
  INT0_WritePString(PSTR("      TX_ADDR: "));
  for (uint8_t i = 0; i < 5; i++) {
    INT0_WriteUInt(buffer[i], 16);
    INT0_WritePString(PSTR(","));
  }
  INT0_WritePString(PSTR("\n"));
}

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
}

#else
void NRF24Manager::info()  {}
void NRF24Manager::summary() {}
#endif