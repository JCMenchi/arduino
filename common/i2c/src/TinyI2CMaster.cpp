
/**
 * @file TinyI2CMaster.cpp
 * @brief Implementation of I2C master library for ATtiny and ATmega microcontrollers.
 * 
 * This file contains hardware-specific implementations for two different microcontroller
 * families:
 * 
 * **1. ATtiny with USI (Universal Serial Interface) - USIDR defined:**
 *   - Used for ATtiny45, ATtiny85, ATtiny84, and similar chips
 *   - Implements bit-banging using the USI peripheral
 *   - Software-controlled SCL clock generation
 *   - SDA line controlled via open-drain output
 * 
 * **2. ATmega with TWI (Two-Wire Interface) - TWDR defined:**
 *   - Used for ATmega328P, ATmega8535, ATmega1284P, and similar chips
 *   - Hardware I2C controller (more efficient than bit-banging)
 *   - Automatic clock generation and protocol handling
 * 
 * The implementation uses conditional compilation (#if defined) to select the
 * appropriate code path based on available hardware registers detected at compile time.
 * 
 * @note Requires F_CPU to be defined for proper I2C timing calculations.
 * @note Both implementations are blocking and do not use interrupts.
 */

#include "TinyI2CMaster.h"

#include <avr/io.h>
#include <util/delay.h>

#ifdef HAS_SERIAL
#include <usart_serial.h>
#endif

#ifdef HAS_INT0_SERIAL
#include <int0_serial.h>
#include <avr/pgmspace.h>
#endif

/**
 * @brief Constructor initializes I2C state variables.
 * 
 * Sets I2Ccount to 0 (no pending reads) and initialised flag to false.
 * Hardware setup must be done separately with init().
 */
TinyI2CMaster::TinyI2CMaster() : I2Ccount(0), initialised(false) {}

#if defined(SOFTWARE_I2C)

#include <gpio.h>

#ifndef I2C_PORT
#define I2C_PORT B
#endif

#ifndef I2C_SCL_PIN
#define I2C_SCL_PIN PB0
#endif

#ifndef I2C_SDA_PIN
#define I2C_SDA_PIN PB1
#endif


#define DELAY_I2C (_delay_loop_1(3))
#define PULSE_CLOCK (_delay_loop_1(3))

#define SDA_HIGH GPIO_INPUT(I2C_PORT, I2C_SDA_PIN)

// set SCL high with clock stretching
#define SCL_HIGH GPIO_INPUT(I2C_PORT, I2C_SCL_PIN); while((PINB & 0x01)==0)

#define SDA_LOW GPIO_OUTPUT(I2C_PORT, I2C_SDA_PIN); GPIO_SET_LOW(I2C_PORT, I2C_SDA_PIN)
#define SCL_LOW GPIO_OUTPUT(I2C_PORT, I2C_SCL_PIN); GPIO_SET_LOW(I2C_PORT, I2C_SCL_PIN)

uint8_t TinyI2CMaster::transfer(uint8_t data) {
  
  return data; 
}

/*
  Init GPIO. Be sure to add pullup resistor on those 2 pins (3.3 kOhm is good for 5V, 4.7 kOhm is good for 3.3V)

  High resistance external pull-up is required for I2C lines to ensure proper logic levels and bus stability. 
  The open-drain configuration allows multiple devices to share the bus without contention, 
  as any device can pull the line low but cannot drive it high. 
*/
void TinyI2CMaster::init(bool fast) {
  if (initialised) return;

  // Configure SCL and SDA pins as open-drain outputs (initially high thanks to external pullup restistor)
  SDA_HIGH;
  SCL_HIGH;
  DELAY_I2C;

  initialised = true; // Set initialised flag
}

bool TinyI2CMaster::start(uint8_t address, uint8_t readcount) {

  if (readcount != 0) {
    I2Ccount = readcount;
    readcount = 1;  // Set R/W bit for read
  }
  uint8_t addressRW = address << 1 | readcount; // Shift address and set R/W bit

  // send start sequence: SDA goes low while SCL is high, then SCL goes low
  // stop or init have left both SDA and SCL HIGH
  SDA_LOW;
  PULSE_CLOCK;
  SCL_LOW;

  if (!write(addressRW)) {
    stop(); // If address not acknowledged, send STOP condition
    #if defined(HAS_INT0_SERIAL) && defined(I2C_DEBUG)
    INT0_WritePString(PSTR("I2C start failed\n"));
    #endif
    return false; // Start failed due to no ACK
  }

  return true; // Start successfully completed
}

bool TinyI2CMaster::restart(uint8_t address, uint8_t readcount) {
  return TinyI2CMaster::start(address, readcount);
}

void TinyI2CMaster::stop(void) {
  // last start/read/write has left SCL & SDA LOW

  // send stop sequence
  SCL_HIGH;
  PULSE_CLOCK;
  SDA_HIGH;
}

