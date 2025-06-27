# SPI driver for AVR CPU with USI (Universal Serial Interface)

## Key Concepts

- **3-Wire Mode**: The USI is configured for "3-wire mode" to function as SPI. The USI pins (DI, DO, USCK) directly correspond to SPI's
  MISO, MOSI, and SCK.
- **Manual Clocking**: As an SPI master, you are responsible for generating the clock signal. You do this by manually toggling the USITC bit in
  the USI Control Register (USICR) to create the clock pulses.
- **Data Register**: You send and receive data through the 8-bit USI Data Register (USIDR). When you write a byte to USIDR, the USI
  hardware shifts it out on the DO pin while simultaneously shifting in a byte from the DI pin at each clock pulse.
- **Counter Overflow**: The USI has a 4-bit counter. To transfer a full byte (8 bits), you need to pulse the clock 8 times. The USI counter
    overflow flag (USIOIF) in the USI Status Register (USISR) is set automatically after 8 clock pulses, signaling that the byte transfer is complete.
- **Manual Chip Select (CS)**: The USI hardware does not control the Chip Select pin. You must manage this manually using a standard GPIO pin.

## Implementation Steps

Here is a C code example for an ATtiny85. The principles are the same for other ATtiny models with a USI module, but you should
always consult the datasheet for your specific chip to confirm pin and register names.

This example sets up the ATtiny as an SPI master and provides a function to transfer a single byte.

Assumed Pinout on ATtiny85:

- PB0: MISO (DI)
- PB1: MOSI (DO)
- PB2: SCK (USCK)
- PB3: Chip Select (CS) - This is an arbitrary choice for the manual CS pin.

### C Code

```C
    1 #include <avr/io.h>
    2 
    3 // Define the SPI pins for clarity
    4 #define SPI_DDR  DDRB
    5 #define SPI_PORT PORTB
    6 #define SPI_MISO PB0
    7 #define SPI_MOSI PB1
    8 #define SPI_SCK  PB2
    9 #define SPI_CS   PB3 // Manual Chip Select
   10 
   11 /**
   12  * @brief Initializes the USI module for SPI Master mode.
   13  */
   14 void spi_master_init(void) {
   15     // 1. Set MOSI, SCK, and CS pins as outputs. MISO is automatically an input.
   16     SPI_DDR |= (1 << SPI_MOSI) | (1 << SPI_SCK) | (1 << SPI_CS);
   17 
   18     // 2. Set CS pin high (inactive)
   19     SPI_PORT |= (1 << SPI_CS);
   20 
   21     // 3. Configure the USI for 3-wire SPI master mode.
   22     //    - USIWM0: Sets to 3-wire mode (SPI).
   23     //    - USICS1: Selects software clock strobe (manual clocking).
   24     USICR = (1 << USIWM0) | (1 << USICS1);
   25 }
   26 
   27 /**
   28  * @brief Transfers one byte of data over SPI.
   29  *
   30  * @param data The byte to send.
   31  * @return The byte received from the slave.
   32  */
   33 uint8_t spi_master_transfer(uint8_t data) {
   34     // 1. Load the data to be sent into the USI Data Register.
   35     USIDR = data;
   36 
   37     // 2. Clear the USI Counter Overflow Flag to start the transfer.
   38     //    This also pre-loads the USI counter.
   39     USISR = (1 << USIOIF);
   40 
   41     // 3. Manually clock the USI until the transfer is complete.
   42     //    The USIOIF flag will be set by hardware after 8 clock cycles.
   43     while ((USISR & (1 << USIOIF)) == 0) {
   44         // Strobe the clock pin (toggle USICLK bit).
   45         // This generates one SCK pulse and shifts one bit.
   46         USICR |= (1 << USITC);
   47     }
   48 
   49     // 4. The received data is now in the USI Data Register.
   50     return USIDR;
   51 }
   52 
   53 int main(void) {
   54     spi_master_init();
   55 
   56     // --- Example Usage ---
   57     // Data to send to the slave device
   58     uint8_t data_to_send = 0xA5;
   59     uint8_t received_data;
   60 
   61     // 1. Pull Chip Select LOW to activate the slave
   62     SPI_PORT &= ~(1 << SPI_CS);
   63 
   64     // 2. Send the data and store the received byte
   65     received_data = spi_master_transfer(data_to_send);
   66 
   67     // 3. Pull Chip Select HIGH to deactivate the slave
   68     SPI_PORT |= (1 << SPI_CS);
   69 
   70     // Now `received_data` holds the value returned from the slave.
   71     // You can add a delay or further logic here.
   72 
   73     while (1) {
   74         // Your main application loop
   75     }
   76 
   77     return 0;
   78 }
```

### How the Code Works

  1. `spi_master_init()`:
      - Sets the direction of the MOSI, SCK, and our chosen CS pin to OUTPUT. The MISO pin is left as an input by default, which is
        correct.
      - Drives the CS pin HIGH to ensure no slave devices are active initially.
      - Configures the USICR register to enable 3-wire mode and set the clock source to "Software Strobe," meaning we have to pulse it
        manually.

  2. `spi_master_transfer(uint8_t data)`:
      - It places the byte you want to send into USIDR.
      - It clears the overflow flag USIOIF. This is the signal to the hardware to begin.
      - The while loop waits for the USIOIF flag to be set again. Inside the loop, we toggle the USITC bit. Each toggle generates a
        clock pulse, shifts one bit out of USIDR (on MOSI), and one bit into USIDR (from MISO).
      - After 8 toggles, the hardware automatically sets USIOIF, the loop terminates, and the full byte that was shifted in from the
        slave is now available in USIDR to be returned.

## Important Considerations

- SPI Modes (CPOL/CPHA): This example implements SPI Mode 0 (CPOL=0, CPHA=0), which is the most common. The clock is idle-low, and
  data is sampled on the rising edge. To implement other modes, you would need to change the clock polarity and when you toggle the
  USITC bit relative to the data being present.
- Datasheet is Your Best Friend: Always have the datasheet for your specific ATtiny MCU open. It will have a dedicated chapter on the
  USI module with register descriptions and timing diagrams that are invaluable.
- Optimization: For higher speeds, you can unroll the while loop in the transfer function and write the USICR toggles directly 8 times
  to avoid loop overhead. However, the while loop is more readable and works perfectly for most applications.
