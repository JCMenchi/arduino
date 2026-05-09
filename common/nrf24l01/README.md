# NRF24L01+ Manager Library

A simple C++ library for controlling NRF24L01+ 2.4GHz RF Transceiver modules on AVR microcontrollers. 
This library provides high-level packet communication interfaces.

## Features

- **Fixed or Dynamic Payload Sizes**: Support for 0-32 byte payloads with flexible sizing
- **Auto-Acknowledgment**: Enhanced ShockBurst for reliable packet delivery with automatic ACK
- **ACK Payload Support**: Send data back with acknowledgment packets for bidirectional communication
- **6 RX Pipes**: Multiple receive addresses for multi-node scenarios (simplified to 2 pipes in this implementation)
- **Configurable RF Parameters**: Channel, data rate (1Mbps/2Mbps/250kbps), and TX power settings
- **SPI-based Communication**: Fast command/data transfer to NRF24 registers and FIFOs
- **GPIO Control**: CE (Chip Enable) and CSN (Chip Select) pin management

## Hardware Requirements

- [NRF24L01+](./nrf24l01.md) wireless module
- AVR microcontroller (ATtiny84, ATmega328P, etc.)
- SPI interface with CE and CSN GPIO pins
- external board with 3.3V power supply for NRF24L01+ chip

## Initialization

```cpp
// Include the manager header
#include "nrf24mgr.h"

// Create manager instance with fixed 32-byte payloads
NRF24Manager nrf24(32);

// Or use dynamic payloads
// NRF24Manager nrf24(NRF24_DYNAMIC_PAYLOAD_SIZE);

// Set addresses before initialization
nrf24.setDestinationAddress("2Node");  // TX address (5 bytes)
nrf24.setMyAddress("1Node");            // RX address (5 bytes)

// Initialize with SPI manager and GPIO pins
nrf24.init(&spiManager, CE_PIN, CSN_PIN);
```

## Use Cases

### Use Case 1: One-to-One Bidirectional Communication (Fixed Payload)

Two nodes communicating with each other using fixed size payloads. Each node can send and receive packets independently.

**Configuration:**

- Payload size: 1 to 32 bytes (fixed)
- Communication: Bidirectional
- Each node configured as both sender and receiver

**Node A (Sender/Receiver):**

```cpp
// Setup
NRF24Manager nrf24A(32);
nrf24A.setDestinationAddress("NodeB");  // Where to send
nrf24A.setMyAddress("NodeA");            // Where we listen
nrf24A.init(&spiManager, CE_PIN, CSN_PIN);

// Send data to Node B
uint8_t message[32] = "Hello from Node A";
uint8_t length = strlen((char*)message);
nrf24A.send_binary(message, length);

// Receive data from Node B
while (nrf24A.dataAvailable() == -1) {
  // Wait for data
}
uint8_t rxLength;
uint8_t *rxData = nrf24A.read_binary_message(rxLength);
if (rxData) {
  // Process received data
}
```

**Node B (Sender/Receiver):**

```cpp
// Setup
NRF24Manager nrf24B(32);
nrf24B.setDestinationAddress("NodeA");  // Where to send
nrf24B.setMyAddress("NodeB");            // Where we listen
nrf24B.init(&spiManager, CE_PIN, CSN_PIN);

// Same send/receive pattern as Node A
```

**Advantages:**

- Full bidirectional communication
- Reliable delivery with auto-acknowledgment
- Simple implementation with fixed payload sizes
- Ideal for applications with consistent message sizes

**Limitations:**

- Cannot easily support multiple simultaneous senders
- Fixed payload size may waste bandwidth for shorter messages

---

### Use Case 2: Unidirectional with ACK Payload Response (Master-Slave)

One master node sends commands to a slave node. The slave responds with data using ACK payloads (piggybacked on the acknowledgment).

**Configuration:**

- Payload size: Dynamic
- Communication: Master → Slave with Slave → Master via ACK
- Master always sends, slave always responds
- Reduced latency for slave responses

**Master Node:**

