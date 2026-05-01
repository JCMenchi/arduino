/**
 * @file tinyspi.cpp
 * @brief SPI Manager implementation for AVR microcontrollers
 * 
 * Provides hardware-specific SPI implementations supporting both:
 * - Hardware USART module (ATmega328P, ATmega32, etc.)
 * - USI (Universal Serial Interface) module (ATtiny84, ATtiny45, etc.)
 * 
 * @copyright (c) 2015 B. Sidhipong <bsidhipong@gmail.com>
 * @license GNU General Public License v3.0 or later
 * 
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 * 
 * @note Includes appnote AVR319 implementation for USI-based SPI
 */

#include <avr/common.h>
#include <avr/io.h>
#include <avr/interrupt.h>

#include "tinyspi.h"
#include <gpio.h>
#include <util/delay.h>


/** @brief Temporary buffer for received SPI data */
uint8_t storedSPIData;

/**
 * @struct SPIdriverStatus_t
 * @brief SPI driver status flags
 * 
 * Contains status information for the SPI driver. Analogous to the
 * SPI hardware status register bits.
 */
struct SPIdriverStatus_t {
    /** @brief Master mode flag (1=master, 0=slave) */
    uint8_t masterMode : 1;
    /** @brief Transfer complete flag (set by hardware/ISR) */
    uint8_t transferComplete : 1;
};

/** @brief Volatile driver status flags (updated by hardware and ISR) */
volatile struct SPIdriverStatus_t spiX_status;

//-----------------------------------------------------------------------
// For MCU with SPI support
/** @brief Compile-time check for hardware SPI module availability */
#if defined(SPCR)

/** @brief Device-specific SPI port and pin configuration */
#if defined(__AVR_ATmega8535__) || defined(__AVR_ATmega32__)
/** @brief SPI output port (PORTB) */
#define PORT_SPI    PORTB
/** @brief SPI input port (PINB) */
#define PIN_SPI     PINB
/** @brief SPI direction register (DDRB) */
#define DDR_SPI		DDRB
/** @brief MOSI pin (PB5) */
#define DD_MOSI		DDB5
/** @brief MISO pin (PB6) */
#define DD_MISO		DDB6
/** @brief Clock pin (PB7) */
#define DD_SCK		DDB7
/** @brief Chip select pin (PB4) */
#define DD_CS		DDB4
#elif defined(__AVR_ATmega328P__)
/** @brief SPI output port (PORTB) */
#define PORT_SPI    PORTB
/** @brief SPI input port (PINB) */
#define PIN_SPI     PINB
/** @brief SPI direction register (DDRB) */
#define DDR_SPI		DDRB
/** @brief MOSI pin (PB3) */
#define DD_MOSI		DDB3
/** @brief MISO pin (PB4) */
#define DD_MISO		DDB4
/** @brief Clock pin (PB5) */
#define DD_SCK		DDB5
/** @brief Chip select pin (PB2) */
#define DD_CS		DDB2
#endif

/**
 * @brief Initialize hardware SPI in slave mode
 * 
 * Configures SPI control register for slave operation with mode 0,
 * MSB-first transmission, and no interrupts. MISO pin set to output,
 * other pins to input with CS pullup.
 */
void SPIManager::startSlave() {
    // Configure port directions: MISO output, others input
    DDR_SPI |= _BV(DD_MISO);
    DDR_SPI &= ~(_BV(DD_MOSI) | _BV(DD_SCK) | _BV(DD_CS));

    // SPCR configuration:
    // SPIE=0 (no interrupt), SPE=1 (enable), DORD=0 (MSB first),
    // MSTR=0 (slave), CPOL=0, CPHA=0 (mode 0),
    // SPR1/SPR0=0 (no effect in slave mode - master provides clock)
    SPCR = (0 << SPIE) | (1 << SPE)  | (0 << DORD) | (0 << MSTR) |
           (0 << CPOL) | (0 << CPHA) | (0 << SPR1) | (0 << SPR0);

    // Pull up on ChipSelect to detect deassert
    PORT_SPI |= _BV(DD_CS);

    // Initialize status register and send it to master
    this->_statusRegister = SPI_MODE_SLAVE;
    SPDR = this->_statusRegister;
}

/**
 * @brief Initialize hardware SPI in master mode
 * 
 * Configures SPI control register for master operation with mode 0,
 * MSB-first transmission, CPU clock / 64 speed, and no interrupts.
 * MOSI, SCK, and CS pins set to output; MISO to input.
 */
