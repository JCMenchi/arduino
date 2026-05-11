/**
 * @file TinyI2CMaster.h
 * @brief I2C master library for ATtiny microcontrollers.
 * 
 * Provides I2C (Two-Wire Interface) master functionality for ATtiny devices,
 * enabling communication with I2C slave devices. Supports both standard
 * (100 kHz) and fast (400 kHz) I2C modes.
 * 
 * **Features:**
 * - Master-only I2C implementation
 * - Standard and fast mode support
 * - Start, repeated start, and stop condition generation
 * - Read and write operations
 * - Automatic SCL clock stretching handling
 * 
 * **Usage Example:**
 * @code
 * TinyI2C.init(false);  // Initialize in standard mode
 * if (TinyI2C.start(0x52, 0)) {  // Start with slave at 0x52
 *     TinyI2C.write(0xFA);  // Write command
 *     TinyI2C.stop();
 * }
 * @endcode
 * 
 * **I2C Pin Configuration:**
 * - SCL (Serial Clock): typically PA4 or PA6 on ATtiny84
 * - SDA (Serial Data): typically PA5 or PA7 on ATtiny84
 * - Both lines use open-drain outputs (require pull-up resistors)
 * 
 * @note The actual pin configuration depends on the specific ATtiny model
 *       and must match the hardware setup.
 * 
 * @see http://ww1.microchip.com/downloads/en/DeviceDoc/Atmel-2586-AVR-8-bit-Microcontroller-ATtiny25-ATtiny45-ATtiny85_Datasheet.pdf
 */
#ifndef TinyI2CMaster_h
#define TinyI2CMaster_h

#include <stdint.h>

/**
 * @class TinyI2CMaster
 * @brief I2C master controller for ATtiny microcontrollers.
 * 
 * Manages I2C communication as the master device. Handles bus initialization,
 * slave addressing, data transfer, and bus protocol compliance. This is a
 * singleton pattern implementation with a single global instance.
 * 
 * The class operates in master mode only and follows the I2C protocol for:
 * - Generating START and STOP conditions
 * - Addressing slave devices
 * - Reading and writing data bytes
 * - Clock stretching detection
 * 
 * **Typical Communication Sequence:**
 * 1. Call init() to configure the I2C bus
 * 2. Call start() with slave address and read count
 * 3. Perform read() or write() operations as needed
 * 4. Call stop() to terminate the transaction
 * 5. Optionally use restart() to change direction without stop
 * 
 * @note All operations are blocking and do not use interrupts.
 * @note This is a master-only implementation; slave functionality is not supported.
 */
class TinyI2CMaster {

public:
  /**
   * @brief Constructor for TinyI2CMaster.
   * 
   * Initializes member variables to default values. The I2C bus must be
   * configured with init() before communication can begin.
   * 
   * @note This does not initialize the I2C hardware; use init() after construction.
   */
  TinyI2CMaster();

  /**
   * @brief Initializes the I2C master bus.
   * 
   * Configures the microcontroller's I2C interface for master mode operation.
   * Sets the I2C bit rate based on the fast parameter:
   * - Standard mode: ~100 kHz
   * - Fast mode: ~400 kHz
   * 
   * This function must be called once before any I2C communication.
   * It configures the I2C control registers and sets the bus timing parameters.
   * 
   * @param fast If true, configures fast mode (400 kHz); if false, standard mode (100 kHz).
   *             Default is false (standard mode).
   * 
   * @note The actual frequency depends on the ATtiny clock frequency.
   * @note SCL and SDA lines must have external pull-up resistors (typically 4.7 kΩ).
   * 
   * @see start()
   */
  void init(bool fast = false);

  /**
   * @brief Reads a data byte from the I2C bus.
   * 
   * Reads the next byte from the slave device and sends an ACK (acknowledgment)
   * signal. Use this when you expect more data after this byte, or when the
   * start() function was called with a non-zero readcount.
   * 
   * The byte is received MSB first (most significant bit first).
   * The function automatically handles:
   * - Clock stretching (waits for slave to release SCL)
   * - Bit sampling at the correct time
   * - ACK signal generation
   * 
   * @return The 8-bit data received from the slave device.
   * 
   * @see readLast()
   * @see start()
   */
  uint8_t read(void);

  /**
   * @brief Reads the final data byte and sends NACK (Not Acknowledged).
   * 
   * Reads the last byte from the slave device and sends a NACK signal,
   * indicating to the slave that no more data is expected. This should be
   * called for the last byte in a read sequence.
   * 
   * After calling this function, the bus is ready for a stop() or restart()
   * condition. The byte is received MSB first.
   * 
   * The function automatically handles:
   * - Clock stretching
   * - Bit sampling
   * - NACK signal generation (informing slave this is the last read)
   * 
   * @return The 8-bit data received from the slave device.
   * 
   * @see read()
   * @see stop()
   * @see restart()
   */
  uint8_t readLast(void);