bool TinyI2CMaster::write(uint8_t data) {

  for(int8_t i = 7; i >= 0; i--) {
    // Set SDA according to the current bit
    if (data & (1 << i)) {
      SDA_HIGH;
    } else {
      SDA_LOW;
    }
    // Generate clock pulse on SCL (start has left SCL low, so we can just toggle it)
    SCL_HIGH;
    PULSE_CLOCK;
    SCL_LOW;
  }

  // read acknowledgment bit from slave
  SDA_HIGH; // set SDA as input

  SCL_HIGH; // Clock high to allow slave to send ACK
  PULSE_CLOCK;
  bool ack = (GPIO_READ(I2C_PORT, I2C_SDA_PIN) == GPIO_LOW); // ACK is active low
  SCL_LOW; // Clock low
  SDA_LOW; // leave SDA LOW

  if (!ack) {
    #if defined(HAS_INT0_SERIAL) && defined(I2C_DEBUG)
    INT0_WritePString(PSTR("I2C write NACK\n"));
    #endif
    return false; // No ACK received, write failed
  }

  return true; // Write successfully completed
}


uint8_t TinyI2CMaster::read(void) {
  if ((I2Ccount != 0) && (I2Ccount != -1))
    I2Ccount--;

  SDA_HIGH; // set as input
  uint8_t data = 0;

  for(int8_t i = 7; i >= 0; i--) {
    DELAY_I2C;
    // Generate clock pulse on SCL (start has left SCL low, so we can just toggle it)
    SCL_HIGH;
    PULSE_CLOCK;

    // Set SDA according to the current bit
    if ((GPIO_READ(I2C_PORT, I2C_SDA_PIN) == GPIO_HIGH)) {
      data |= (1 << i);
    }
    
    SCL_LOW;
  }

  bool ack = I2Ccount == 0;
  if (ack) {
    SDA_HIGH; 
  } else {
    SDA_LOW; 
  }
 
  SCL_HIGH; // pulse clock
  PULSE_CLOCK;
  SCL_LOW;

  // keep SDA LOW
  SDA_LOW; 

  return data; 
}

uint8_t TinyI2CMaster::readLast(void) {
  I2Ccount = 0;
  return TinyI2CMaster::read();
}

#elif defined(USIDR)

/**
 * @defgroup ATtinyUSI ATtiny USI-based I2C Implementation
 * @brief Hardware-specific implementation using the Universal Serial Interface.
 * 
 * This implementation section is compiled when USIDR (USI Data Register) is defined,
 * indicating an ATtiny microcontroller with a USI peripheral.
 * 
 * **Communication Protocol:**
 * - Uses software-controlled clock generation (bit-banging)
 * - Clock stretching is handled by polling SCL for high state
 * - Timing constants define delays for standard vs. fast I2C modes
 * - SDA and SCL are controlled via open-drain outputs (must have pull-ups)
 * 
 * **USI Registers Used:**
 * - USISR: Status Register (flags, bit counter)
 * - USICR: Control Register (mode, clock source, toggle control)
 * - USIDR: Data Register (I2C data and output control)
 * 
 * @{
 */

/**
 * @defgroup I2CTimingConstants I2C Timing Delays
 * @brief Timing delays for I2C protocol compliance.
 * 
 * These delays ensure the I2C bus operates within specification for either
 * standard (100 kHz) or fast (400 kHz) mode.
 * 
 * @{
 */

#define TWI_FAST_MODE

#ifdef TWI_FAST_MODE // TWI FAST mode timing limits. SCL = 100-400kHz
  /** T2 timing: Delay between SCL high to SDA setup, >1.3us for fast mode */
  #define DELAY_T2TWI (_delay_us(2)) // >1.3us
  
  /** T4 timing: Delay for SCL-to-SDA hold time, >0.6us for fast mode */
  #define DELAY_T4TWI (_delay_us(1)) // >0.6us
#else // TWI STANDARD mode timing limits. SCL <= 100kHz
  /** T2 timing: Delay between SCL high to SDA setup, >4.7us for standard mode */
  #define DELAY_T2TWI (_delay_us(5)) // >4.7us
  
  /** T4 timing: Delay for SCL-to-SDA hold time, >4.0us for standard mode */
  #define DELAY_T4TWI (_delay_us(4)) // >4.0us
#endif

/// @}

/** Bit position of the (N)ACK bit in the received byte during acknowledge phase */
#define TWI_NACK_BIT 0

/**
 * @defgroup PinDefinitions ATtiny Pin Definitions
 * @brief I2C pin mapping for specific ATtiny devices.
 * 
 * Defines which I/O pins are used for SCL (clock) and SDA (data) on different
 * ATtiny microcontrollers. Includes PORT, DDR, and PIN register mappings.
 * 
 * @{
 */

