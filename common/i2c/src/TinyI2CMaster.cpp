/* TinyI2C v2.0.1

   David Johnson-Davies - www.technoblogy.com - 5th June 2022

   CC BY 4.0
   Licensed under a Creative Commons Attribution 4.0 International license:
   http://creativecommons.org/licenses/by/4.0/
*/

#include "TinyI2CMaster.h"

#include <avr/io.h>
#include <util/delay.h>

//#define HAS_SERIAL

#ifdef HAS_SERIAL
#include <usart_serial.h>
#endif

TinyI2CMaster::TinyI2CMaster() {}

#if defined(USIDR)

/* *********************************************************************************************************************

   Minimal Tiny I2C Routines for original ATtiny chips that support I2C using a
USI peripheral, such as ATtiny45.

*********************************************************************************************************************
*/

// Defines
#define TWI_FAST_MODE

#ifdef TWI_FAST_MODE // TWI FAST mode timing limits. SCL = 100-400kHz
#define DELAY_T2TWI (_delay_us(2)) // >1.3us
#define DELAY_T4TWI (_delay_us(1)) // >0.6us
#else // TWI STANDARD mode timing limits. SCL <= 100kHz
#define DELAY_T2TWI (_delay_us(5)) // >4.7us
#define DELAY_T4TWI (_delay_us(4)) // >4.0us
#endif

#define TWI_NACK_BIT 0 // Bit position for (N)ACK bit.

#if defined (__AVR_ATtiny45__)
#define PIN_USI_SCL PINB2
#define PIN_USI_SDA PINB0
#define PIN_USI_CL PINB
#define DDR_USI DDRB
#define DDR_USI_CL DDRB
#define PORT_USI PORTB
#define PORT_USI_CL PORTB
#endif

// Constants
// Prepare register value to: Clear flags, and set USI to shift 8 bits i.e.
// count 16 clock edges.
unsigned char const USISR_8bit =
    1 << USISIF | 1 << USIOIF | 1 << USIPF | 1 << USIDC | 0x0 << USICNT0;
// Prepare register value to: Clear flags, and set USI to shift 1 bit i.e. count
// 2 clock edges.
unsigned char const USISR_1bit =
    1 << USISIF | 1 << USIOIF | 1 << USIPF | 1 << USIDC | 0xE << USICNT0;

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
      ; // Wait for SCL to go high.
    DELAY_T4TWI;
    USICR = data;                   // Generate negative SCL edge.
  } while (!(USISR & 1 << USIOIF)); // Check for transfer complete.

  DELAY_T2TWI;
  data = USIDR;                  // Read out data.
  USIDR = 0xFF;                  // Release SDA.
  DDR_USI |= (1 << PIN_USI_SDA); // Enable SDA as output.

  return data; // Return the data from the USIDR
}

void TinyI2CMaster::init(bool fast) {
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
}

uint8_t TinyI2CMaster::read(void) {
  if ((I2Ccount != 0) && (I2Ccount != -1))
    I2Ccount--;

  /* Read a byte */
  DDR_USI &= ~(1 << PIN_USI_SDA); // Enable SDA as input.
  uint8_t data = TinyI2CMaster::transfer(USISR_8bit);

  /* Prepare to generate ACK (or NACK in case of End Of Transmission) */
  if (I2Ccount == 0)
    USIDR = 0xFF;
  else
    USIDR = 0x00;
  TinyI2CMaster::transfer(USISR_1bit); // Generate ACK/NACK.

  return data; // Read successfully completed
}

uint8_t TinyI2CMaster::readLast(void) {
  I2Ccount = 0;
  return TinyI2CMaster::read();
}

bool TinyI2CMaster::write(uint8_t data) {
  /* Write a byte */
  PORT_USI_CL &= ~(1 << PIN_USI_SCL);  // Pull SCL LOW.
  USIDR = data;                        // Setup data.
  TinyI2CMaster::transfer(USISR_8bit); // Send 8 bits on bus.

  /* Clock and verify (N)ACK from slave */
  DDR_USI &= ~(1 << PIN_USI_SDA); // Enable SDA as input.
  if (TinyI2CMaster::transfer(USISR_1bit) & 1 << TWI_NACK_BIT)
    return false;

  return true; // Write successfully completed
}

