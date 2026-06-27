
// AVR libc includes
#include <avr/io.h>
#include <avr/pgmspace.h>
#include <string.h>
#include <util/delay.h>

// avrftools includes
#include <gpio.h>
#ifdef HAS_INT0_SERIAL
#include <int0_serial.h>
#endif
#include <millisec.h>

// Peripheral library includes
#include <SSD1306Display.h>
#include <bitmap_font.h>
#include <nrf24mgr.h>
#include <nunchuk.h>
#include <tinyspi.h>

// Create global instances of peripherals
SPIManager spimgr;

const int8_t PL_SIZE = NRF24_DYNAMIC_PAYLOAD_SIZE;
NRF24Manager radio(PL_SIZE);

#define HAS_DISPLAY
#ifdef HAS_DISPLAY
SSD1306Display display(128, 32);
#endif

Nunchuk joystick;
bool nunchukInitialized = false;

// DISPLAY positoning constants
const uint8_t DISPLAY_TEXT_OFFSET = 4;
const uint8_t DISPLAY_SEND_Y_POS = 8;
const uint8_t DISPLAY_ACK_Y_POS = 16;
const uint8_t DISPLAY_ERR_Y_POS = 24;

// setup peripherals and initial state
void setup() {
    // LED driver setup
    GPIO_OUTPUT(A, 0);
    GPIO_SET_LOW(A, 0);

#ifdef HAS_INT0_SERIAL
    // Init serial interface on INT0 and PA7 pin with callback to INT0_SerialCommandMgr::serialInput
    INT0_Init(PA7, INT0_SerialCommandMgr::serialInput);
    INT0_WritePString(PSTR("ATtiny84\n"));
#endif

    // init nunchuk, before all other peripherals
    if (!joystick.initialize()) {
#ifdef HAS_INT0_SERIAL
        INT0_WriteString("Nunchuk not found.\n");
#endif
    } else {
        nunchukInitialized = true;
        joystick.display_calibration();
    }

    // init NRF24
    _delay_ms(1500);  // give some time to NRF24 module to start
    // init SPI bus to control NRF24
    spimgr.startMaster();

    GPIO_OUTPUT(A, 2);  // PA2 is CSN for radio, set to output
    radio.setDestinationAddress("amg32");
    radio.setMyAddress("atn84");
    radio.init(&spimgr, PA1, PA2);  // Initialize NRF24 radio with SPI manager and CE pin

    //radio.flushRX();
    //radio.flushTX();
    radio.summary();

#ifdef HAS_DISPLAY
    // Init OLED Display
    display.init(0x01);
    display.invert(SSD1306_OFF);
    display.flip(SSD1306_OFF);
    display.drawScreen(0x00, false);
    display.drawString(90, 0, "atn84");

    if (!nunchukInitialized) {
        display.drawString(1, 0, "NC KO");
    } else {
        display.drawString(1, 0, "NC OK");
    }

    display.drawString(DISPLAY_TEXT_OFFSET, DISPLAY_SEND_Y_POS, "SND:");
    display.drawString(DISPLAY_TEXT_OFFSET, DISPLAY_ACK_Y_POS, "ACK:");
    display.drawString(DISPLAY_TEXT_OFFSET, DISPLAY_ERR_Y_POS, "ERR:");
#endif
}

// LED management
int ledState = GPIO_LOW;               // ledState used to set the LED
uint32_t ledStatePreviousMillis = 0;   // will store last time LED was updated
const int32_t ledBlinkInterval = 500;  // interval at which to blink (milliseconds)

void blinkLED(uint32_t currentMillis) {
    if (currentMillis - ledStatePreviousMillis >= ledBlinkInterval) {
        // save the last time you blinked the LED
        ledStatePreviousMillis = currentMillis;

        // if the LED is off turn it on and vice-versa:
        if (ledState == GPIO_LOW) {
            ledState = GPIO_HIGH;
            GPIO_SET_HIGH(A, 0);
        } else {
            ledState = GPIO_LOW;
            GPIO_SET_LOW(A, 0);
        }
    }
}

// Message management variables
uint32_t errMessageDisplayTime = 0;
const int32_t errMessageDisplayDelay = 1000;

uint32_t previousSendRadioMessage = 0;
const int32_t sendRadioMessageInterval = 500;

char sendBuffer[NRF24_MAX_MESSAGE_SIZE + 1];

