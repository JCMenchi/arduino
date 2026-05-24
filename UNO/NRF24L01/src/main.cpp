#include <avr/io.h>
#include <gpio.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <util/delay.h>

#include "millisec.h"
#include "nrf24mgr.h"
#include "tinyspi.h"
#include "usart_serial.h"

const uint8_t RADIO_CE_PIN = 1;
const uint8_t RADIO_CS_PIN = 2;

SPIManager spi;
NRF24Manager radio(NRF24_DYNAMIC_PAYLOAD_SIZE);

char ackBuffer[32] = "UNO ACK";
char sendBuffer[32] = "UNO";
int32_t counter = 0;

void setup() {
    // init serial com
    USART_Init(BAUD_RATE_115200, SerialCommandMgr::serialInput);

    USART_WriteString("Init SPI\n");
    // init SPI bus to control NRF24
    spi.startMaster();
    // init NRF24
    USART_WriteString("Wait for radio\n");
    _delay_ms(1500);  // give some time to NRF24 module to start
    USART_WriteString("Init radio\n");
    radio.setDestinationAddress("atn84");
    radio.setMyAddress("amg32");
    radio.init(&spi, RADIO_CE_PIN, RADIO_CS_PIN);

    // ready to enter main loop
    USART_WriteString("UNO Ready\n");
    _delay_ms(100);

    //radio.set_ack_buffer((uint8_t*)ackBuffer, strlen(ackBuffer));
    radio.info();
}

void execCommand(const char* cmd) {
    // check if command is defined
    if (cmd == NULL || strlen(cmd) == 0) return;

    USART_WriteString("Exec command: ");
    USART_WriteString(cmd);
    USART_WriteString("\n\n");
    if (strcmp(cmd, "status") == 0) {
        radio.summary();
    } else if (strcmp(cmd, "info") == 0) {
        // radio.setDynamicPayload();
        radio.info();
    }
}

uint32_t previousSendCounter = 0;
const int32_t send_interval = 5000;

void loop() {
    uint32_t now = milliseconds();
    if (SerialCommandMgr::hasCommand()) {
        execCommand(SerialCommandMgr::command());
    }

    if (radio.dataAvailable() == 1) {
        USART_WriteString("now ");
        USART_WriteUInt(now / 1000);
        USART_WriteString("s: ");
        uint8_t l = 0;
        char* msg = (char*)radio.read_binary_message(l);
        if (msg) {
            msg[l] = '\0';
            USART_WriteString("Received: ");
            USART_WriteString(msg);
            USART_WriteString("\n");
        }

        counter++;
        memset(ackBuffer, 0, sizeof(ackBuffer));
        strcat(ackBuffer, "UNO ACK: ");
        itoa(counter, ackBuffer + strlen(ackBuffer), 10);
        radio.set_ack_buffer((uint8_t*)ackBuffer, strlen(ackBuffer));
    }

    //if (now - previousSendCounter >= send_interval) {
    //    // save the last time
    //    previousSendCounter = now;
    //    counter++;
    //    memset(sendBuffer, 0, sizeof(sendBuffer));
    //    strcat(sendBuffer, "UNO: ");
    //    itoa(counter, sendBuffer + strlen(sendBuffer), 10);
    //    USART_WriteString("now ");
    //    USART_WriteUInt(now / 1000);
    //    USART_WriteString("s: Send: ");
    //    USART_WriteString(sendBuffer);
    //    
    //    uint8_t length = strlen(sendBuffer);
    //    uint8_t* response = radio.send_binary((uint8_t*)sendBuffer, length);
    //    if (length == 33) {
    //        USART_WriteString(" => Send failed.");
    //    } else if (response) {
    //        response[length] = '\0'; // Ensure null-termination
    //        USART_WriteString(" => Received response: ");
    //        USART_WriteString((char*)response);
    //    } else {
    //        USART_WriteString(" => Acked.");
    //    }
    //    USART_WriteString("\n");
    //}
}//

#include <main.cpp.h>