# SPI Library Documentation

## Overview

The **SPIManager** library provides a lightweight Serial Peripheral Interface (SPI) implementation for AVR microcontrollers (Arduino). It supports both **master** and **slave** modes with configurable hardware and software SPI backends.

## Features

- **Dual Mode Support**: Master and slave mode operation
- **Hardware & Software SPI**: Configurable via preprocessor directives
- **Flexible Clock Control**: Speed selection for master mode (normal/slow)
- **Slave Communication**: Command and data exchange support
- **Special Display Support**: MOSI pin can be configured as input for TFT display controllers
- **Multi-MCU Support**: Compatible with ATmega8535, ATmega328P, ATmega1284P

## Hardware Configuration

The library uses the following pin mappings for different microcontrollers:

### ATmega328P (Arduino UNO)
- **MOSI**: PB3 (Pin 11)
- **MISO**: PB4 (Pin 12)
- **SCK**: PB5 (Pin 13)
- **CS**: PB2 (Pin 10)

### ATmega1284P
- **MOSI**: PB5
- **MISO**: PB6
- **SCK**: PB7
- **CS**: PB4

### ATmega8535
- **MOSI**: PB5
- **MISO**: PB6
- **SCK**: PB7
- **CS**: PB4

## Configuration Macros

Define these macros before compiling to modify SPI behavior:

- `#define SOFTWARE_SPI` - Use software-based SPI (slower but flexible)
- `#define SLOW_SPI` - Reduce SPI clock speed to CPU freq / 64 (default is CPU freq / 4)

## Basic Usage

### Master Mode

```cpp
SPIManager spi;

// Initialize as master
spi.startMaster();

// Transmit a single byte
spi.begin(CS_PIN);
spi.send(0xAA);
spi.end(CS_PIN);

// Transmit multiple bytes
uint8_t data[] = {0x01, 0x02, 0x03};
spi.begin(CS_PIN);
spi.sendData(3, data);
spi.end(CS_PIN);
```

### Slave Mode

```cpp
SPIManager spi;

// Initialize as slave
spi.startSlave();

// Receive a command
uint8_t cmd;
if (spi.receiveCommand(cmd)) {
    // Process command
}

// Execute command with data exchange
uint8_t out_buf[] = {0x10, 0x20};
uint8_t in_buf[2];
spi.execCommand(2, out_buf, in_buf);
```

## API Reference

### Initialization

- `startMaster()` - Initialize as SPI master
- `startSlave()` - Initialize as SPI slave

### Mode Checking

- `isMaster()` - Check if currently in master mode
- `isSlave()` - Check if currently in slave mode

### Master Operations

- `begin(uint8_t cspin)` - Assert chip select (CS) low before transmission
- `end(uint8_t cspin)` - Release chip select (CS) high after transmission
- `send(uint8_t data)` - Send a single byte
- `sendData(uint8_t size, uint8_t* inbuffer)` - Send multiple bytes
- `sendCommand(uint8_t& command)` - Send command and receive response (full-duplex)
- `sendCommandData(uint8_t size, uint8_t* outbuffer, uint8_t* inbuffer)` - Send/receive data
- `sendCommandData(uint8_t size, uint8_t* inoutbuffer)` - Send/receive using single buffer

### Slave Operations

- `receiveCommand(uint8_t& command)` - Receive command from master
- `execCommand(uint8_t size, uint8_t* outbuffer, uint8_t* inbuffer)` - Execute command and exchange data

### Special Features

- `setMosiAsInput()` - Configure MOSI as input (for TFT display controllers)
- `setMosiAsOutput()` - Configure MOSI as output (normal SPI)
- `dummyClock()` - Generate one clock cycle
- `readFromMosi()` - Read 8-bit value from MOSI pin

### Status Information

- `getState()` / `setState(uint8_t s)` - Get/set internal state
- `getStatusRegister()` - Get current SPI status register value

## SPI Protocol Details

### Transfer Format

- **Bit Order**: MSB first
- **Clock Polarity (CPOL)**: 0 (idle low)
- **Clock Phase (CPHA)**: 0 (sample on leading edge)
- **Default Speed**: CPU Freq / 4 (or / 64 with `SLOW_SPI`)

### State Machine

The library maintains an internal state for managing SPI operations:

- `SPI_INIT` (1) - Initial state
- `SPI_WAIT_MASTER` (2) - Waiting for master
- `SPI_WAIT_COMMAND` (3) - Waiting for command
- `SPI_WAIT_DATA` (4) - Waiting for data

## TFT Display Support

For displays like ST7735 that require special MOSI handling:

```cpp
// Switch MOSI to input for display read operations
spi.setMosiAsInput();

// Read data from display
uint8_t value = spi.readFromMosi();

// Generate clock cycles as needed
spi.dummyClock();

// Switch back to output for normal SPI
spi.setMosiAsOutput();
```

## Performance Considerations

- Hardware SPI is significantly faster than software SPI
- Software SPI provides more flexibility but uses more CPU cycles
- The `SLOW_SPI` option reduces speed by 16x to improve signal integrity
- Full-duplex operations (simultaneous send/receive) are most efficient

## Dependencies

- `<avr/common.h>` - AVR common definitions
- `<avr/interrupt.h>` - Interrupt handling (included but not used in standard mode)
- `<avr/io.h>` - I/O port definitions
- `<util/delay.h>` - Delay utilities (used with `SLOW_SPI`)

## License

GNU General Public License v3.0 or later
Copyright (c) 2015 B. Sidhipong &lt;bsidhipong@gmail.com&gt;

## Common Issues & Solutions

| Issue | Solution |
|-------|----------|
| SPI not communicating | Verify CS pin is correctly toggled via `begin()` and `end()` |
| Data corruption | Try enabling `SLOW_SPI` macro or reduce clock speed |
| Inconsistent slave mode | Ensure slave is initialized before master starts transmission |
| MOSI not reading correctly | Call `setMosiAsInput()` and `dummyClock()` for clock generation |