// Start transmission by sending address
bool TinyI2CMaster::start(uint8_t address, uint8_t readcount) {
  if (readcount != 0) {
    I2Ccount = readcount;
    readcount = 1;
  }
  uint8_t addressRW = address << 1 | readcount;

  /* Release SCL to ensure that (repeated) Start can be performed */
  PORT_USI_CL |= 1 << PIN_USI_SCL; // Release SCL.
  while (!(PIN_USI_CL & 1 << PIN_USI_SCL))
    ; // Verify that SCL becomes high.
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
    return false;

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

bool TinyI2CMaster::restart(uint8_t address, uint8_t readcount) {
  return TinyI2CMaster::start(address, readcount);
}

void TinyI2CMaster::stop(void) {
  PORT_USI &= ~(1 << PIN_USI_SDA); // Pull SDA low.
  PORT_USI_CL |= 1 << PIN_USI_SCL; // Release SCL.
  while (!(PIN_USI_CL & 1 << PIN_USI_SCL))
    ; // Wait for SCL to go high.
  DELAY_T4TWI;
  PORT_USI |= 1 << PIN_USI_SDA; // Release SDA.
  DELAY_T2TWI;
}

#elif defined(TWDR)

/* *********************************************************************************************************************

   Minimal Tiny I2C Routines for ATmega processors with TWI support, such as
the ATmega328P used in the Arduino Uno, and the ATmega8535, ATmega1284P.

*********************************************************************************************************************
*/

// 400kHz clock
uint32_t const F_TWI_NORMAL = 40000L; // Hardware I2C clock in Hz

// Choose for 1MHz clock
uint32_t const F_TWI_FAST = 100000L;                                // Hardware
// I2C clock in Hz

const uint8_t TWSR_MTX_DATA_ACK = 0x28;
const uint8_t TWSR_MTX_ADR_ACK = 0x18;
const uint8_t TWSR_MRX_ADR_ACK = 0x40;
const uint8_t TWSR_START = 0x08;
const uint8_t TWSR_REP_START = 0x10;
const uint8_t I2C_READ = 1;
const uint8_t I2C_WRITE = 0;

void TinyI2CMaster::init(bool fast) {
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
  // ATMega8535 => SCL = PC0 SDA = PC1
  DDRC &= ~((1 << DDC0)|(1 << DDC1));
  PORTC |= (1 << PORTC0) | (1 << PORTC1);
#else
  #error "No yet configured, check datasheet"
#endif
  
  TWSR = 0;                        // No prescaler
  if (fast) {
    TWBR = (F_CPU / F_TWI_FAST - 16) / 2; // Set bit rate factor
  } else {
    TWBR = (F_CPU / F_TWI_NORMAL - 16) / 2; // Set bit rate factor
  }
}

uint8_t TinyI2CMaster::read(void) {
  if (I2Ccount != 0)
    I2Ccount--;
  TWCR = 1 << TWINT | 1 << TWEN | ((I2Ccount == 0) ? 0 : (1 << TWEA));

  uint16_t max = 1500;
  while (!(TWCR & 1 << TWINT) && max--);

  if (max == 0) {
  #ifdef HAS_SERIAL
    USART_WriteString("TinyI2CMaster::read error 1\n");
  #endif
  }

  return TWDR;
}

uint8_t TinyI2CMaster::readLast(void) {
  I2Ccount = 0;
  return TinyI2CMaster::read();
}

bool TinyI2CMaster::write(uint8_t data) {
  TWDR = data;
  TWCR = 1 << TWINT | 1 << TWEN;
  uint16_t max = 15000;
  while (!(TWCR & 1 << TWINT) && max--);

  if (max == 0) {
  #ifdef HAS_SERIAL
    USART_WriteString("TinyI2CMaster::write error 1\n");
  #endif
  }

  return (TWSR & 0xF8) == TWSR_MTX_DATA_ACK;
}

// Start transmission by sending address
bool TinyI2CMaster::start(uint8_t address, uint8_t readcount) {
  bool read;
  if (readcount == 0)
    read = 0; // Write
  else {
    I2Ccount = readcount;
    read = 1;
  } // Read
  uint8_t addressRW = address << 1 | read;
  TWCR = 1 << TWINT | 1 << TWSTA | 1 << TWEN; // Send START condition
  
  uint16_t max = 1500;

  while (!(TWCR & 1 << TWINT) && max--);

  if (max == 0) {
    #ifdef HAS_SERIAL
    USART_WriteString("TinyI2CMaster::start error 1\n");
    #endif
    return false;
  }

  if ((TWSR & 0xF8) != TWSR_START && (TWSR & 0xF8) != TWSR_REP_START) {
    #ifdef HAS_SERIAL
    USART_WriteString("TinyI2CMaster::start error 2\n");
    #endif
    return false;
  }

  TWDR = addressRW; // Send device address and direction
  TWCR = 1 << TWINT | 1 << TWEN;

  max = 1500;
  while (!(TWCR & 1 << TWINT) && max--);

  if (max == 0) {
    #ifdef HAS_SERIAL
    USART_WriteString("TinyI2CMaster::start error 3\n");
    #endif
    return false;
  }

  if (addressRW & I2C_READ)
    return (TWSR & 0xF8) == TWSR_MRX_ADR_ACK;
  else
    return (TWSR & 0xF8) == TWSR_MTX_ADR_ACK;
}

bool TinyI2CMaster::restart(uint8_t address, uint8_t readcount) {
  return TinyI2CMaster::start(address, readcount);
}

void TinyI2CMaster::stop(void) {
  TWCR = 1 << TWINT | 1 << TWEN | 1 << TWSTO;

  uint16_t max = 1500;
  while ((TWCR & 1 << TWSTO) && max--); // wait until stop and bus released

  if (max == 0) {
    #ifdef HAS_SERIAL
    USART_WriteString("TinyI2CMaster::stop error 1\n");
    #endif
  }
}

#else
#error "Sorry TinyI2C doesn't support this processor"
#endif

// All versions

TinyI2CMaster TinyI2C = TinyI2CMaster(); // Instantiate a TinyI2C object