void SPIManager::startMaster() {
    // Configure port directions: MOSI, SCK, CS output; MISO input
    DDR_SPI &= ~(_BV(DD_MISO));
    DDR_SPI |= (_BV(DD_MOSI) | _BV(DD_SCK) | _BV(DD_CS));

    // SPCR configuration:
    // SPIE=0 (no interrupt), SPE=1 (enable), DORD=0 (MSB first),
    // MSTR=1 (master), CPOL=0, CPHA=0 (mode 0),
    // SPI2X=1, SPR1=1, SPR0=1 (clock = CPU freq / 64)
    SPCR = (0 << SPIE) | (1 << SPE)  | (0 << DORD) | (1 << MSTR) |
           (0 << CPOL) | (0 << CPHA) | (1 << SPI2X) | (1 << SPR1) | (1 << SPR0);

    // Initialize output lines low
    PORT_SPI &= ~(_BV(DD_MOSI) | _BV(DD_SCK) | _BV(DD_CS));

    // Initialize status register and send to slave
    this->_statusRegister = SPI_MODE_MASTER;
    SPDR = this->_statusRegister;
}

/**
 * @brief Assert chip select (pull low)
 * @param cspin GPIO pin number to pull low
 */
void SPIManager::begin(uint8_t cspin) {
    PORT_SPI &= ~_BV(cspin);
}

/**
 * @brief Deassert chip select (pull high)
 * @param cspin GPIO pin number to release high
 */
void SPIManager::end(uint8_t cspin) {
    PORT_SPI |= _BV(cspin);
}

/**
 * @brief Wait for command byte from master (slave mode)
 * 
 * Blocks until either:
 * - SPIF is set (transfer complete - command received)
 * - CS pin goes HIGH (master deasserted chip select)
 * 
 * @param command Reference to store received command byte
 * @return true if command received before CS released, false if CS deasserted first
 */
bool SPIManager::receiveCommand(uint8_t& command) {
    if (this->isMaster()) return false;

    bool success = true;
    // Wait for reception complete (SPIF=1) or CS HIGH (master deasserts)
    do { } while (bit_is_clear(SPSR, SPIF) && bit_is_clear(PIN_SPI, DD_CS));

    // Check if loop exited due to CS release before transfer completion
    if (bit_is_set(PIN_SPI, DD_CS) && bit_is_clear(SPSR, SPIF)) {
        success = false;
    }

    command = SPDR;
    return success;
}

/**
 * @brief Exchange command/response data with master (slave mode)
 * 
 * Performs full-duplex SPI transfer, sending bytes from outbuffer
 * while receiving bytes into inbuffer. Continues until all bytes
 * transferred or CS deasserted by master.
 * 
 * @param size Number of bytes to exchange
 * @param outbuffer Pointer to data to transmit
 * @param inbuffer Pointer to buffer to store received data
 * @return true if all bytes transferred before CS released, false if transfer interrupted
 */
bool SPIManager::execCommand(uint8_t size, uint8_t* outbuffer,  uint8_t* inbuffer) {
    if (this->isMaster()) return false;

    bool success = true;

    for(uint8_t i = 0; i < size; i++) {
        // Load output byte to transmit
        SPDR = outbuffer[i];
        // Wait for transfer complete (SPIF=1) or CS HIGH
        do { } while (bit_is_clear(SPSR, SPIF) && bit_is_clear(PIN_SPI, DD_CS));

        // Check for premature CS release
        if (bit_is_set(PIN_SPI, DD_CS) && bit_is_clear(SPSR, SPIF)) {
            success = false;
            break;
        }

        // Store received byte
        inbuffer[i] = SPDR;
    }
    
    // Preload status register for next command
    SPDR = this->_statusRegister;
    return success;
}

/**
 * @brief Send and receive command byte (master mode)
 * 
 * Performs full-duplex single-byte transfer, replacing outgoing
 * command with received response byte.
 * 
 * @param command Byte to send; receives response byte on return
 * @return true if successful
 */
bool SPIManager::sendCommand(uint8_t& command) {
    if (this->isSlave()) return false;

    SPDR = command;
    // Block until transfer complete (SPIF set)
    loop_until_bit_is_set(SPSR, SPIF);
    
    // Replace command with received response
    command = SPDR;
    return true;
}

/**
 * @brief Send single byte without capturing response (master mode)
 * 
 * @param data Byte to transmit
 * @return true if successful
 */
bool SPIManager::send(uint8_t data) {
    if (this->isSlave()) return false;

    SPDR = data;
    // Block until transfer complete
    loop_until_bit_is_set(SPSR, SPIF);
    return true;
}

