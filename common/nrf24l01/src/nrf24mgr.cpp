
// AVR
#include <avr/interrupt.h>
#include <avr/io.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <util/delay.h>
#include <avr/pgmspace.h>

// external lib
#include <SPIManager.h>
#include <gpio.h>

#define HAS_SERIAL

#ifdef HAS_SERIAL
#include <usart_serial.h>
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
#define NRF24CMD_ACTIVATE 0b01010000
#define NRF24CMD_R_RX_PL_WID 0b01100000
#define NRF24CMD_ACK_PAYLOAD_MASK 0b00000111
#define NRF24CMD_W_ACK_PAYLOAD 0b10101000 /* 1010 1PPP | PPP = pipe number */
#define NRF24CMD_W_TX_PAYLOAD_NOACK 0b10110000
#define NRF24CMD_NOP 0xFF

//-----------------------------------------------------------------------------------------
// RADIO address
uint8_t radio_address[5] = {0xe5, 0xe6, 0xe7, 0xe8, 0xe9};
uint8_t tx_radio_address[5] = {0xe5, 0xe6, 0xe7, 0xe8, 0xe9};

void NRF24Manager::celow() { GPIO_SET_LOW(B, this->_ce_pin); }

void NRF24Manager::cehigh() { GPIO_SET_HIGH(B, this->_ce_pin); }

//-------------------------------------------------------------------------------------
// R/W Register
uint8_t NRF24Manager::send_spi(uint8_t cmd, uint8_t *data, uint8_t size) {
  uint8_t status;

  this->_spi->begin();
  this->_spi->sendCommand(cmd);
  status = cmd; // sendCommand is replacing sent data with received
  if (size > 0) {
    this->_spi->sendCommandData(size, data);
  }
  this->_spi->end();

  return status;
}

uint8_t NRF24Manager::writeRegister(uint8_t reg, uint8_t *data, uint8_t size) {
  return this->send_spi(NRF24CMD_W_REGISTER | reg, data, size);
}

uint8_t NRF24Manager::readRegister(uint8_t reg, uint8_t *data, uint8_t size) {
  return this->send_spi(NRF24CMD_R_REGISTER | reg, data, size);
}