#if defined (__AVR_ATtiny45__)
  /** SCL (Serial Clock) pin for ATtiny45 */
  #define PIN_USI_SCL PINB2
  
  /** SDA (Serial Data) pin for ATtiny45 */
  #define PIN_USI_SDA PINB0
  
  /** Port letter for I2C pins on ATtiny45 (Port B) */
  #define PIN_USI_CL PINB
  
  /** DDR register for I2C on ATtiny45 */
  #define DDR_USI DDRB
  
  /** DDR register for I2C clock pin on ATtiny45 */
  #define DDR_USI_CL DDRB
  
  /** PORT register for I2C on ATtiny45 */
  #define PORT_USI PORTB
  
  /** PORT register for I2C clock pin on ATtiny45 */
  #define PORT_USI_CL PORTB
  
#elif defined (__AVR_ATtiny84__)
  /** SCL (Serial Clock) pin for ATtiny84 */
  #define PIN_USI_SCL PINA4
  
  /** SDA (Serial Data) pin for ATtiny84 */
  #define PIN_USI_SDA PINA6
  
  /** Port letter for I2C pins on ATtiny84 (Port A) */
  #define PIN_USI_CL PINA
  
  /** DDR register for I2C on ATtiny84 */
  #define DDR_USI DDRA
  
  /** DDR register for I2C clock pin on ATtiny84 */
  #define DDR_USI_CL DDRA
  
  /** PORT register for I2C on ATtiny84 */
  #define PORT_USI PORTA
  
  /** PORT register for I2C clock pin on ATtiny84 */
  #define PORT_USI_CL PORTA
#endif

/// @}

/**
 * @defgroup USIRegisterConstants USI Register Configuration Constants
 * @brief Pre-calculated values for common USI register configurations.
 * 
 * These constants represent different USI shift register configurations used during
 * I2C communication. They encode which flags to clear and how many bits to shift.
 * 
 * @{
 */

/**
 * 8-bit shift configuration for data transfers.
 * 
 * Clears flags (USISIF, USIOIF, USIPF, USIDC) and sets the counter for 16 clock
 * edges (2 per bit × 8 bits = 16 edges), which shifts exactly 8 bits of data.
 */
unsigned char const USISR_8bit =
    1 << USISIF | 1 << USIOIF | 1 << USIPF | 1 << USIDC | 0x0 << USICNT0;

/**
 * 1-bit shift configuration for ACK/NACK bit handling.
 * 
 * Clears the same flags and sets the counter for 2 clock edges (1 bit transfer).
 * Used during the acknowledge bit phase of the I2C protocol.
 */
unsigned char const USISR_1bit =
    1 << USISIF | 1 << USIOIF | 1 << USIPF | 1 << USIDC | 0xE << USICNT0;

/// @}

/**
 * @brief Low-level USI transfer function for one complete byte or bit sequence.
 * 
 * Performs a complete bit-bang transfer cycle using the USI peripheral in software
 * clock mode. Handles the I2C protocol timing and clock stretching.
 * 
 * **Transfer Cycle:**
 * 1. Load USISR with bit configuration (8-bit or 1-bit mode)
 * 2. Configure USI for two-wire mode with software clock control
 * 3. Loop: Generate SCL pulse (low→high→low), wait for SCL release
 * 4. Repeat until transfer counter reaches zero (USIOIF flag set)
 * 5. Release SDA (write 0xFF to USIDR to release the line)
 * 6. Enable SDA as output
 * 7. Return the received byte from USIDR
 * 
 * **Clock Stretching:**
 * If the slave device holds SCL low (clock stretching), the master waits in the
 * loop until the slave releases SCL.
 * 
 * @param data USISR value indicating transfer type (USISR_8bit or USISR_1bit)
 * 
 * @return The byte received on the I2C bus. For write operations, this contains
 *         the device's ACK/NACK response in bit 0.
 * 
 * @see USISR_8bit
 * @see USISR_1bit
 */
uint8_t TinyI2CMaster::transfer(uint8_t data) {
  USISR = data;                      // Set USISR according to data.
                                     // Prepare clocking.
  data = 0 << USISIE | 0 << USIOIE | // Interrupts disabled
         1 << USIWM1 | 0 << USIWM0 | // Set USI in Two-wire mode.
         1 << USICS1 | 0 << USICS0 |
         1 << USICLK | // Software clock strobe as source.
         1 << USITC;   // Toggle Clock Port.
  do {
    DELAY_T2TWI;
    USICR = data; // Generate positive SCL edge.
    while (!(PIN_USI_CL & 1 << PIN_USI_SCL))
      ; // Wait for SCL to go high (clock stretching).
    DELAY_T4TWI;
    USICR = data;                   // Generate negative SCL edge.
  } while (!(USISR & 1 << USIOIF)); // Check for transfer complete.

  DELAY_T2TWI;
  data = USIDR;                  // Read out data.
  USIDR = 0xFF;                  // Release SDA.
  DDR_USI |= (1 << PIN_USI_SDA); // Enable SDA as output.

  return data; // Return the data from the USIDR
}

