// MIT License
//
// Copyright (c) 2018 Helvijs Adams
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.

// AVR includes
#include <avr/interrupt.h>
#include <avr/io.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <util/delay.h>

// nRF24L01+ include files
#include "nrf24l01.h"

// external lib
#include <gpio.h>
#include <SPIManager.h>
#include <usart_serial.h>

//-----------------------------------------------------------------------------------------
// SPI commands
#define NRF24CMD_ADDRESS_MASK       0x1F  /* 000A AAAA */
#define NRF24CMD_R_REGISTER         0x00 
#define NRF24CMD_W_REGISTER         0b00100000
#define NRF24CMD_R_RX_PAYLOAD       0b01100001
#define NRF24CMD_W_TX_PAYLOAD       0b10100000
#define NRF24CMD_FLUSH_TX           0b11100001
#define NRF24CMD_FLUSH_RX           0b11100010
#define NRF24CMD_REUSE_TX_PL        0b11100011
#define NRF24CMD_ACTIVATE           0b01010000 
#define NRF24CMD_R_RX_PL_WID        0b01100000
#define NRF24CMD_ACK_PAYLOAD_MASK   0b00000111
#define NRF24CMD_W_ACK_PAYLOAD      0b10101000 /* 1010 1PPP | PPP = pipe number */
#define NRF24CMD_W_TX_PAYLOAD_NOACK 0b10110000
#define NRF24CMD_NOP                0xFF

//-----------------------------------------------------------------------------------------
// PIPE address
uint8_t p0_rx_address[5] = {0xe7, 0xe7, 0xe7, 0xe7, 0xe7};
uint8_t p1_rx_address[5] = {0xc2, 0xc2, 0xc2, 0xc2, 0xc2};
uint8_t tx_address[5] = {0xe7, 0xe7, 0xe7, 0xe7, 0xe7};

//
// -PIN map.
// -If CE is changed to different PIN e.g. PC0
// then change DDRB -> DDRC, PORTB -> PORTC and so on
//
// CE
static uint8_t CE_PIN = 3; // CE connected to PB1

#if defined(__AVR_ATmega8535__)
// IRQ
#define IRQ_DDR DDRD
#define IRQ_PORT PORTD
#define IRQ_PIN DDD2 // IRQ connected to PD2

#elif defined(__AVR_ATmega328P__)

// IRQ
#define IRQ_DDR DDRB
#define IRQ_PORT PORTB
#define IRQ_PIN DDB0 // IRQ connected to UNO 8

#endif

// PIN toggling
#define setbit(port, bit) port |= (1 << (bit))
#define clearbit(port, bit) port &= ~(1 << (bit))
#define ce_low clearbit(PORTB, CE_PIN)
#define ce_high setbit(PORTB, CE_PIN)

//-------------------------------------------------------------------------------------
// SPI utilities
SPIManager* myspi = NULL;

// Used to store SPI commands
uint8_t nrf24_send_spi(uint8_t cmd, uint8_t* data, unsigned int bytes) {
  uint8_t status;

  myspi->begin();
  myspi->sendCommand(cmd);
  status = cmd; // sendCommand is replacing sent data with received
  myspi->sendCommandData(bytes, data);
  myspi->end();

  return status;
}

//-------------------------------------------------------------------------------------
// R/W Register
uint8_t nrf24_write(uint8_t register_address, uint8_t *data, unsigned int bytes) {
  return nrf24_send_spi(NRF24CMD_W_REGISTER | register_address, data, bytes);
}

uint8_t nrf24_read(uint8_t register_address, uint8_t *data, unsigned int bytes) {
  return nrf24_send_spi(NRF24CMD_R_REGISTER | register_address, data, bytes);
}

