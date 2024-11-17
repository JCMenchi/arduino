#ifndef _AVRTOOLS_USART_SERIAL_H
#define _AVRTOOLS_USART_SERIAL_H

#include <stddef.h>
#include <stdint.h>

#define BAUD_RATE_9600 1
#define BAUD_RATE_57600 2
#define BAUD_RATE_115200 3

void USART_Init(uint8_t baudrate, volatile void (*usart_rec_cb)(uint8_t, bool));

void USART_Transmit(uint8_t data);
uint8_t USART_Receive();

void USART_WriteString(const char *str);
void USART_WritePString(const char* str);
void USART_WriteInt(int32_t i, uint8_t base = 10);
void USART_WriteUInt(uint32_t i, uint8_t base = 10);
void USART_WriteChar(char d);

void USART_WriteFloat(float d, uint8_t width = 11, uint8_t prec = 2);

uint8_t USART_GetLastChar();

// Serial command management
#ifndef SERIAL_CMD_BUF_SIZE
#define SERIAL_CMD_BUF_SIZE 32
#endif

class SerialCommandMgr {
private:
  static char commandBuffer[SERIAL_CMD_BUF_SIZE];
  static uint8_t commandBufferPos;
  static uint8_t _hasCommand;

public:
  static uint8_t hasCommand() { return _hasCommand; }

  static const char *command() {
    const char* ret = (_hasCommand == 1) ? commandBuffer : NULL;
    commandBufferPos = 0;
    _hasCommand = 0;
    
    return ret;
  }

  static const char *peekCommand() {
    return commandBuffer;
  }

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
