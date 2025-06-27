#ifndef _AVRTOOLS_INT0_SERIAL_H
#define _AVRTOOLS_INT0_SERIAL_H

#include <stddef.h>
#include <stdint.h>


#ifndef INT0_SERIAL_TRANSMIT_PORT
#define INT0_SERIAL_TRANSMIT_PORT B
#endif

// BAUD RATE is only 9600, this is a bitbang serial line in case the hardware USART is used
void INT0_Init(uint8_t tpin, volatile void (*INT0_rec_cb)(uint8_t));

uint8_t INT0_Transmit(uint8_t data);

void INT0_WriteString(const char *str);
void INT0_WritePString(const char *str);
void INT0_WriteInt(int32_t i, uint8_t base = 10);
void INT0_WriteUInt(uint32_t i, uint8_t base = 10);
void INT0_WriteChar(char d);

void INT0_WriteFloat(float d, uint8_t width = 11, uint8_t prec = 2);

// Serial command management
#ifndef INT0_SERIAL_CMD_BUF_SIZE
#define INT0_SERIAL_CMD_BUF_SIZE 32
#endif

class INT0_SerialCommandMgr {
   private:
    static char commandBuffer[INT0_SERIAL_CMD_BUF_SIZE];
    static uint8_t commandBufferPos;
    static uint8_t _hasCommand;

   public:
    static uint8_t hasCommand() { return _hasCommand; }

    static const char *command() {
        const char *ret = (_hasCommand == 1) ? commandBuffer : NULL;
        commandBufferPos = 0;
        _hasCommand = 0;

        return ret;
    }

    static const char *peekCommand() {
        return commandBuffer;
    }

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
