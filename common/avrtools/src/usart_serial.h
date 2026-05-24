/**
 * @file usart_serial.h
 * @brief USART (Universal Synchronous/Asynchronous Receiver-Transmitter) serial communication
 * 
 * Provides hardware USART functionality for serial communication at configurable baud rates.
 * Includes interrupt-based receive with callback support and output formatting functions.
 * 
 * Features:
 * - Multiple baud rate options (9600, 57600, 115200 bps)
 * - Interrupt-driven receive with user callback
 * - String, integer, and floating-point output functions
 * - Built-in command buffering and parsing
 * 
 * @note Requires proper timer configuration for baud rate generation
 */

#ifndef _AVRTOOLS_USART_SERIAL_H
#define _AVRTOOLS_USART_SERIAL_H

#include <stddef.h>
#include <stdint.h>

/** @brief Baud rate constant for 9600 bps */
#define BAUD_RATE_9600 1
/** @brief Baud rate constant for 57600 bps */
#define BAUD_RATE_57600 2
/** @brief Baud rate constant for 115200 bps */
#define BAUD_RATE_115200 3

/**
 * @brief Initialize USART for serial communication
 * 
 * Configures the hardware USART with specified baud rate and sets up
 * interrupt-driven receive. The callback function is invoked for each
 * received byte along with an error flag.
 * 
 * @param baudrate Baud rate selection (BAUD_RATE_9600, BAUD_RATE_57600, or BAUD_RATE_115200)
 * @param usart_rec_cb Callback function pointer for received bytes
 *                     Function signature: void callback(uint8_t data, bool error)
 *                     error = true if framing or overrun error occurred
 * 
 * @example
 * void serialRxCallback(uint8_t data, bool error) {
 *     if (!error) {
 *         // Process received data
 *     }
 * }
 * USART_Init(BAUD_RATE_115200, serialRxCallback);
 */
void USART_Init(uint8_t baudrate, volatile void (*usart_rec_cb)(uint8_t, bool));

/**
 * @brief Transmit a single byte via USART
 * @param data Byte to transmit
 */
void USART_Transmit(uint8_t data);

/**
 * @brief Receive a single byte from USART (blocking)
 * 
 * Waits until a byte is available in the receive buffer.
 * 
 * @return Received byte
 */
uint8_t USART_Receive();

/**
 * @brief Write a null-terminated string to USART
 * @param str Pointer to string in RAM
 */
void USART_WriteString(const char *str);

/**
 * @brief Write a null-terminated string stored in program memory to USART
 * @param str Pointer to string in PROGMEM (program memory)
 */
void USART_WritePString(const char* str);

/**
 * @brief Write a signed 32-bit integer to USART
 * @param i Integer value to write
 * @param base Number base for conversion (default: 10 for decimal)
 */
void USART_WriteInt(int32_t i, uint8_t base = 10);

/**
 * @brief Write an unsigned 32-bit integer to USART
 * @param i Unsigned integer value to write
 * @param base Number base for conversion (default: 10 for decimal)
 */
void USART_WriteUInt(uint32_t i, uint8_t base = 10);

/**
 * @brief Write a single character to USART
 * @param d Character to write
 */
void USART_WriteChar(char d);

/**
 * @brief Write a boolean value to USART
 * @param b Boolean value (outputs "on" or "off")
 */
void USART_WriteBool(bool b);

/**
 * @brief Write a floating-point number to USART
 * @param d Float value to write
 * @param width Field width for output (default: 11)
 * @param prec Decimal precision (default: 2 decimal places)
 */
void USART_WriteFloat(float d, uint8_t width = 11, uint8_t prec = 2);

/**
 * @brief Get the most recently received character
 * 
 * Returns the last character received via USART interrupt.
 * Useful for non-callback-based character access.
 * 
 * @return Most recent received byte
 */
uint8_t USART_GetLastChar();

/** @brief Configure USART serial command buffer size (default: 32 bytes). Override before include if needed */
#ifndef SERIAL_CMD_BUF_SIZE
#define SERIAL_CMD_BUF_SIZE 32
#endif

/**
 * @class SerialCommandMgr
 * @brief Static command buffering and parsing for USART serial
 * 
 * Accumulates received bytes into a command buffer until a newline (\n) is received.
 * Handles carriage returns (\r) by ignoring them. Respects error flags and only
 * processes data from error-free receptions. Provides functions to check for and
 * retrieve complete commands.
 * 
 * @note All methods are static; no instantiation needed
 * @note Buffer wraps around if it exceeds SERIAL_CMD_BUF_SIZE
 * @note Use with USART_Init callback: USART_Init(baudrate, SerialCommandMgr::serialInput)
 */
class SerialCommandMgr {
private:
  /** @brief Buffer holding accumulated command bytes */
  static char commandBuffer[SERIAL_CMD_BUF_SIZE];
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
    const char* ret = (_hasCommand == 1) ? commandBuffer : NULL;
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
   * @brief Static callback for USART receive interrupt handler
   * 
   * Processes incoming bytes: accumulates them in the buffer until a newline
   * is received, which signals a complete command. Carriage returns are ignored.
   * Only processes error-free data.
   * 
   * @param data Received byte
   * @param error Error flag (true if framing or overrun error occurred)
   */
  static volatile void serialInput(uint8_t data, bool error) {
    if (!error) {
      if (data == '\r') {
        // ignore
      } else if (data == '\n') {
        SerialCommandMgr::commandBuffer[SerialCommandMgr::commandBufferPos] = '\0';
        SerialCommandMgr::_hasCommand = 1;
      } else {
        SerialCommandMgr::commandBuffer[SerialCommandMgr::commandBufferPos++] = data;
        SerialCommandMgr::commandBufferPos %= SERIAL_CMD_BUF_SIZE;
      }
    }
  }
};

#endif