//-------------------------------------------------------------------------------------
// SPI init
void nrf24_init(SPIManager* spimgr, uint8_t ce_pin) 
{
  CE_PIN = ce_pin;

  myspi = spimgr;

  // TODO: rewrite for PB0 PCINT0; 
  cli(); // Disable interrupts
  #if defined(__AVR_ATmega8535__)
  //MCUCR |= (1 << ISC01); // The falling edge of INT0 generates an interrupt request.
  //GICR |= (1 << INT0); // INT0 is on PIN 16 PD2
  #else
  EICRA |= (1 << ISC01); // The falling edge of INT0 generates an interrupt request.
  EIMSK |= (1 << INT0);  // INT0 is on PIN 2 PD2
  #endif
  sei(); // Enable interrupts

  // CE as outputs and initial states
  GPIO_OUTPUT(B, ce_pin);
  ce_low;

  // NRF24 needs 100ms after power on to be stable
  _delay_ms(100);

  // set config register
  uint8_t cmd = (1 << CONFIG_REG_MASK_RX_DR) |  // interrupt on RX
         (1 << CONFIG_REG_MASK_TX_DS) |  // no interrupt on TX
         (1 << CONFIG_REG_MASK_MAX_RT) | // interrupt on auto retransmit counter overflow
         (1 << CONFIG_REG_EN_CRC) |      // CRC enable
         (1 << CONFIG_REG_CRC0) |        // CRC scheme
         (1 << CONFIG_REG_PWR_UP) |      // Power up
         (1 << CONFIG_REG_PRIM_RX);      // RX mode by default (set PRIM_RX to 0 for transmit)
  nrf24_write(CONFIG_REG, &cmd, 1);

  // NRF24 needs some time after power up to be stable
  _delay_ms(2);

  // Enhanced ShockBurst Auto-acknowledge
  uint8_t autoack = 0;
  cmd = (autoack << EN_AA_REG_ENAA_P5) | 
        (autoack << EN_AA_REG_ENAA_P4) | 
        (autoack << EN_AA_REG_ENAA_P3) |
        (autoack << EN_AA_REG_ENAA_P2) | 
        (autoack << EN_AA_REG_ENAA_P1) | 
        (autoack << EN_AA_REG_ENAA_P0);
  nrf24_write(EN_AA_REG, &cmd, 1);

  // set address width to 3 bytes
  cmd = 0x01;
  nrf24_write(SETUP_AW_REG, &cmd, 1);
  
  // Set retries
  cmd = 0xF0; // use max delay of 4000us (F) with no retry (0)
  nrf24_write(SETUP_RETR_REG, &cmd, 1);

  // Sets the frequency channel nRF24L01+
  cmd = 0x0F;
  nrf24_write(RF_CH_REG, &cmd, 1);

  // Setup
  cmd = (1 << RF_SETUP_REG_RF_DR_LOW) | // Data rate low 250kbps
         0x06;  // Set RF output power in TX mode to max 0dBm
  nrf24_write(RF_SETUP_REG, &cmd, 1);

  // Status - clear TX/RX FIFO's and MAX_RT by writing 1 into them
  cmd = (1 << STATUS_REG_RX_DR) | // RX FIFO
        (1 << STATUS_REG_TX_DS) | // TX FIFO
        (1 << STATUS_REG_MAX_RT); // MAX RT
  nrf24_write(STATUS_REG, &cmd, 1);

  // Dynamic payload on all pipes
  cmd = (1 << DYNPD_REG_DPL_P0) | 
        (1 << DYNPD_REG_DPL_P1) |
        (1 << DYNPD_REG_DPL_P2) | 
        (1 << DYNPD_REG_DPL_P3) |
        (1 << DYNPD_REG_DPL_P4) | 
        (1 << DYNPD_REG_DPL_P5);
  nrf24_write(DYNPD_REG, &cmd, 1);

  // Enable dynamic payload
  cmd = (1 << FEATURE_REG_EN_DPL) | 
        (0 << FEATURE_REG_EN_ACK_PAY) |
        (0 << FEATURE_REG_EN_DYN_ACK);
  nrf24_write(FEATURE_REG, &cmd, 1);

  // Open pipes
  nrf24_write(RX_ADDR_P0_REG, p0_rx_address, 5);
  nrf24_write(RX_ADDR_P1_REG, p1_rx_address, 5);
  nrf24_write(TX_ADDR_REG, tx_address, 5);
  // Enable pipes
  cmd = 0x01; // use pipe 0 
  nrf24_write(EN_RXADDR_REG, &cmd, 1);
}

//-------------------------------------------------------------------------------------
// change NRF24 state
void nrf24_state(uint8_t state) {
  uint8_t config_register, data;
  nrf24_read(CONFIG_REG, &config_register, 1);

  switch (state) {
  case NRF24_POWERUP:
    // Check if already powered up
    if (!(config_register & (1 << CONFIG_REG_PWR_UP))) {
      data = config_register | (1 << CONFIG_REG_PWR_UP);
      nrf24_write(CONFIG_REG, &data, 1);
      // 1.5ms from POWERDOWN to start up
      _delay_ms(2);
    }
    break;
  case NRF24_POWERDOWN:
    data = config_register & ~(1 << CONFIG_REG_PWR_UP);
    nrf24_write(CONFIG_REG, &data, 1);
    break;
  case NRF24_RECEIVE:
    data = config_register | (1 << CONFIG_REG_PRIM_RX);
    nrf24_write(CONFIG_REG, &data, 1);
    // Clear STATUS register
    data = (1 << STATUS_REG_RX_DR) | (1 << STATUS_REG_TX_DS) | (1 << STATUS_REG_MAX_RT);
    nrf24_write(STATUS_REG, &data, 1);
    _delay_ms(1);
    break;
  case NRF24_TRANSMIT:
    data = config_register & ~(1 << CONFIG_REG_PRIM_RX);
    nrf24_write(CONFIG_REG, &data, 1);
    _delay_ms(1);
    break;
  case NRF24_STANDBY1:
    ce_low;
    break;
  case NRF24_STANDBY2:
    data = config_register & ~(1 << CONFIG_REG_PRIM_RX);
    nrf24_write(CONFIG_REG, &data, 1);
    ce_high;
    _delay_us(150);
    break;
  }
}