/**
 * @brief Initializes the I2C bus for USI-based microcontroller.
 * 
 * Configures GPIO pins for open-drain I2C operation and sets up the USI peripheral.
 * 
 * **Configuration Steps:**
 * 1. Enable pull-ups on both SDA and SCL via PORT registers
 * 2. Configure SDA and SCL as outputs (open-drain via DDR)
 * 3. Preload USIDR with 0xFF (releases SDA when output)
 * 4. Configure USICR for two-wire mode, software clock source
 * 5. Reset USISR (clear flags, reset counter)
 * 6. Set initialised flag to prevent re-initialization
 * 
 * @param fast Unused for USI implementation (timing is fixed based on compile-time mode)
 * 
 * @see #define TWI_FAST_MODE
 */
void TinyI2CMaster::init(bool fast) {
  if (initialised) return;
  PORT_USI |= 1 << PIN_USI_SDA;    // Enable pullup on SDA.
  PORT_USI_CL |= 1 << PIN_USI_SCL; // Enable pullup on SCL.

  DDR_USI_CL |= 1 << PIN_USI_SCL; // Enable SCL as output.
  DDR_USI |= 1 << PIN_USI_SDA;    // Enable SDA as output.

  USIDR = 0xFF;                       // Preload data register with data.
  USICR = 0 << USISIE | 0 << USIOIE | // Disable Interrupts.
          1 << USIWM1 | 0 << USIWM0 | // Set USI in Two-wire mode.
          1 << USICS1 | 0 << USICS0 |
          1 << USICLK | // Software stobe as counter clock source
          0 << USITC;
  USISR = 1 << USISIF | 1 << USIOIF | 1 << USIPF | 1 << USIDC | // Clear flags,
          0x0 << USICNT0; // and reset counter.

  initialised = true; // Set initialised flag
}

/**
 * @brief Reads a single byte from the I2C bus and sends ACK.
 * 
 * **Read Sequence:**
 * 1. Decrement read counter (if not 0 or -1)
 * 2. Release SDA to allow slave to drive data
 * 3. Perform 8-bit transfer to receive the byte
 * 4. If more bytes expected, send ACK (drive SDA low)
 * 5. If last byte, send NACK (release SDA to remain high)
 * 6. Return the received byte
 * 
 * The I2Ccount variable tracks remaining bytes. After reaching 0,
 * a NACK is sent (SDA released = high) to indicate end of transmission.
 * 
 * @return The 8-bit data byte received from the slave
 * 
 * @see readLast()
 * @see I2Ccount
 */
uint8_t TinyI2CMaster::read(void) {
  if ((I2Ccount != 0) && (I2Ccount != -1))
    I2Ccount--;

  /* Read a byte */
  DDR_USI &= ~(1 << PIN_USI_SDA); // Enable SDA as input.
  uint8_t data = TinyI2CMaster::transfer(USISR_8bit);

  /* Prepare to generate ACK (or NACK in case of End Of Transmission) */
  if (I2Ccount == 0)
    USIDR = 0xFF;  // NACK: Release SDA (stays high)
  else
    USIDR = 0x00;  // ACK: Drive SDA low
  TinyI2CMaster::transfer(USISR_1bit); // Generate ACK/NACK.

  return data; // Read successfully completed
}

/**
 * @brief Reads the final data byte from the I2C bus and sends NACK.
 * 
 * Convenience function that sets I2Ccount to 0 before calling read(),
 * ensuring a NACK is sent after this byte. This signals the slave that
 * no more bytes will be requested.
 * 
 * @return The 8-bit data byte received from the slave
 * 
 * @see read()
 */
uint8_t TinyI2CMaster::readLast(void) {
  I2Ccount = 0;
  return TinyI2CMaster::read();
}

/**
 * @brief Writes a single data byte to the I2C bus.
 * 
 * **Write Sequence:**
 * 1. Pull SCL low to ensure stable data phase
 * 2. Load byte into USIDR
 * 3. Perform 8-bit transfer to send the byte
 * 4. Release SDA to allow slave to pull it low (ACK)
 * 5. Perform 1-bit transfer to receive ACK/NACK bit
 * 6. Return true if ACK received, false if NACK
 * 
 * The ACK bit is checked at bit position TWI_NACK_BIT. If the slave
 * pulls SDA low (ACK), the bit reads as 0. If not acknowledged (NACK),
 * SDA remains high and the bit reads as 1.
 * 
 * @param data The 8-bit value to transmit to the slave
 * 
 * @return true if slave acknowledged (ACK received),
 *         false if slave sent NACK (not acknowledged or error)
 * 
 * @see TWI_NACK_BIT
 */
