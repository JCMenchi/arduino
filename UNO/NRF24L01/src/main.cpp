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
NRF24Manager radio(-1);

char ackBuffer[32] = "UNO INIT";

void setup() {
    // init serial com
    USART_Init(BAUD_RATE_115200, SerialCommandMgr::serialInput);

    USART_WriteString("Arduino UNO\n");

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

    radio.set_ack_buffer((uint8_t*)ackBuffer, strlen(ackBuffer));
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
        radio.info();
     } else if (strcmp(cmd, "init") == 0) {
        radio.setDestinationAddress("atn84");
        radio.setMyAddress("amg32");
        radio.init(&spi, RADIO_CE_PIN, RADIO_CS_PIN);
    } else if (strcmp(cmd, "activate") == 0) {
        radio.activate();
    } else {
        memset(ackBuffer, 0, sizeof(ackBuffer));
        strcat(ackBuffer, cmd);
        
        radio.set_ack_buffer((uint8_t*)ackBuffer, strlen(ackBuffer));
    }
}

uint32_t previousSendCounter = 0;

uint32_t lastWatchdogMessageTime = 0;

void decodeRemoteProtocolJoystick(const char* msg) {
    if (msg[2] == 'J' && strlen(msg) == 8) {
        uint8_t x = msg[3];
        uint8_t y = msg[4];
        char z = msg[5];
        char c = msg[6];
        uint8_t s = msg[7];
        memset(ackBuffer, 0, sizeof(ackBuffer));

        itoa(x, ackBuffer, 10);
        
        strcat(ackBuffer + strlen(ackBuffer), ",");
        itoa(y, ackBuffer + strlen(ackBuffer), 10);
        strcat(ackBuffer + strlen(ackBuffer), ",");
        itoa(s, ackBuffer + strlen(ackBuffer), 10);

        strcat(ackBuffer + strlen(ackBuffer), " ");
        if (z == 'Z') {
            strcat(ackBuffer + strlen(ackBuffer), " Z");
        } else {
            strcat(ackBuffer + strlen(ackBuffer), " ");
        }
        if (c == 'C') {
            strcat(ackBuffer + strlen(ackBuffer), "C");
        }

        USART_WriteString("   ack buf: ");
            USART_WriteString(ackBuffer);
            USART_WriteString("\n");
        radio.set_ack_buffer((uint8_t*)ackBuffer, strlen(ackBuffer));
    
    } else {
        USART_WriteString("Unknown J format: ");
        USART_WriteString(msg);
        USART_WriteString("\n");
        memset(ackBuffer, 0, sizeof(ackBuffer));
        strcat(ackBuffer, "ERR: bad J format");
        radio.set_ack_buffer((uint8_t*)ackBuffer, strlen(ackBuffer));
    }
}

void decodeRemoteProtocolV1(const char* msg) {
    if (msg[2] == 'J') {
        decodeRemoteProtocolJoystick(msg);
    } else if (msg[2] == 'W' ) {
        // watchdog
        lastWatchdogMessageTime = milliseconds();
    } else {
        USART_WriteString("Unknown msg type.");
        USART_WriteString(msg);
        USART_WriteString("\n");
        memset(ackBuffer, 0, sizeof(ackBuffer));
        strcat(ackBuffer, "ERR: bad msg");
        radio.set_ack_buffer((uint8_t*)ackBuffer, strlen(ackBuffer));
    }
}

void decodeRemoteProtocol(const char* msg) {
    if (msg[1] == '1') {
        decodeRemoteProtocolV1(msg);
    } else {
        USART_WriteString("Unknown REMOTE version.");
        memset(ackBuffer, 0, sizeof(ackBuffer));
        strcat(ackBuffer, "ERR: bad vers");
        radio.set_ack_buffer((uint8_t*)ackBuffer, strlen(ackBuffer));
    }
}



// Message format used to send sensor data over the nRF24L01 radio.
// The payload is kept compact to fit dynamic payload sizes.
struct RadioMessage {
    char protocol;     // Protocol identifier, e.g., 'W' for weather station
    char version;      // Protocol version, e.g., '1'
    char type;         // Message type, e.g., 'T' for temperature
    uint32_t timecode; // time
    int16_t temp;      // Temperature value in tenths of degrees Celsius
    int8_t humidity;   // Humidity value in percentage
    int16_t pressure;  // Pressure value in hPa (hectopascals)
};

// Send the latest temperature and humidity values over the nRF24L01 link.
void decodeWeatherStationProtocol(const char* msg) {
    RadioMessage rmsg;
    memcpy(&rmsg, msg, sizeof(RadioMessage));

    if (rmsg.protocol != 'W' || rmsg.version != '1') {
        USART_WriteString("Invalid weather station protocol version.\n");
        return;
    }

    if (rmsg.type != 'T') {
        USART_WriteString("Invalid weather station message type.\n");
        return;
    }

    USART_WriteString("Weather Station Data: ");
    USART_WriteString("Timecode: ");
    USART_WriteUInt(rmsg.timecode/1000);
    USART_WriteString(" s, ");
    USART_WriteString("Temp: ");
    USART_WriteUInt(rmsg.temp / 10);
    USART_WriteString(".");
    USART_WriteUInt(rmsg.temp % 10);
    USART_WriteString("C, Humidity: ");
    USART_WriteUInt(rmsg.humidity);
    USART_WriteString("%, Pressure: ");
    USART_WriteUInt(rmsg.pressure);
    USART_WriteString("hPa\n");
}


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

            // check protocol
            if (msg[0] == 'R' && strlen(msg) >= 3) {
                decodeRemoteProtocol(msg);
            } else if (msg[0] == 'W') {
                // weather station protocol
                // decode message
                decodeWeatherStationProtocol(msg);
            } else {
                USART_WriteString("Unknown protocol.");
                memset(ackBuffer, 0, sizeof(ackBuffer));
                strcat(ackBuffer, "ERR: bad prtl");
                radio.set_ack_buffer((uint8_t*)ackBuffer, strlen(ackBuffer));
            }

        }
    }

}

#include <main.cpp.h>