/**
 * @brief Exchange data with slave with separate buffers (master mode)
 * 
 * Performs full-duplex transfer sending from outbuffer while
 * receiving into separate inbuffer.
 * 
 * @param size Number of bytes to exchange
 * @param outbuffer Pointer to transmission data
 * @param inbuffer Pointer to receive buffer
 * @return true if successful
 */
bool SPIManager::sendCommandData(uint8_t size, uint8_t* outbuffer,  uint8_t* inbuffer) {
    if (this->isSlave()) return false;

    for(uint8_t i = 0; i < size; i++) {
        // Send output byte
        SPDR = outbuffer[i];
        // Block until transfer complete
        loop_until_bit_is_set(SPSR, SPIF);
        // Store received byte
        inbuffer[i] = SPDR;
    }
    
    return true;
}

/**
 * @brief Exchange data in-place (master mode)
 * 
 * Performs full-duplex transfer using single buffer for both
 * transmission and reception. Each byte is replaced with its
 * corresponding received response.
 * 
 * @param size Number of bytes to exchange
 * @param inoutbuffer Pointer to combined send/receive buffer
 * @return true if successful
 */
bool SPIManager::sendCommandData(uint8_t size, uint8_t* inoutbuffer) {
    if (this->isSlave()) return false;

    for(uint8_t i = 0; i < size; i++) {
        // Send byte from buffer
        SPDR = inoutbuffer[i];
        // Block until transfer complete
        loop_until_bit_is_set(SPSR, SPIF);
        // Replace with received byte
        inoutbuffer[i] = SPDR;
    }
    
    return true;
}

#endif

//-----------------------------------------------------------------------
// For MCU without SPI, but support USI
// Based on Atmel appnote AVR319: Using the USI module for SPI communication
/** @brief Compile-time check for USI module availability */
#if defined(USISR)

/** @brief Device-specific USI port and pin configuration */
#if defined(__AVR_ATtiny84__)
/** @brief USI port output register (PORTA for ATtiny84) */
#define USI_OUT_REG	PORTA
/** @brief USI port input register (PINA) */
#define USI_IN_REG	PINA
/** @brief USI port direction register (DDRA) */
#define USI_DIR_REG	DDRA
/** @brief USI clock pin (PA4 on ATtiny84) */
#define USI_CLOCK_PIN	PA4
/** @brief USI data input pin (PA6 on ATtiny84) */
#define USI_DATAIN_PIN	PA6
/** @brief USI data output pin (PA5 on ATtiny84) */
#define USI_DATAOUT_PIN	PA5
#else
/** @brief USI port output register (PORTB for other ATtiny) */
#define USI_OUT_REG	PORTB
/** @brief USI port input register (PINB) */
#define USI_IN_REG	PINB
/** @brief USI port direction register (DDRB) */
#define USI_DIR_REG	DDRB
/** @brief USI clock pin (PB2 on ATtiny45, etc.) */
#define USI_CLOCK_PIN	PB2
/** @brief USI data input pin (PB0 on ATtiny45, etc.) */
#define USI_DATAIN_PIN	PB0
/** @brief USI data output pin (PB1 on ATtiny45, etc.) */
#define USI_DATAOUT_PIN	PB1
#endif

/**
 * @brief Initialize USI as SPI master
 * 
 * Configures USI module for SPI master operation with software-controlled
 * clock. Sets appropriate pin directions and module configuration.
 * 
 * @note USIDR is cleared by hardware upon configuration
 */
void spi_master_init()
{
    // Configure port directions: data-out and clock as outputs
    USI_DIR_REG |= (1<<USI_DATAOUT_PIN) | (1<<USI_CLOCK_PIN);
    // Data-in as input
    USI_DIR_REG &= ~(1<<USI_DATAIN_PIN);

    // Configure USI to 3-wire master mode with software clock
    // USIWM0=1, USIWM1=0 (3-wire mode)
    // USICS1=1, USICS0=0 (software clock strobe)
    USICR = (1<<USIWM0) | (0<<USIWM1) | (1<<USICS1) | (0<<USICS0);
    
    // Initialize driver status
    spiX_status.masterMode = 1;
}

/**
 * @brief Initialize USI as SPI slave
 * 
 * Configures USI module for SPI slave operation with external clock
 * and overflow interrupt for data exchange notification.
 * 
 * @note USIDR is cleared by hardware upon configuration
 */