//-------------------------------------------------------------------------------------
// SPI init
void NRF24Manager::init(SPIManager *s, uint8_t ce_pin) {
  this->_ce_pin = ce_pin;
  this->_spi = s;

#ifdef HAS_INT0
  // TODO: rewrite for PB0 PCINT0;
  cli(); // Disable interrupts
#if defined(__AVR_ATmega8535__)
// MCUCR |= (1 << ISC01); // The falling edge of INT0 generates an interrupt
// request. GICR |= (1 << INT0); // INT0 is on PIN 16 PD2
#else
  EICRA |=
      (1 << ISC01); // The falling edge of INT0 generates an interrupt request.
  EIMSK |= (1 << INT0); // INT0 is on PIN 2 PD2
#endif
  sei(); // Enable interrupts
#endif

  // CE as outputs and initial states
  GPIO_OUTPUT(B, this->_ce_pin);
  this->celow();
  ;

  // NRF24 needs more than 100ms after power on to be stable
  // especially on slow AVR
  _delay_ms(200);

  // set config register
  uint8_t cmd = (1 << CONFIG_REG_MASK_RX_DR) |  // interrupt on RX
                (1 << CONFIG_REG_MASK_TX_DS) |  // no interrupt on TX
                (1 << CONFIG_REG_MASK_MAX_RT) | // interrupt on auto retransmit
                                                // counter overflow
                (1 << CONFIG_REG_EN_CRC) |      // CRC enable
                (1 << CONFIG_REG_CRC0) |        // CRC scheme
                (1 << CONFIG_REG_PWR_UP) |      // Power up
                (1 << CONFIG_REG_PRIM_RX); // RX mode by default (set PRIM_RX to
                                           // 0 for transmit)
  this->writeRegister(CONFIG_REG, &cmd, 1);

  // NRF24 needs some time after power up to be stable
  _delay_ms(2);

  // Enhanced ShockBurst Auto-acknowledge
  if (this->_autoack) {
    cmd = (0 << EN_AA_REG_ENAA_P5) | (0 << EN_AA_REG_ENAA_P4) |
          (0 << EN_AA_REG_ENAA_P3) | (0 << EN_AA_REG_ENAA_P2) |
          (0 << EN_AA_REG_ENAA_P1) | (1 << EN_AA_REG_ENAA_P0);
  } else {
    cmd = (0 << EN_AA_REG_ENAA_P5) | (0 << EN_AA_REG_ENAA_P4) |
          (0 << EN_AA_REG_ENAA_P3) | (0 << EN_AA_REG_ENAA_P2) |
          (0 << EN_AA_REG_ENAA_P1) | (0 << EN_AA_REG_ENAA_P0);
  }
  this->writeRegister(EN_AA_REG, &cmd, 1);

  // set address width to 3 bytes
  cmd = 0x01;
  this->writeRegister(SETUP_AW_REG, &cmd, 1);

  // Set retries
  if (this->_autoack) {
    cmd = 0xF1; // use max delay of 4000us (F) with 1 retry (1)
  } else {
    cmd = 0xF0; // use max delay of 4000us (F) with 1 retry (1)
  }
  this->writeRegister(SETUP_RETR_REG, &cmd, 1);

  // Sets the frequency channel
  cmd = 0x0F;
  this->writeRegister(RF_CH_REG, &cmd, 1);

  // Setup
  cmd = (1 << RF_SETUP_REG_RF_DR_LOW) | // Data rate low 250kbps
        0x06; // Set RF output power in TX mode to max 0dBm
  this->writeRegister(RF_SETUP_REG, &cmd, 1);

  // Status - clear TX/RX FIFO's and MAX_RT by writing 1 into them
  cmd = (1 << STATUS_REG_RX_DR) | // RX FIFO
        (1 << STATUS_REG_TX_DS) | // TX FIFO
        (1 << STATUS_REG_MAX_RT); // MAX RT
  this->writeRegister(STATUS_REG, &cmd, 1);

  // Dynamic payload on all pipes
  cmd = (1 << DYNPD_REG_DPL_P0) | (0 << DYNPD_REG_DPL_P1) |
        (0 << DYNPD_REG_DPL_P2) | (0 << DYNPD_REG_DPL_P3) |
        (0 << DYNPD_REG_DPL_P4) | (0 << DYNPD_REG_DPL_P5);
  this->writeRegister(DYNPD_REG, &cmd, 1);

  // Enable dynamic payload and autoack if set
  if (this->_autoack) {
    cmd = (1 << FEATURE_REG_EN_DPL) | (1 << FEATURE_REG_EN_ACK_PAY) |
          (1 << FEATURE_REG_EN_DYN_ACK);
  } else {
    cmd = (1 << FEATURE_REG_EN_DPL) | (0 << FEATURE_REG_EN_ACK_PAY) |
          (0 << FEATURE_REG_EN_DYN_ACK);
  }
  this->writeRegister(FEATURE_REG, &cmd, 1);

  // Open pipe
  this->writeRegister(RX_ADDR_P0_REG, radio_address, 5);
  this->writeRegister(TX_ADDR_REG, tx_radio_address, 5);
  // Enable pipes
  cmd = 0x01; // use pipe 0
  this->writeRegister(EN_RXADDR_REG, &cmd, 1);

  // Flush TX/RX and clear TX interrupt
  this->writeRegister(NRF24CMD_FLUSH_RX, 0, 0);
  this->writeRegister(NRF24CMD_FLUSH_TX, 0, 0);

  // set config register a second time
  // startup is not always stable
  uint8_t config =
      (1 << CONFIG_REG_MASK_RX_DR) |  // interrupt on RX
      (1 << CONFIG_REG_MASK_TX_DS) |  // no interrupt on TX
      (1 << CONFIG_REG_MASK_MAX_RT) | // interrupt on auto retransmit counter
                                      // overflow
      (1 << CONFIG_REG_EN_CRC) |      // CRC enable
      (1 << CONFIG_REG_CRC0) |        // CRC scheme
      (1 << CONFIG_REG_PWR_UP) |      // Power up
      (1 << CONFIG_REG_PRIM_RX); // RX mode by default (set PRIM_RX to 0 for
                                 // transmit)

  cmd = config;
  this->writeRegister(CONFIG_REG, &cmd, 1);

  // NRF24 needs some time after power up to be stable
  _delay_ms(2);

  // check config reg and retry
  uint8_t config_register;
  this->readRegister(CONFIG_REG, &config_register, 1);

  if (config != config_register) {
    _delay_ms(500);
    this->writeRegister(CONFIG_REG, &config, 1);
  }
}