```cpp
// Setup with dynamic payloads for variable-length messages
NRF24Manager nrfMaster(NRF24_DYNAMIC_PAYLOAD_SIZE);
nrfMaster.setDestinationAddress("Slave");  // Send to slave
nrfMaster.setMyAddress("Master");           // Listen for responses
nrfMaster.init(&spiManager, CE_PIN, CSN_PIN);

// Send command to slave
uint8_t command[8] = {0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08};
uint8_t cmdLen = 8;

uint8_t *ackPayload = nrfMaster.send_binary(command, cmdLen);

if (ackPayload != NULL) {
  // Slave responded with ACK payload containing data
  // Process response from slave immediately
  for (int i = 0; i < cmdLen; i++) {
    // Use ackPayload[i]
  }
} else {
  // No ACK received - transmission failed or no response
}
```

**Slave Node:**

```cpp
// Setup with dynamic payloads
NRF24Manager nrfSlave(NRF24_DYNAMIC_PAYLOAD_SIZE);
nrfSlave.setDestinationAddress("Master");  // For ACK (auto-configured)
nrfSlave.setMyAddress("Slave");             // Listen for commands
nrfSlave.init(&spiManager, CE_PIN, CSN_PIN);

// Main loop
while (1) {
  if (nrfSlave.dataAvailable() != -1) {
    // Received command from master
    uint8_t rxLen;
    uint8_t *command = nrfSlave.read_binary_message(rxLen);
    
    if (command != NULL) {
      // Process command and prepare response
      uint8_t response[32];
      uint8_t responseLen = 0;
      
      switch(command[0]) {
        case 0x01:
          // Handle command 0x01
          response[0] = 0xAA;  // Status
          responseLen = 1;
          break;
        // ... more commands
      }
      
      // Send response via ACK payload
      // Must be set BEFORE receiving the next packet
      // this payload will be read during next query from master.
      nrfSlave.set_ack_buffer(response, responseLen);
    }
  }
}
```

**Advantages:**

- Master gets immediate response via ACK without waiting
- Slave always in RX mode (lower power consumption)
- Ideal for query-response patterns (sensor readings, status, etc.)
- Reduces latency compared to separate TX/RX cycles
- Simpler state management - master controls timing

**Limitations:**

- Only works with auto-acknowledgment enabled
- Slave cannot initiate communication

---

## Key Differences Between Use Cases

| Aspect            | Bidirectional (Fixed)   | Master-Slave (ACK Payload) |
| ----------------- | ----------------------- | -------------------------- |
| **Payload Size**  | Fixed                   | Dynamic                    |
| **Initiation**    | Either node can send    | Master only                |
| **Response Time** | Slower (separate cycle) | Faster (via ACK)           |
| **Power (Slave)** | Both TX/RX active       | RX only (lower)            |
| **Complexity**    | Simple                  | Moderate                   |
| **Best For**      | Peer-to-peer            | Query-response, sensors    |

## API Reference

### Public Methods

#### `NRF24Manager(int8_t payloadSize = 32)`

Constructor. Use `NRF24_DYNAMIC_PAYLOAD_SIZE` (-1) for variable payloads.

#### `void init(SPIManager *s, uint8_t ce_pin, uint8_t cs_pin)`

Initialize and configure the module. Must be called before other operations.

#### `void setDestinationAddress(const char address[5])`

Set TX address (where packets are sent). Call before `init()`.

#### `void setMyAddress(const char address[5])`

Set RX address (where this node listens). Call before `init()`.

#### `int8_t dataAvailable()`

Check if packet received. Returns pipe number (0-1) or -1 if empty.

#### `uint8_t* read_binary_message(uint8_t& length)`

Read received packet. Returns pointer to data or NULL if FIFO empty.

#### `uint8_t* send_binary(uint8_t *msg, uint8_t &length)`

Send packet. Returns ACK payload pointer if received, NULL otherwise.

#### `void set_ack_buffer(uint8_t *msg, uint8_t length)`

Set data to send in ACK response (slave use).

#### `void info()` / `void summary()`

Print diagnostic information (requires serial support).

## Default Configuration

- **Address Width**: 5 bytes
- **Channel**: Channel 1 (2401 MHz)
- **Data Rate**: 2 Mbps
- **TX Power**: 0 dBm
- **Auto-ACK**: Enabled on pipes 0 & 1
- **CRC**: Enabled (2 bytes)
- **Retransmission**: 1 attempt with 750µs delay

## Debugging

Enable serial output for diagnostics:

```cpp
// In platformio.ini or build flags
-DHAS_SERIAL  // or -DHAS_INT0_SERIAL

// Then call:
nrf24.info();      // Detailed register dump
nrf24.summary();   // Quick status summary
```

## References

- NRF24L01+ Datasheet
- SPI Communication Protocol