void spi_slave_init()
{
    // Configure port directions: data-out as output
    USI_DIR_REG |= (1<<USI_DATAOUT_PIN);
    // Data-in and clock as inputs (controlled by master)
    USI_DIR_REG &= ~(1<<USI_DATAIN_PIN);

    // Configure USI to 3-wire slave mode with external clock and overflow interrupt
    // USIOIE=1 (overflow interrupt enable), USIWM0=1, USIWM1=0 (3-wire mode),
    // USICS1=1, USICS0=0 (external clock)
    USICR = (1<<USIOIE) | (1<<USIWM0) | (1<<USICS1) | (0<<USICS0);

    // Initialize driver status
    spiX_status.masterMode = 0;
}

/**
 * @brief Send and receive byte via USI (master mode)
 * 
 * Loads data into USIDR and clocks out all 8 bits while clocking in
 * response. In master mode, software toggles the clock to shift data.
 * In slave mode, waits for external master clock.
 * 
 * @param val Byte to transmit
 * @return Received byte from slave
 */
uint8_t spi_send(uint8_t val)
{
    // Load byte to transmit into USI data register
    USIDR = val;
    
    if(spiX_status.masterMode == 1) {
        // Master mode: software-controlled clock
        // Clear overflow flag and reset counter for 8-bit transfer
        USISR = (1<<USIOIF);

        // Clock out 8 bits by toggling USITC (User Serial Interface Toggle Clock)
        while((USISR & (1<<USIOIF)) == 0) {
            // USIOIF is set by hardware after 8 clock cycles
            USICR |= (1<<USITC);
        }
    }

    return USIDR;
}

/**
 * @brief Send multiple bytes (master mode)
 * 
 * Transmits array of bytes without capturing receive data.
 * Useful for command sequences or data dumps.
 * 
 * @param send_buffer Pointer to data array
 * @param count Number of bytes to send
 */
void spi_bulk_send(uint8_t *send_buffer, uint8_t count)
{
    while (count--) {
        uint8_t data = *send_buffer++;
        spi_send(data);
    }
}

/**
 * @brief Exchange data with separate send and receive buffers (master mode)
 * 
 * Performs full-duplex transfer, transmitting from send_buffer
 * while receiving into receive_buffer.
 * 
 * @param send_buffer Pointer to transmission data
 * @param receive_buffer Pointer to receive buffer
 * @param count Number of bytes to exchange
 */
void spi_bulk_exchange(uint8_t *send_buffer, uint8_t *receive_buffer, uint8_t count)
{
    while (count--) {
        uint8_t data = *send_buffer++;
        // Send byte and receive response
        *receive_buffer++ = spi_send(data);
    }
}

/**
 * @brief Assert chip select for USI mode (pull low)
 * @param cspin GPIO pin number
 */
void SPIManager::begin(uint8_t cspin) {
    GPIO_SET_LOW(A, cspin);
}

/**
 * @brief Deassert chip select for USI mode (pull high)
 * @param cspin GPIO pin number
 */
void SPIManager::end(uint8_t cspin) {
    GPIO_SET_HIGH(A, cspin);
}

/**
 * @brief Initialize USI for master mode (ATtiny)
 */
void SPIManager::startMaster() {
    this->_statusRegister = SPI_MODE_MASTER;
    spi_master_init();
}

/**
 * @brief Initialize USI for slave mode (ATtiny)
 */
void SPIManager::startSlave() {
    this->_statusRegister = SPI_MODE_SLAVE;
    spi_slave_init();
}

/**
 * @brief Send and receive command byte (USI mode)
 * @param command Byte to send; receives response on return
 * @return true
 */
bool SPIManager::sendCommand(uint8_t& command) {
    command = spi_send(command);
    return true;
}

/**
 * @brief Send single byte (USI mode)
 * @param data Byte to transmit
 * @return true
 */
bool SPIManager::send(uint8_t data) {
    spi_send(data);
    return true;
}

/**
 * @brief Exchange data in-place (USI mode)
 * @param size Number of bytes to exchange
 * @param inoutbuffer Combined send/receive buffer
 * @return true
 */
bool SPIManager::sendCommandData(uint8_t size, uint8_t* inoutbuffer) {
    spi_bulk_exchange(inoutbuffer, inoutbuffer, size);
    return true;
}

/**
 * @brief Exchange data with slave with separate buffers (master mode)
 * 
 * Performs full-duplex transfer sending from outbuffer while
 * receiving into separate inbuffer.
 * 
 * @param size Number of bytes to exchange
 * @param outbuffer Pointer to transmission data
 * @param inbuffer Pointer to receive buffer
 * @return true if successful
 */
bool SPIManager::sendCommandData(uint8_t size, uint8_t* outbuffer,  uint8_t* inbuffer) {
    if (this->isSlave()) return false;

    spi_bulk_exchange(outbuffer, inbuffer, size);
    
    return true;
}

#endif
