
//#define HAS_NUNCHUK

#include <avr/io.h>
#include <gpio.h>
#include <int0_serial.h>
#include <millisec.h>
#include <avr/pgmspace.h>

#include <util/delay.h>
#include <tinyspi.h>
#include <nrf24mgr.h>
#include <SSD1306Display.h>

int ledState = GPIO_LOW;  // ledState used to set the LED

uint32_t previousMillis = 0;  // will store last time LED was updated

const int32_t interval = 500;  // interval at which to blink (milliseconds)

uint32_t previousSendCounter = 0;
const int32_t send_interval = 2000;

#include <string.h>

const int8_t PL_SIZE = NRF24_DYNAMIC_PAYLOAD_SIZE;
SPIManager spimgr;
NRF24Manager radio(PL_SIZE);
SSD1306Display display(128, 32);

uint8_t po = (*(volatile uint8_t *)(0x1B)) + 0x20;

//#define HAS_NUNCHUK
#ifdef HAS_NUNCHUK
#include <nunchuk.h>
Nunchuk joystick;
#endif

void setup() {
    INT0_Init(PA7, INT0_SerialCommandMgr::serialInput);

    GPIO_OUTPUT(A, 0);
    GPIO_SET_LOW(A, 0);

     // init NRF24
    _delay_ms(1500); // give some time to NRF24 module to start
    
    INT0_WritePString(PSTR("Welcome ATtiny84\n"));

     
    // init SPI bus to control NRF24
    INT0_WritePString(PSTR("Init SPI.\n"));
    spimgr.startMaster();

    INT0_WritePString(PSTR("Init radio.\n"));

    GPIO_OUTPUT(A, 2); // PA2 is CSN for radio, set to output
    radio.setDestinationAddress("amg32");
    radio.setMyAddress("atn84");
    radio.init(&spimgr, PA1, PA2);  // Initialize NRF24 radio with SPI manager and CE pin
    
    INT0_WritePString(PSTR("Starting in TRANSMITTER mode.\n"));

    radio.flushRX();
    radio.flushTX();
    radio.info();

    display.init(0x01);
    display.invert(SSD1306_OFF);
    display.flip(SSD1306_ON);
    display.drawScreen(0x00, false);

    display.drawString(90, 0, "atn84");

    #ifdef HAS_NUNCHUK
    // init nunchuk
    if (!joystick.initialize()) {
        INT0_WritePString(PSTR("Nunchuk not found.\n"));
    } else {
        INT0_WritePString(PSTR("Nunchuk initialized.\n"));
    }
    #endif
}

int32_t counter = 0;
char sendBuffer[NRF24_MAX_MESSAGE_SIZE+1];

void loop() {
    unsigned long currentMillis = milliseconds();

    if (currentMillis - previousMillis >=  interval) {
        // save the last time you blinked the LED
        previousMillis = currentMillis;

        // if the LED is off turn it on and vice-versa:
        if (ledState == GPIO_LOW) {
            ledState = GPIO_HIGH;
            GPIO_SET_HIGH(A, 0);
            //INT0_WriteUInt(currentMillis);
            //INT0_WriteString("\n");
        } else {
            ledState = GPIO_LOW;
            GPIO_SET_LOW(A, 0);
        }
    }

    if (INT0_SerialCommandMgr::hasCommand()) {
        const char *command = INT0_SerialCommandMgr::command();
        if (command) {
            INT0_WriteString("Received command: ");
            INT0_WriteString(command);
            INT0_WriteString("\n");
            radio.info();
        }
    }

    if (currentMillis - previousSendCounter >=  send_interval) {
        // save the last time
        previousSendCounter = currentMillis;
        counter += 1;

        memset(sendBuffer, 0, sizeof(sendBuffer));
        strcat(sendBuffer, "Tiny84: ");
        itoa(counter, sendBuffer + strlen(sendBuffer), 10);

        INT0_WriteString("now ");
        INT0_WriteUInt(currentMillis / 1000);
        INT0_WriteString("s: ");
        INT0_WritePString(PSTR(" Send msg: "));
        INT0_WriteString(sendBuffer);
        display.clearPage(1);
        display.clearPage(2);
        display.drawString(0, 8, "SND: ");
        display.drawString(28, 8, sendBuffer);

        uint8_t length = strlen(sendBuffer);
        uint8_t res = radio.send_binary((uint8_t*)sendBuffer, length);
        if (res == 33) {
            INT0_WritePString(PSTR(" => Send failed."));
            display.drawString(0, 16, "ACK: failed");
        } else {
            INT0_WritePString(PSTR(" => Acked."));
        }

        INT0_WriteString("\n");
    }

    int8_t p = radio.dataAvailable();

    if (p == 1) {
        INT0_WriteString("now ");
        INT0_WriteUInt(currentMillis / 1000);
        INT0_WriteString("s: ");
        INT0_WriteString(" Received msg: ");
        uint8_t l;
        char* response = (char*) radio.read_binary_message(l);
        if (response) {
            response[l] = '\0'; // Ensure null-termination
            
            INT0_WriteString(response);
            INT0_WriteString("\n");
             display.clearPage(3);
            display.drawString(0, 24, "RCV: ");
            display.drawString(28, 24, response);
        }
    } else if (p == 0) {
        INT0_WriteString("now ");
        INT0_WriteUInt(currentMillis / 1000);
        INT0_WriteString("s: ");
        INT0_WriteString(" Received ack message ");
        uint8_t l;
        char* response = (char*) radio.read_binary_message(l);
        if (response) {
            response[l] = '\0'; // Ensure null-termination
            INT0_WriteString(response);
            display.clearPage(2);
            display.drawString(0, 16, "ACK: ");
            display.drawString(28, 16, response);
        }
        INT0_WriteString("\n");
    }

}

#ifndef ARDUINO
#include <main.cpp.h>
#endif