bool TinyI2CMaster::write(uint8_t data) {
  /* Write a byte */
  PORT_USI_CL &= ~(1 << PIN_USI_SCL);  // Pull SCL LOW.
  USIDR = data;                        // Setup data.
  TinyI2CMaster::transfer(USISR_8bit); // Send 8 bits on bus.

  /* Clock and verify (N)ACK from slave */
  DDR_USI &= ~(1 << PIN_USI_SDA); // Enable SDA as input.
  if (TinyI2CMaster::transfer(USISR_1bit) & 1 << TWI_NACK_BIT)
    return false;  // NACK received

  return true; // Write successfully completed
}

/**
 * @brief Initiates I2C START condition and sends slave address.
 * 
 * **START Sequence:**
 * 1. Release SCL and wait for it to go high (clock stretching)
 * 2. Verify START condition flag (USISIF) is set
 * 3. Shift the 7-bit address left and add R/W bit (LSB)
 * 4. Pull SCL low
 * 5. Send the address byte (8 bits including R/W)
 * 6. Release SDA and check for ACK from slave
 * 
 * **R/W Bit:**
 * - readcount = 0: Write mode (R/W bit = 0)
 * - readcount > 0: Read mode (R/W bit = 1), and I2Ccount is set to readcount
 * 
 * The function performs clock stretching detection and waits for the slave
 * to acknowledge the address.
 * 
 * @param address 7-bit I2C slave address (0x00-0x7F)
 * @param readcount Number of bytes expected to read. If 0, write mode selected.
 *                  If > 0, read mode selected and byte counter initialized.
 * 
 * @return true if START condition generated and slave acknowledged address,
 *         false if START failed or no ACK from slave
 * 
 * @see restart()
 */
bool TinyI2CMaster::start(uint8_t address, uint8_t readcount) {
  if (readcount != 0) {
    I2Ccount = readcount;
    readcount = 1;  // Set R/W bit for read
  }
  uint8_t addressRW = address << 1 | readcount;

  /* Release SCL to ensure that (repeated) Start can be performed */
  PORT_USI_CL |= 1 << PIN_USI_SCL; // Release SCL.
  while (!(PIN_USI_CL & 1 << PIN_USI_SCL))
    ; // Verify that SCL becomes high (clock stretching).
#ifdef TWI_FAST_MODE
  DELAY_T4TWI;
#else
  DELAY_T2TWI;
#endif

  /* Generate Start Condition */
  PORT_USI &= ~(1 << PIN_USI_SDA); // Force SDA LOW.
  DELAY_T4TWI;
  PORT_USI_CL &= ~(1 << PIN_USI_SCL); // Pull SCL LOW.
  PORT_USI |= 1 << PIN_USI_SDA;       // Release SDA.

  if (!(USISR & 1 << USISIF))
    return false;  // START condition not detected

  /*Write address */
  PORT_USI_CL &= ~(1 << PIN_USI_SCL);  // Pull SCL LOW.
  USIDR = addressRW;                   // Setup data.
  TinyI2CMaster::transfer(USISR_8bit); // Send 8 bits on bus.

  /* Clock and verify (N)ACK from slave */
  DDR_USI &= ~(1 << PIN_USI_SDA); // Enable SDA as input.
  if (TinyI2CMaster::transfer(USISR_1bit) & 1 << TWI_NACK_BIT)
    return false; // No ACK

  return true; // Start successfully completed
}

/**
 * @brief Generates a REPEATED START condition for direction change.
 * 
 * For the USI implementation, this simply calls start() with the new address
 * and readcount. The START condition is automatically repeated without a STOP.
 * 
 * @param address 7-bit slave address for the new transaction
 * @param readcount 0 for write mode, > 0 for read mode
 * 
 * @return true if repeated START and address acknowledge successful,
 *         false if failed
 * 
 * @see start()
 */
bool TinyI2CMaster::restart(uint8_t address, uint8_t readcount) {
  return TinyI2CMaster::start(address, readcount);
}

/**
 * @brief Generates I2C STOP condition and releases the bus.
 * 
 * **STOP Sequence:**
 * 1. Pull SDA low
 * 2. Release SCL (allow to go high)
 * 3. Wait for SCL to actually go high
 * 4. Release SDA (goes high due to pull-up)
 * 5. Add timing delay for protocol compliance
 * 
 * After this function, the I2C bus is idle and available for other devices
 * or the next START condition.
 * 
 * @see start()
 * @see restart()
 */