  /**
   * @brief Writes a data byte to the I2C bus.
   * 
   * Sends an 8-bit data byte to the currently addressed slave device.
   * The byte is transmitted MSB first (most significant bit first).
   * 
   * After transmission, the function waits for:
   * - ACK from the slave (slave pulls SDA low after the 8th clock bit)
   * - Or timeout if no ACK is received (slave doesn't acknowledge)
   * 
   * **Transmission steps:**
   * 1. Sends 8 data bits (MSB first)
   * 2. Releases SDA to allow slave to send ACK/NACK
   * 3. Detects ACK/NACK status
   * 4. Returns result
   * 
   * @param data The 8-bit value to send to the slave device.
   * 
   * @return true if the slave acknowledged (pulled SDA low) after transmission,
   *         false if no acknowledge was received (transmission failed or slave not responding).
   * 
   * @see start()
   * @see read()
   */
  bool write(uint8_t data);

  /**
   * @brief Generates I2C START condition and selects a slave device.
   * 
   * Initiates an I2C transaction by:
   * 1. Generating a START condition (SDA goes low while SCL is high)
   * 2. Sending a 7-bit slave address followed by a read (R/W) bit
   * 3. Waiting for slave acknowledgment
   * 
   * The read/write bit is determined by the readcount parameter:
   * - readcount = 0: Send mode (write 0 to R/W bit)
   * - readcount > 0: Receive mode (write 1 to R/W bit), indicating how many bytes to read
   * 
   * **Addressing:**
   * - Address parameter should be the 7-bit slave address (0x00-0x7F)
   * - The function automatically adds the R/W bit
   * - Example: address 0x52 becomes 0xA4 (write) or 0xA5 (read)
   * 
   * @param address 7-bit I2C slave address (0x00-0x7F).
   * @param readcount Number of bytes to read (0 = write mode, >0 = read mode).
   *                  This parameter is informational for the master; the value
   *                  passed here doesn't directly affect transfers.
   * 
   * @return true if the slave acknowledged the address,
   *         false if no acknowledge received (slave not present or not responding).
   * 
   * @see restart()
   * @see write()
   * @see read()
   * @see stop()
   */
  bool start(uint8_t address, uint8_t readcount);

  /**
   * @brief Generates a REPEATED START condition.
   * 
   * Transitions from one I2C transaction to another without generating a STOP
   * condition. This is useful for:
   * - Changing I2C bus direction (write to read or vice versa)
   * - Multiple transactions with the same or different slave devices
   * - Keeping the bus under master control
   * 
   * **Repeated START sequence:**
   * 1. SCL goes low
   * 2. SDA goes high
   * 3. SCL goes high
   * 4. SDA goes low (START condition)
   * 5. Send new slave address and R/W bit
   * 6. Wait for acknowledgment
   * 
   * This is more efficient than stop() followed by start() as it doesn't
   * release the bus between transactions.
   * 
   * @param address 7-bit I2C slave address (0x00-0x7F) for the new transaction.
   * @param readcount Number of bytes to read (0 = write mode, >0 = read mode).
   *                  Similar to start(), this is primarily informational.
   * 
   * @return true if the slave acknowledged the address after the repeated START,
   *         false if no acknowledge received.
   * 
   * @see start()
   * @see stop()
   * 
   * @example
   * // Write to address 0x52, then read 2 bytes from same device
   * TinyI2C.start(0x52, 0);
   * TinyI2C.write(0xFA);  // Send command
   * TinyI2C.restart(0x52, 2);  // Switch to read mode
   * uint8_t byte1 = TinyI2C.read();
   * uint8_t byte2 = TinyI2C.readLast();
   * TinyI2C.stop();
   */
  bool restart(uint8_t address, uint8_t readcount);

  /**
   * @brief Generates I2C STOP condition and releases the bus.
   * 
   * Terminates an I2C transaction and releases control of the I2C bus.
   * After calling stop(), the bus is idle and can be used by other devices
   * (if the system is multi-master) or for a new transaction.
   * 
   * **STOP condition sequence:**
   * 1. SCL goes low
   * 2. SDA goes low
   * 3. SCL goes high
   * 4. SDA goes high (bus idle)
   * 
   * This function should be called at the end of every I2C transaction.
   * 
   * @note After stop() is called, you must call start() to begin a new transaction.
   * @note If you need to switch bus direction without releasing the bus,
   *       use restart() instead of stop() + start().
   * 
   * @see start()
   * @see restart()
   * 
   * @example
   * TinyI2C.start(0x52, 0);
   * TinyI2C.write(0x42);
   * TinyI2C.stop();  // Transaction complete, bus released
   */
  void stop(void);

private:
  /** Number of bytes to read in current transaction */
  uint8_t I2Ccount;

  /** Flag indicating whether I2C has been initialized (non-zero = initialized) */
  uint8_t initialised;

  /**
   * @brief Transfers a byte on the I2C bus with full clock cycle.
   * 
   * Low-level I2C bit-banging function that handles the complete byte transfer
   * including clock cycle management. Implements the I2C protocol for:
   * - Driving SDA and SCL lines
   * - Detecting slave responses
   * - Clock stretching (waiting if slave holds SCL low)
   * 
   * @param data The byte to transfer. MSB is sent/received first.
   * 
   * @return The byte received from the slave on the bus.
   *         If sending, this contains the ACK/NACK bit position data.
   * 
   * @see write()
   * @see read()
   */
  uint8_t transfer(uint8_t data);
};

extern TinyI2CMaster TinyI2C;

#endif