//-------------------------------------------------------------------------------------
// change NRF24 state
void NRF24Manager::changeState(uint8_t state) {
  if (this->_state == state) {
    return;
  }

  uint8_t config_register, data;
  this->readRegister(CONFIG_REG, &config_register, 1);

  switch (state) {
  case NRF24_POWERUP:
    // Check if already powered up
    if (!(config_register & (1 << CONFIG_REG_PWR_UP))) {
      data = config_register | (1 << CONFIG_REG_PWR_UP);
      this->writeRegister(CONFIG_REG, &data, 1);
      // 1.5ms from POWERDOWN to start up
      _delay_ms(2);
    }
    break;
  case NRF24_POWERDOWN:
    data = config_register & ~(1 << CONFIG_REG_PWR_UP);
    this->writeRegister(CONFIG_REG, &data, 1);
    break;
  case NRF24_RECEIVE:
    data = config_register | (1 << CONFIG_REG_PRIM_RX);
    this->writeRegister(CONFIG_REG, &data, 1);
    // Clear STATUS register
    data = (1 << STATUS_REG_RX_DR) | (1 << STATUS_REG_TX_DS) |
           (1 << STATUS_REG_MAX_RT);
    this->writeRegister(STATUS_REG, &data, 1);
    _delay_ms(1);
    break;
  case NRF24_TRANSMIT:
    data = config_register & ~(1 << CONFIG_REG_PRIM_RX);
    this->writeRegister(CONFIG_REG, &data, 1);
    // Clear STATUS register
    data = (1 << STATUS_REG_RX_DR) | (1 << STATUS_REG_TX_DS) |
           (1 << STATUS_REG_MAX_RT);
    this->writeRegister(STATUS_REG, &data, 1);
    _delay_ms(1);
    break;
  case NRF24_STANDBY1:
    this->celow();
    break;
  case NRF24_STANDBY2:
    data = config_register & ~(1 << CONFIG_REG_PRIM_RX);
    this->writeRegister(CONFIG_REG, &data, 1);
    this->cehigh();
    _delay_us(150);
    break;
  }

  this->_state = state;
}

//-------------------------------------------------------------------------------------
// Change to active receiving state, TODO: put in change state
void NRF24Manager::listen(void) {
  this->changeState(NRF24_RECEIVE); // Receive mode
  // if (AUTO_ACK) nrf24_write_ack();	// Write acknowledgment
  this->cehigh();
  ;               // set low to stop listening
  _delay_us(150); // setup time
}

uint8_t NRF24Manager::dataAvailable(void) {
  uint8_t fifo;
  this->readRegister(FIFO_STATUS_REG, &fifo, 1);
  if (!(fifo & (1 << FIFO_STATUS_REG_RX_EMPTY))) {
    return 1;
  }
  return 0;
}

void NRF24Manager::ack() {
  const char *ack = "A";
  unsigned int length = 1;
  this->_spi->begin();
  this->_spi->send(NRF24CMD_W_ACK_PAYLOAD);
  while (length--)
    this->_spi->send(*(uint8_t *)ack++);
  this->_spi->end();
}

const char *NRF24Manager::read_message() {
  // Message placeholder
  static char rx_message[32];
  memset(rx_message, 0, 32);

  // Write ACK message TODO: refactor
  // this->ack();

  // Get length of incoming message
  uint8_t data = 0;
  this->readRegister(NRF24CMD_R_RX_PL_WID, &data, 1);

  // Read message
  if (data > 0)
    this->send_spi(NRF24CMD_R_RX_PAYLOAD, (uint8_t *)&rx_message, data + 1);
  // Clear RX interrupt
  data = (1 << STATUS_REG_RX_DR);
  this->writeRegister(STATUS_REG, &data, 1);

  // Check if there is response message in array
  if (strlen(rx_message) > 0) {
    return rx_message;
  }

  return NULL;
}