void TinyI2CMaster::stop(void) {
  PORT_USI &= ~(1 << PIN_USI_SDA); // Pull SDA low.
  PORT_USI_CL |= 1 << PIN_USI_SCL; // Release SCL.
  while (!(PIN_USI_CL & 1 << PIN_USI_SCL))
    ; // Wait for SCL to go high.
  DELAY_T4TWI;
  PORT_USI |= 1 << PIN_USI_SDA; // Release SDA.
  DELAY_T2TWI;
}

/// @} // End of ATtinyUSI group

#elif defined(TWDR)

/**
 * @defgroup ATmegaTWI ATmega TWI-based I2C Implementation
 * @brief Hardware-specific implementation using the Two-Wire Interface (TWI).
 * 
 * This implementation section is compiled when TWDR (TWI Data Register) is defined,
 * indicating an ATmega microcontroller with a hardware I2C/TWI controller.
 * 
 * **Hardware I2C Advantages:**
 * - Automatic clock generation (no bit-banging required)
 * - Automatic protocol handling (START, STOP, ACK/NACK)
 * - More efficient and predictable timing
 * - Slave mode support (not used in this master-only implementation)
 * 
 * **Registers Used:**
 * - TWCR: Control Register (START, STOP, ACK, interrupt enable)
 * - TWSR: Status Register (I2C state indicators)
 * - TWDR: Data Register (address and data bytes)
 * - TWBR: Bit Rate Register (I2C clock frequency)
 * 
 * **Timing:**
 * The bit rate is calculated as: SCL_frequency = F_CPU / (16 + 2 * TWBR * Prescaler)
 * For standard mode (100 kHz) at 1 MHz: TWBR = (1,000,000 / 100,000 - 16) / 2 = 42
 * 
 * @{
 */

/**
 * @defgroup TWIConstants TWI Status and Constant Definitions
 * @brief Status codes and I2C mode constants for TWI implementation.
 * @{
 */

/** Normal I2C clock rate (100 kHz standard mode) */
uint32_t const F_TWI_NORMAL = 40000L;   // Hardware I2C clock in Hz

/** Fast I2C clock rate (400 kHz fast mode) */
uint32_t const F_TWI_FAST = 400000L;    // Hardware I2C clock in Hz

/** TWI Status Code: Master Transmit Data Acknowledged */
const uint8_t TWSR_MTX_DATA_ACK = 0x28;

/** TWI Status Code: Master Transmit Address Acknowledged */
const uint8_t TWSR_MTX_ADR_ACK = 0x18;

/** TWI Status Code: Master Receive Address Acknowledged */
const uint8_t TWSR_MRX_ADR_ACK = 0x40;

/** TWI Status Code: START condition transmitted */
const uint8_t TWSR_START = 0x08;

/** TWI Status Code: REPEATED START condition transmitted */
const uint8_t TWSR_REP_START = 0x10;

/** I2C Read bit (R/W bit = 1 for read) */
const uint8_t I2C_READ = 1;

/** I2C Write bit (R/W bit = 0 for write) */
const uint8_t I2C_WRITE = 0;

/// @}

/**
 * @brief Initializes the I2C bus for TWI-based microcontroller.
 * 
 * Configures GPIO pins for open-drain I2C operation and sets up the TWI hardware.
 * 
 * **Configuration Steps:**
 * 1. Configure GPIO: Set SCL and SDA as inputs with pull-ups enabled
 *    (Open-drain mode via DDR and PORT registers)
 * 2. Set TWI Status Register to 0 (prescaler = 1)
 * 3. Set TWBR (bit rate register) based on fast parameter:
 *    - Fast mode: TWBR = 3 (400 kHz)
 *    - Standard mode: TWBR = (F_CPU / F_TWI_NORMAL - 16) / 2
 * 4. Set initialised flag
 * 
 * **Pin Mapping by Device:**
 * - ATmega328P (UNO): SCL = PC5, SDA = PC4
 * - ATmega8535/16/32: SCL = PC0, SDA = PC1
 * - ATmega1284P: SCL = PC0, SDA = PC1
 * 
 * @param fast If true, configure 400 kHz fast mode; if false, standard mode
 * 
 * @note F_CPU must be defined for correct timing calculations
 * @note Compilation will fail if the microcontroller is not recognized
 */