//-------------------------------------------------------------------------------------
// Change to active receiving state
void nrf24_start_listening(void) {
  nrf24_state(NRF24_RECEIVE); // Receive mode
  // if (AUTO_ACK) nrf24_write_ack();	// Write acknowledgment
  ce_high; // set low to stop listening
  _delay_us(150); // setup time
}

uint8_t nrf24_available(void) {
  uint8_t fifo;
  nrf24_read(FIFO_STATUS_REG, &fifo, 1);
  if (!(fifo & (1 << FIFO_STATUS_REG_RX_EMPTY))) {
    // DEBUG
    USART_WriteString("Data in FIFO.\n");
    print_nrf24_summary();
    // DEBUG
    return 1;
  }
  return 0;
}

void nrf24_write_ack(void) {
  const char *ack = "A";
  unsigned int length = 1;
  myspi->begin();
  myspi->send(NRF24CMD_W_ACK_PAYLOAD);
  while (length--)
    myspi->send(*(uint8_t *)ack++);
  myspi->end();
}

const char *nrf24_read_message(void) {
  // Message placeholder
  static char rx_message[32];
  memset(rx_message, 0, 32);

  // Write ACK message TODO: refactor
  nrf24_write_ack();

  // Get length of incoming message
  uint8_t data = 0;
  nrf24_read(NRF24CMD_R_RX_PL_WID, &data, 1);

  // Read message
  if (data > 0)
    nrf24_send_spi(NRF24CMD_R_RX_PAYLOAD, (uint8_t*)&rx_message, data + 1);

  // Check if there is message in array
  if (strlen(rx_message) > 0) {
    // Clear RX interrupt
    data = (1 << STATUS_REG_RX_DR);
    nrf24_write(STATUS_REG, &data, 1);

    return rx_message;
  }

  // Clear RX interrupt
  data = (1 << STATUS_REG_RX_DR);
  nrf24_write(STATUS_REG, &data, 1);

  return "failed";
}

//-------------------------------------------------------------------------------------
// active transmit
uint8_t nrf24_send_message(const char* tx_message) {
  // Message length
  uint8_t length = strlen(tx_message);

  // Transmit mode
  ce_low; // stop receive mode
  nrf24_state(NRF24_TRANSMIT);

  // Flush TX/RX and clear TX interrupt
  nrf24_write(NRF24CMD_FLUSH_RX, 0, 0);
  nrf24_write(NRF24CMD_FLUSH_TX, 0, 0);
  uint8_t data = (1 << STATUS_REG_TX_DS);
  nrf24_write(STATUS_REG, &data, 1);

  // Disable interrupt on RX
  // nrf24_read(CONFIG_REG, &data, 1);
  // data |= (1 << CONFIG_REG_MASK_RX_DR);
  // nrf24_write(CONFIG_REG, &data, 1);

  // Start SPI, load message into TX_PAYLOAD
  myspi->begin();
  myspi->send(NRF24CMD_W_TX_PAYLOAD); // auto ack is active
  while (length--)
    myspi->send(*(uint8_t *)tx_message++);
  myspi->send(0);
  myspi->end();

  // Send message by pulling CE high for more than 10us
  ce_high;
  _delay_ms(1); // up to 4 ms
  ce_low;

  // Wait for message to be sent (TX_DS flag raised)
  nrf24_read(STATUS_REG, &data, 1);
  while (!(data & (1 << STATUS_REG_TX_DS)))
    nrf24_read(STATUS_REG, &data, 1);

  // Enable interrupt on RX
  // nrf24_read(CONFIG_REG, &data, 1);
  // data &= ~(1 << CONFIG_REG_MASK_RX_DR);
  // nrf24_write(CONFIG_REG, &data, 1);

  // Continue listening
  //nrf24_start_listening();

  return 1;
}


ISR(INT0_vect) {  }