void sendRadioMessage(uint32_t currentMillis) {
    if (currentMillis - previousSendRadioMessage >= sendRadioMessageInterval) {
        joystick.update();

        // changed = joystick.update();

        // save the last time
        previousSendRadioMessage = currentMillis;

        // if (!changed) return;

        // check if value are valid
        uint8_t x = joystick.joystick_x();
        uint8_t y = joystick.joystick_y();
        uint8_t s = joystick.joystick_strength();

        if (x == 0 || y == 0) {
            return;
        }
        memset(sendBuffer, 0, sizeof(sendBuffer));
        sendBuffer[0] = 'R';  // Protocol is R for remote
        sendBuffer[1] = '1';  // Protocol version 1
        sendBuffer[2] = 'J';  // Joystick position
        sendBuffer[3] = x;
        sendBuffer[4] = y;
        if (joystick.z_button()) {
            sendBuffer[5] = 'Z';
        } else {
            sendBuffer[5] = 'z';
        }
        if (joystick.c_button()) {
            sendBuffer[6] = 'C';
        } else {
            sendBuffer[6] = 'c';
        }
        if (s == 0) {
            sendBuffer[7] = 1;
        } else {
            sendBuffer[7] = s;
        }

#ifdef HAS_INT0_SERIAL
        // INT0_WriteString("now ");
        // INT0_WriteUInt(currentMillis / 1000);
        // INT0_WriteString("s: ");
        // INT0_WritePString(PSTR(" Send msg: "));
        // INT0_WriteString(sendBuffer);
#endif

#ifdef HAS_DISPLAY
        display.drawString(DISPLAY_TEXT_OFFSET + 5 * (FONT_CHAR_WIDTH + 1), DISPLAY_SEND_Y_POS, "              ");
        display.drawString(DISPLAY_TEXT_OFFSET + 5 * (FONT_CHAR_WIDTH + 1), DISPLAY_SEND_Y_POS, sendBuffer);
#endif

        uint8_t length = 8;
        uint8_t res = radio.send_binary((uint8_t*)sendBuffer, length);
        if (res == 33) {
#ifdef HAS_INT0_SERIAL
            INT0_WritePString(PSTR(" => Send failed.\n"));
#endif
            errMessageDisplayTime = currentMillis;

#ifdef HAS_DISPLAY
            display.drawString(DISPLAY_TEXT_OFFSET + 5 * (FONT_CHAR_WIDTH + 1), DISPLAY_ERR_Y_POS, "                     ");
            display.drawString(DISPLAY_TEXT_OFFSET + 5 * (FONT_CHAR_WIDTH + 1), DISPLAY_ERR_Y_POS, "send failed.");
#endif

        } else {
#ifdef HAS_INT0_SERIAL
            //INT0_WritePString(PSTR(" => Acked."));
#endif
        }
#ifdef HAS_INT0_SERIAL
        INT0_WriteString("\n");
#endif

        _delay_ms(10);
        display.drawString(DISPLAY_TEXT_OFFSET + 5 * (FONT_CHAR_WIDTH + 1), DISPLAY_ACK_Y_POS, "               ");
        memset(sendBuffer, 0, sizeof(sendBuffer));
        strcat(sendBuffer, "R1W");
        radio.send_binary((uint8_t*)sendBuffer, strlen(sendBuffer));
    }
}

void decodeReceivedMessage(int8_t pipeNum, uint32_t currentMillis) {
    if (pipeNum == 1) {
#ifdef HAS_INT0_SERIAL
        INT0_WriteString("now ");
        INT0_WriteUInt(currentMillis / 1000);
        INT0_WriteString("s: ");
        INT0_WriteString(" Received: ");
#endif
        uint8_t l;
        char* response = (char*)radio.read_binary_message(l);
        if (response) {
            response[l] = '\0';  // Ensure null-termination
#ifdef HAS_INT0_SERIAL
            INT0_WriteString(response);
            INT0_WriteString("\n");
#endif

#ifdef HAS_DISPLAY
            display.drawString(DISPLAY_TEXT_OFFSET + 5 * (FONT_CHAR_WIDTH + 1), DISPLAY_ERR_Y_POS, "               ");
            display.drawString(DISPLAY_TEXT_OFFSET + 5 * (FONT_CHAR_WIDTH + 1), DISPLAY_ERR_Y_POS, response);
#endif
        }
    } else if (pipeNum == 0) {
#ifdef HAS_INT0_SERIAL
        // INT0_WriteString("now ");
        // INT0_WriteUInt(currentMillis / 1000);
        // INT0_WriteString("s: ");
        // INT0_WriteString(" Received ack message ");
#endif
        uint8_t l;
        char* response = (char*)radio.read_binary_message(l);
        if (response) {
            response[l] = '\0';  // Ensure null-termination
#ifdef HAS_INT0_SERIAL
            INT0_WriteString(response);
#endif

#ifdef HAS_DISPLAY
            if (response[0] == 'E' && response[1] == 'R' && response[2] == 'R') {
                errMessageDisplayTime = currentMillis;
                display.drawString(1, DISPLAY_ERR_Y_POS, "                     ");
                display.drawString(1, DISPLAY_ERR_Y_POS, response + 3);
            } else {
                display.drawString(DISPLAY_TEXT_OFFSET + 5 * (FONT_CHAR_WIDTH + 1), DISPLAY_ACK_Y_POS, "               ");
                display.drawString(DISPLAY_TEXT_OFFSET + 5 * (FONT_CHAR_WIDTH + 1), DISPLAY_ACK_Y_POS, response);
            }
#endif
        }
#ifdef HAS_INT0_SERIAL
        INT0_WriteString("\n");
#endif
    }
}

// Main loop
void loop() {
    unsigned long currentMillis = milliseconds();

    // Blink LED; used as a heartbeat to indicate the program is running. Toggles every 500ms.
    blinkLED(currentMillis);

#ifdef HAS_INT0_SERIAL
    // check if we have received a command over serial (INT0) and print it if so
    if (INT0_SerialCommandMgr::hasCommand()) {
        const char* command = INT0_SerialCommandMgr::command();
        if (command) {
            INT0_WriteString("Cmd: ");
            INT0_WriteString(command);
            INT0_WriteString("\n");
            radio.summary();
            radio.send_binary((uint8_t*)command, strlen(command));
        }
    }
#endif

    // send message over radio
    sendRadioMessage(currentMillis);

    // Check if we have received any messages over radio
    int8_t p = radio.dataAvailable();
    decodeReceivedMessage(p, currentMillis);

#ifdef HAS_DISPLAY
    // clear error
    if (currentMillis - errMessageDisplayTime > errMessageDisplayDelay) {
        display.drawString(DISPLAY_TEXT_OFFSET + 5 * (FONT_CHAR_WIDTH + 1), DISPLAY_ERR_Y_POS, "                     ");
    }
#endif
}

#ifndef ARDUINO
#include <main.cpp.h>
#endif