void TinyI2CMaster::init(bool fast) {
  
  if (initialised) return;
  // activate pull up
#if defined (__AVR_ATmega328P__)
  // ATMega328P (UNO) => SCL = PC5 SDA = PC4
  DDRC &= ~((1 << DDC4)|(1 << DDC5));
  PORTC |= (1 << PORTC4) | (1 << PORTC5);
#elif defined (__AVR_ATmega8515__)
  #error "No TWI for 8515"
#elif defined (__AVR_ATmega8535__) || defined (__AVR_ATmega16__) || defined (__AVR_ATmega32__)
  // ATMega8535 => SCL = PC0 SDA = PC1
  DDRC &= ~((1 << DDC0)|(1 << DDC1));
  PORTC |= (1 << PORTC0) | (1 << PORTC1);
#elif defined (__AVR_ATmega1284P__)
  // ATMega1284P => SCL = PC0 SDA = PC1
  DDRC &= ~((1 << DDC0)|(1 << DDC1));
  PORTC |= (1 << PORTC0) | (1 << PORTC1);
#else
  #error "No yet configured, check datasheet"
#endif
  
  TWSR = 0;                        // No prescaler (prescaler = 1)
  if (fast) {
    TWBR = 3; // Set bit rate factor for 400 kHz
  } else {
    TWBR = (F_CPU / F_TWI_NORMAL - 16) / 2; // Set bit rate factor for 100 kHz
  }

  this->initialised = true; // Set initialised flag
}

/**
 * @brief Reads a single byte from the I2C bus.
 * 
 * Uses the hardware TWI controller to receive data from the slave device.
 * 
 * **Read Operation:**
 * 1. Decrement I2Ccount if it's not 0 or -1
 * 2. Set TWCR with TWINT and TWEN, and TWEA only if more bytes expected
 *    - TWEA (ACK enable): If I2Ccount != 0, send ACK; otherwise send NACK
 * 3. Wait for TWINT flag (interrupt/completion)
 * 4. Return received byte from TWDR
 * 
 * A timeout mechanism (max counter) is included to prevent infinite blocking
 * if the I2C bus has issues.
 * 
 * @return The 8-bit data byte received from the slave device
 * 
 * @see readLast()
 */
uint8_t TinyI2CMaster::read(void) {
  if (I2Ccount != 0)
    I2Ccount--;
  
  // Send read command with ACK if more bytes expected, NACK if last byte
  TWCR = 1 << TWINT | 1 << TWEN | ((I2Ccount == 0) ? 0 : (1 << TWEA));

  uint16_t max = 1500;
  while (!(TWCR & 1 << TWINT) && max--);  // Wait for completion

  if (max == 0) {
  #if defined(HAS_SERIAL) && defined(I2C_DEBUG)
    USART_WriteString("TinyI2CMaster::read error 1\n");
  #endif
  }

  return TWDR;  // Return received byte
}

/**
 * @brief Reads the final data byte and sends NACK.
 * 
 * Convenience function that sets I2Ccount to 0 before calling read(),
 * ensuring a NACK is sent after this byte to signal end of transmission.
 * 
 * @return The 8-bit data byte received from the slave
 * 
 * @see read()
 */
uint8_t TinyI2CMaster::readLast(void) {
  I2Ccount = 0;
  return TinyI2CMaster::read();
}

/**
 * @brief Writes a single data byte to the I2C bus.
 * 
 * Uses the hardware TWI controller to transmit data to the slave device.
 * 
 * **Write Operation:**
 * 1. Load byte into TWDR
 * 2. Set TWCR with TWINT and TWEN (start transmission)
 * 3. Wait for TWINT flag (transmission complete + ACK/NACK received)
 * 4. Check TWSR status code to verify ACK was received
 * 
 * Status codes checked:
 * - 0x28: Data acknowledged (successful write)
 * - Any other value: Write failed or NACK received
 * 
 * A timeout mechanism is included to prevent infinite blocking.
 * 
 * @param data The 8-bit value to transmit to the slave
 * 
 * @return true if data was acknowledged by the slave,
 *         false if NACK received or timeout occurred
 * 
 * @see TWSR_MTX_DATA_ACK
 */
bool TinyI2CMaster::write(uint8_t data) {
  TWDR = data;
  TWCR = 1 << TWINT | 1 << TWEN;  // Clear TWINT to start transmission
  
  uint16_t max = 15000;
  while (!(TWCR & 1 << TWINT) && max--);  // Wait for completion

  if (max == 0) {
  #if defined(HAS_SERIAL) && defined(I2C_DEBUG)
    USART_WriteString("TinyI2CMaster::write error 1\n");
  #endif
  }

  // Check if data was acknowledged
  return (TWSR & 0xF8) == TWSR_MTX_DATA_ACK;
}

/**
 * @brief Initiates I2C START condition and sends slave address.
 * 
 * Uses the hardware TWI controller to generate the START condition and
 * address the slave device.
 * 
 * **START Sequence:**
 * 1. Determine if operation is read or write based on readcount
 *    - readcount = 0: Write mode (I2C_WRITE)
 *    - readcount > 0: Read mode (I2C_READ), set I2Ccount for byte tracking
 * 2. Shift 7-bit address left and add R/W bit (LSB)
 * 3. Set TWCR to initiate START condition (TWSTA flag)
 * 4. Wait for START to be transmitted (TWSR = 0x08 or 0x10)
 * 5. Load address byte into TWDR
 * 6. Set TWCR to transmit address byte
 * 7. Wait for address transmission complete
 * 8. Verify correct status code:
 *    - 0x18: Master transmit mode, address ACK'd
 *    - 0x40: Master receive mode, address ACK'd
 * 9. Return true if address acknowledged, false otherwise
 * 
 * Multiple timeouts and status checks provide robust error detection.
 * 
 * @param address 7-bit I2C slave address (0x00-0x7F)
 * @param readcount 0 for write mode, > 0 for read mode.
 *                  If read mode, I2Ccount is initialized to readcount.
 * 
 * @return true if START condition generated and slave acknowledged,
 *         false if any step failed
 * 
 * @see restart()
 */