//-------------------------------------------------------------------------------------
// active transmit
uint8_t NRF24Manager::send(const char *msg) {
  // Message length
  uint8_t length = strlen(msg);

  // Transmit mode
  this->celow(); // stop receive mode
  this->changeState(NRF24_TRANSMIT);

  // Flush TX/RX and clear TX interrupt
  this->writeRegister(NRF24CMD_FLUSH_RX, 0, 0);
  this->writeRegister(NRF24CMD_FLUSH_TX, 0, 0);

  // Disable interrupt on RX
  // this->readRegister(CONFIG_REG, &data, 1);
  // data |= (1 << CONFIG_REG_MASK_RX_DR);
  // this->writeRegister(CONFIG_REG, &data, 1);

  // Start SPI, load message into TX_PAYLOAD
  this->_spi->begin();
  this->_spi->send(NRF24CMD_W_TX_PAYLOAD);
  while (length--)
    this->_spi->send(*(uint8_t *)msg++);
  this->_spi->send(0);
  this->_spi->end();

  // Send message by pulling CE high for more than 10us
  this->cehigh();
  _delay_us(15); // up to 4 ms
  this->celow();

  if (!this->_autoack) {
    uint8_t data = 0;
    // Wait for message to be sent (TX_DS flag raised)
    this->readRegister(STATUS_REG, &data, 1);
    while (!(data & (1 << STATUS_REG_TX_DS)))
      this->readRegister(STATUS_REG, &data, 1);
  }

  // Enable interrupt on RX
  // this->readRegister(CONFIG_REG, &data, 1);
  // data &= ~(1 << CONFIG_REG_MASK_RX_DR);
  // this->writeRegister(CONFIG_REG, &data, 1);

  // Continue listening
  // nrf24_start_listening();

  return 1;
}

void NRF24Manager::reset(uint8_t autoack) {
  this->_autoack = autoack;

  // Flush TX/RX and clear TX interrupt
  this->writeRegister(NRF24CMD_FLUSH_RX, 0, 0);
  this->writeRegister(NRF24CMD_FLUSH_TX, 0, 0);

  // set config register
  uint8_t config =
      (1 << CONFIG_REG_MASK_RX_DR) |  // interrupt on RX
      (1 << CONFIG_REG_MASK_TX_DS) |  // no interrupt on TX
      (1 << CONFIG_REG_MASK_MAX_RT) | // interrupt on auto retransmit counter
                                      // overflow
      (1 << CONFIG_REG_EN_CRC) |      // CRC enable
      (1 << CONFIG_REG_CRC0) |        // CRC scheme
      (1 << CONFIG_REG_PWR_UP) |      // Power up
      (1 << CONFIG_REG_PRIM_RX); // RX mode by default (set PRIM_RX to 0 for
                                 // transmit)

  uint8_t cmd = config;
  this->writeRegister(CONFIG_REG, &cmd, 1);

  // NRF24 needs some time after power up to be stable
  _delay_ms(2);

  // Enhanced ShockBurst Auto-acknowledge
  if (this->_autoack) {
    cmd = (0 << EN_AA_REG_ENAA_P5) | (0 << EN_AA_REG_ENAA_P4) |
          (0 << EN_AA_REG_ENAA_P3) | (0 << EN_AA_REG_ENAA_P2) |
          (0 << EN_AA_REG_ENAA_P1) | (1 << EN_AA_REG_ENAA_P0);
  } else {
    cmd = (0 << EN_AA_REG_ENAA_P5) | (0 << EN_AA_REG_ENAA_P4) |
          (0 << EN_AA_REG_ENAA_P3) | (0 << EN_AA_REG_ENAA_P2) |
          (0 << EN_AA_REG_ENAA_P1) | (0 << EN_AA_REG_ENAA_P0);
  }
  this->writeRegister(EN_AA_REG, &cmd, 1);


  // Set retries
  if (this->_autoack) {
    cmd = 0xF1; // use max delay of 4000us (F) with 1 retry (1)
  } else {
    cmd = 0xF0; // use max delay of 4000us (F) with 1 retry (1)
  }
  this->writeRegister(SETUP_RETR_REG, &cmd, 1);

  // Enable dynamic payload and autoack if set
  if (this->_autoack) {
    cmd = (1 << FEATURE_REG_EN_DPL) | (1 << FEATURE_REG_EN_ACK_PAY) |
          (1 << FEATURE_REG_EN_DYN_ACK);
  } else {
    cmd = (1 << FEATURE_REG_EN_DPL) | (0 << FEATURE_REG_EN_ACK_PAY) |
          (0 << FEATURE_REG_EN_DYN_ACK);
  }
  this->writeRegister(FEATURE_REG, &cmd, 1);
}

#ifdef HAS_SERIAL
//-------------------------------------------------------------------------------------
// Print register information for debug

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

#else
void NRF24Manager::info()  {}
void NRF24Manager::summary() {}
#endif