//-------------------------------------------------------------------------------------
// Print register information for debug
void print_nrf24_info()
{
    uint8_t buffer[5];

    USART_WriteString("NRF24 info:\n");

    nrf24_read(CONFIG_REG, buffer, 1);
    USART_WriteString("  CONFIG: ");
    USART_WriteUInt(buffer[0], 2);
    USART_WriteString("\n");

    nrf24_read(EN_AA_REG, buffer, 1);
    USART_WriteString("  EN_AA: ");
    USART_WriteUInt(buffer[0], 2);
    USART_WriteString("\n");

    nrf24_read(EN_RXADDR_REG, buffer, 1);
    USART_WriteString("  EN_RXADDR: ");
    USART_WriteUInt(buffer[0], 2);
    USART_WriteString("\n");

    nrf24_read(SETUP_AW_REG, buffer, 1);
    USART_WriteString("  SETUP_AW: ");
    USART_WriteUInt(buffer[0], 2);
    USART_WriteString("\n");

    nrf24_read(SETUP_RETR_REG, buffer, 1);
    USART_WriteString("  SETUP_RETR: ");
    USART_WriteUInt(buffer[0], 2);
    USART_WriteString("\n");

    nrf24_read(RF_CH_REG, buffer, 1);
    USART_WriteString("  RF_CH: ");
    USART_WriteUInt(buffer[0], 2);
    USART_WriteString("\n");

    nrf24_read(RF_SETUP_REG, buffer, 1);
    USART_WriteString("  RF_SETUP: ");
    USART_WriteUInt(buffer[0], 2);
    USART_WriteString("\n");

    nrf24_read(STATUS_REG, buffer, 1);
    USART_WriteString("  STATUS: ");
    USART_WriteUInt(buffer[0], 2);
    USART_WriteString("\n");

    nrf24_read(OBSERVE_TX_REG, buffer, 1);
    USART_WriteString("  OBSERVE_TX: ");
    USART_WriteUInt(buffer[0], 2);
    USART_WriteString("\n");

    nrf24_read(RPD_REG, buffer, 1);
    USART_WriteString("  RPD: ");
    USART_WriteUInt(buffer[0], 2);
    USART_WriteString("\n");

    nrf24_read(FIFO_STATUS_REG, buffer, 1);
    USART_WriteString("  FIFO_STATUS: ");
    USART_WriteUInt(buffer[0], 2);
    USART_WriteString("\n");

    nrf24_read(DYNPD_REG, buffer, 1);
    USART_WriteString("  DYNPD: ");
    USART_WriteUInt(buffer[0], 2);
    USART_WriteString("\n");

    nrf24_read(FEATURE_REG, buffer, 1);
    USART_WriteString("  FEATURE: ");
    USART_WriteUInt(buffer[0], 2);
    USART_WriteString("\n");

    nrf24_read(RX_ADDR_P0_REG, buffer, 5);
    USART_WriteString("  RX_ADDR_P0: ");
    for(uint8_t i = 0; i<5; i++) {
      USART_WriteUInt(buffer[i], 16);USART_WriteString(",");
    }
    USART_WriteString("\n");

    nrf24_read(RX_PW_P0_REG, buffer, 1);
    USART_WriteString("  RX_PW_P0: ");
    USART_WriteUInt(buffer[0]);
    USART_WriteString("\n");

    nrf24_read(RX_ADDR_P1_REG, buffer, 5);
    USART_WriteString("  RX_ADDR_P1: ");
    for(uint8_t i = 0; i<5; i++) {
      USART_WriteUInt(buffer[i], 16);USART_WriteString(",");
    }
    USART_WriteString("\n");
    
    nrf24_read(RX_PW_P1_REG, buffer, 1);
    USART_WriteString("  RX_PW_P1: ");
    USART_WriteUInt(buffer[0]);
    USART_WriteString("\n");

    nrf24_read(TX_ADDR_REG, buffer, 5);
    USART_WriteString("  TX_ADDR: ");
    for(uint8_t i = 0; i<5; i++) {
      USART_WriteUInt(buffer[i], 16);USART_WriteString(",");
    }
    USART_WriteString("\n");
}

void print_nrf24_summary() {
    uint8_t buffer[1];

    USART_WriteString("NRF24 info:\n");

    nrf24_read(CONFIG_REG, buffer, 1);
    USART_WriteString("  CONFIG: ");
    USART_WriteUInt(buffer[0], 2);
    USART_WriteString("\n");

    nrf24_read(STATUS_REG, buffer, 1);
    USART_WriteString("  STATUS: ");
    USART_WriteUInt(buffer[0], 2);
    USART_WriteString("\n");

    nrf24_read(OBSERVE_TX_REG, buffer, 1);
    USART_WriteString("  OBSERVE_TX: ");
    USART_WriteUInt(buffer[0], 2);
    USART_WriteString("\n");

    nrf24_read(RPD_REG, buffer, 1);
    USART_WriteString("  RPD: ");
    USART_WriteUInt(buffer[0], 2);
    USART_WriteString("\n");

    nrf24_read(FIFO_STATUS_REG, buffer, 1);
    USART_WriteString("  FIFO_STATUS: ");
    USART_WriteUInt(buffer[0], 2);
    USART_WriteString("\n");
}