bool TinyI2CMaster::start(uint8_t address, uint8_t readcount) {
  bool read;
  if (readcount == 0)
    read = 0; // Write
  else {
    I2Ccount = readcount;
    read = 1;
  } // Read
  uint8_t addressRW = address << 1 | read;
  
  // Send START condition
  TWCR = 1 << TWINT | 1 << TWSTA | 1 << TWEN;
  
  uint16_t max = 1500;
  while (!(TWCR & 1 << TWINT) && max--);

  if (max == 0) {
    #if defined(HAS_SERIAL) && defined(I2C_DEBUG)
    USART_WriteString("TinyI2CMaster::start error 1\n");
    #endif
    return false;
  }

  // Verify START condition was transmitted
  if ((TWSR & 0xF8) != TWSR_START && (TWSR & 0xF8) != TWSR_REP_START) {
    #if defined(HAS_SERIAL) && defined(I2C_DEBUG)
    USART_WriteString("TinyI2CMaster::start error 2\n");
    #endif
    return false;
  }

  // Send device address and read/write bit
  TWDR = addressRW;
  TWCR = 1 << TWINT | 1 << TWEN;

  max = 1500;
  while (!(TWCR & 1 << TWINT) && max--);

  if (max == 0) {
    #if defined(HAS_SERIAL) && defined(I2C_DEBUG)
    USART_WriteString("TinyI2CMaster::start error 3\n");
    #endif
    return false;
  }

  // Verify address was acknowledged based on read/write mode
  if (addressRW & I2C_READ)
    return (TWSR & 0xF8) == TWSR_MRX_ADR_ACK;  // Check master receive mode
  else
    return (TWSR & 0xF8) == TWSR_MTX_ADR_ACK;  // Check master transmit mode
}

/**
 * @brief Generates a REPEATED START condition for direction change.
 * 
 * For the TWI implementation, this simply calls start() with the new address
 * and readcount. The hardware generates a repeated START automatically.
 * 
 * @param address 7-bit slave address for the new transaction
 * @param readcount 0 for write mode, > 0 for read mode
 * 
 * @return true if repeated START and address acknowledge successful,
 *         false if failed
 * 
 * @see start()
 */
bool TinyI2CMaster::restart(uint8_t address, uint8_t readcount) {
  return TinyI2CMaster::start(address, readcount);
}

/**
 * @brief Generates I2C STOP condition and releases the bus.
 * 
 * Uses the hardware TWI controller to generate the STOP condition.
 * 
 * **STOP Operation:**
 * 1. Set TWCR with TWINT, TWEN, and TWSTO flags
 *    - TWSTO: Initiate STOP condition
 * 2. Wait for TWSTO bit to clear (indicates STOP has been sent and bus released)
 * 3. Include timeout to prevent infinite blocking
 * 
 * After STOP, the I2C bus is idle and available for other devices or transactions.
 * 
 * @see start()
 * @see restart()
 */
void TinyI2CMaster::stop(void) {
  TWCR = 1 << TWINT | 1 << TWEN | 1 << TWSTO;

  uint16_t max = 1500;
  while ((TWCR & 1 << TWSTO) && max--); // wait until stop and bus released

  if (max == 0) {
    #if defined(HAS_SERIAL) && defined(I2C_DEBUG)
    USART_WriteString("TinyI2CMaster::stop error 1\n");
    #endif
  }
}

/// @} // End of ATmegaTWI group

#else
#error "Sorry TinyI2C doesn't support this processor"
#endif

/**
 * @defgroup GlobalInstance Global TinyI2C Instance
 * @brief Single global instance of the TinyI2CMaster class.
 * @{
 */

/** 
 * Global TinyI2C object - the primary interface for I2C communication.
 * 
 * This is a singleton instance that should be used for all I2C operations.
 * Initialize with TinyI2C.init() and use methods like start(), write(), read(), stop().
 * 
 * @example
 * TinyI2C.init();
 * if (TinyI2C.start(0x52, 0)) {
 *     TinyI2C.write(0xFA);
 *     TinyI2C.stop();
 * }
 */
TinyI2CMaster TinyI2C = TinyI2CMaster();

/// @}
