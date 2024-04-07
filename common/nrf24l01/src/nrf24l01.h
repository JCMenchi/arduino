#ifndef _NRF24L01_H
#define _NRF24L01_H

//	States
#define NRF24_POWERUP   1
#define NRF24_POWERDOWN	2
#define NRF24_RECEIVE   3
#define NRF24_TRANSMIT	4
#define NRF24_STANDBY1	5
#define NRF24_STANDBY2	6

class SPIManager;


uint8_t nrf24_write(uint8_t register_address, uint8_t *data, unsigned int bytes);
uint8_t nrf24_read(uint8_t register_address, uint8_t *data, unsigned int bytes);

void nrf24_init(SPIManager* spimgr, uint8_t ce_pin);

void nrf24_state(uint8_t state);
void nrf24_start_listening(void);
uint8_t nrf24_available(void);

const char * nrf24_read_message(void);
uint8_t nrf24_send_message(const char* tx_message);

void print_nrf24_summary();
void print_nrf24_info();

//-----------------------------------------------------------------------------------------
// Register

// CONFIG: configuration register
#define CONFIG_REG             0x00
#define CONFIG_REG_MASK_RX_DR  6
#define CONFIG_REG_MASK_TX_DS  5
#define CONFIG_REG_MASK_MAX_RT 4
#define CONFIG_REG_EN_CRC      3
#define CONFIG_REG_CRC0        2
#define CONFIG_REG_PWR_UP      1
#define CONFIG_REG_PRIM_RX     0

// EN_AA: Enhanced ShockBurst
// Enable 'Auto Acknowledgment' function.  Disable this functionality
// to be compatible with nRF2401.
#define EN_AA_REG         0x01
#define EN_AA_REG_ENAA_P5 5
#define EN_AA_REG_ENAA_P4 4
#define EN_AA_REG_ENAA_P3 3
#define EN_AA_REG_ENAA_P2 2
#define EN_AA_REG_ENAA_P1 1
#define EN_AA_REG_ENAA_P0 0

// EN_RXADDR: enable RX addresses
#define EN_RXADDR_REG 0x02

// SETUP_AW: seup of address widths
#define SETUP_AW_REG  0x03

// SETUP_RETR: setup of automatic retransmission
#define SETUP_RETR_REG  0x04

// RF Channel
#define RF_CH_REG 0x05

// RF_SETUP: RF setup register
#define RF_SETUP_REG               0x06
#define RF_SETUP_REG_RF_DR_LOW     5

// STATUS: status register
#define STATUS_REG         0x07
#define STATUS_REG_RX_DR   6
#define STATUS_REG_TX_DS   5
#define STATUS_REG_MAX_RT  4
#define STATUS_REG_RX_P_NO 1
#define STATUS_REG_TX_FULL 0

#define OBSERVE_TX_REG 0x08
#define RPD_REG 0x09

// pipe address
#define RX_ADDR_P0_REG 0x0A
#define RX_ADDR_P1_REG 0x0B
#define TX_ADDR_REG    0x10

// receive buffer
#define RX_PW_P0_REG    0x11
#define RX_PW_P1_REG    0x12

// FIFO_STATUS: FIFO status register
#define FIFO_STATUS_REG           0x17
#define FIFO_STATUS_REG_TX_REUSE  6
#define FIFO_STATUS_REG_FIFO_FULL 5
#define FIFO_STATUS_REG_TX_EMPTY  4
#define FIFO_STATUS_REG_RX_FULL   1
#define FIFO_STATUS_REG_RX_EMPTY  0

// DYNPD: enable dynamic payload length
#define DYNPD_REG        0x1C
#define DYNPD_REG_DPL_P5 5
#define DYNPD_REG_DPL_P4 4
#define DYNPD_REG_DPL_P3 3
#define DYNPD_REG_DPL_P2 2
#define DYNPD_REG_DPL_P1 1
#define DYNPD_REG_DPL_P0 0

// FEATURE:
#define FEATURE_REG            0x1D
#define FEATURE_REG_EN_DPL     2
#define FEATURE_REG_EN_ACK_PAY 1
#define FEATURE_REG_EN_DYN_ACK 0

#endif
