/**
 * @file int0_serial.h
 * @brief INT0 interrupt-based bit-bang serial communication driver
 * 
 * Implements software-based serial communication using INT0 interrupt for receive
 * and bit-banging for transmit. Useful when hardware USART is unavailable or needed
 * elsewhere on the microcontroller.
 * 
 * @note Fixed baud rate of 9600 bps
 * @note Requires proper timer configuration for bit timing
 * @note Serial command management class provides buffering and command parsing
 */

#ifndef _AVRTOOLS_INT0_SERIAL_H
#define _AVRTOOLS_INT0_SERIAL_H

#include <stddef.h>
#include <stdint.h>

/** @brief Configure transmit port (default: B). Override before include if needed */
#ifndef INT0_SERIAL_TRANSMIT_PORT
#define INT0_SERIAL_TRANSMIT_PORT B
#endif

/**
 * @brief Initialize INT0 serial communication
 * 
 * Sets up INT0 interrupt handler for receiving serial data via bit-banging at 9600 baud.
 * The transmit pin (tpin) is configured as output on the configured transmit port.
 * 
 * @param tpin GPIO bit number for transmit pin (0-7) on INT0_SERIAL_TRANSMIT_PORT
 * @param INT0_rec_cb Callback function pointer called when a byte is received
 *                    Function signature: void callback(uint8_t data)
 * 
 * @example
 * void serialRxCallback(uint8_t data) {
 *     // Handle received byte
 * }
 * INT0_Init(PA7, serialRxCallback);
 */
void INT0_Init(uint8_t tpin, volatile void (*INT0_rec_cb)(uint8_t));

/**
 * @brief Transmit a single byte via INT0 serial
 * @param data Byte to transmit
 * @return 1 if successful, 0 if error
 */
uint8_t INT0_Transmit(uint8_t data);

/**
 * @brief Write a null-terminated string to serial
 * @param str Pointer to string in RAM
 */
void INT0_WriteString(const char *str);

/**
 * @brief Write a null-terminated string stored in program memory to serial
 * @param str Pointer to string in PROGMEM (program memory)
 */
void INT0_WritePString(const char *str);

/**
 * @brief Write a signed 32-bit integer to serial
 * @param i Integer value to write
 * @param base Number base for conversion (default: 10 for decimal)
 */
void INT0_WriteInt(int32_t i, uint8_t base = 10);

/**
 * @brief Write an unsigned 32-bit integer to serial
 * @param i Unsigned integer value to write
 * @param base Number base for conversion (default: 10 for decimal)
 */
void INT0_WriteUInt(uint32_t i, uint8_t base = 10);

/**
 * @brief Write a single character to serial
 * @param d Character to write
 */
void INT0_WriteChar(char d);

/**
 * @brief Write a floating-point number to serial
 * @param d Float value to write
 * @param width Field width for output (default: 11)
 * @param prec Decimal precision (default: 2 decimal places)
 */
void INT0_WriteFloat(float d, uint8_t width = 11, uint8_t prec = 2);

/** @brief Configure serial command buffer size (default: 32 bytes). Override before include if needed */
#ifndef INT0_SERIAL_CMD_BUF_SIZE
#define INT0_SERIAL_CMD_BUF_SIZE 32
#endif

/**
 * @class INT0_SerialCommandMgr
 * @brief Static command buffering and parsing for INT0 serial
 * 
 * Accumulates received bytes into a command buffer until a newline (\n) is received.
 * Handles carriage returns (\r) by ignoring them. Provides functions to check for
 * and retrieve complete commands.
 * 
 * @note All methods are static; no instantiation needed
 * @note Buffer wraps around if it exceeds INT0_SERIAL_CMD_BUF_SIZE
 * @note Use with INT0_Init callback: INT0_Init(tpin, INT0_SerialCommandMgr::serialInput)
 */
class INT0_SerialCommandMgr {
   private:
    /** @brief Buffer holding accumulated command bytes */
    static char commandBuffer[INT0_SERIAL_CMD_BUF_SIZE];
    /** @brief Current position in command buffer */
    static uint8_t commandBufferPos;
    /** @brief Flag indicating a complete command is available */
    static uint8_t _hasCommand;

   public:
    /**
     * @brief Check if a complete command is available
     * @return 1 if command ready, 0 otherwise
     */
    static uint8_t hasCommand() { return _hasCommand; }

    /**
     * @brief Retrieve and consume the current command
     * 
     * Returns pointer to command buffer if a command is available,
     * otherwise returns NULL. Resets the command buffer state.
     * 
     * @return Pointer to null-terminated command string, or NULL if no command ready
     */
    static const char *command() {
        const char *ret = (_hasCommand == 1) ? commandBuffer : NULL;
        commandBufferPos = 0;
        _hasCommand = 0;

        return ret;
    }

    /**
     * @brief Peek at the current command buffer without consuming it
     * 
     * Allows reading the command buffer without resetting the command state.
     * Useful for non-destructive inspection of the buffer.
     * 
     * @return Pointer to command buffer (may not be null-terminated if incomplete)
     */
    static const char *peekCommand() {
        return commandBuffer;
    }

    /**
     * @brief Static callback for INT0 interrupt handler
     * 
     * Processes incoming bytes: accumulates them in the buffer until a newline
     * is received, which signals a complete command. Carriage returns are ignored.
     * 
     * @param data Received byte
     */
    static volatile void serialInput(uint8_t data) {
        if (data == '\r') {
            // ignore
        } else if (data == '\n') {
            commandBuffer[commandBufferPos] = '\0';
            _hasCommand = 1;
        } else {
            commandBuffer[commandBufferPos++] = data;
            commandBufferPos %= INT0_SERIAL_CMD_BUF_SIZE;
        }
    }
};

#endif
