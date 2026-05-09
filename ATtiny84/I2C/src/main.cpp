
//#define HAS_NUNCHUK

#include <avr/io.h>
#include <gpio.h>
#include <int0_serial.h>
#include <millisec.h>
#include <avr/pgmspace.h>

#include <util/delay.h>
#include <tinyspi.h>
#include <nrf24mgr.h>

int ledState = GPIO_LOW;  // ledState used to set the LED

uint32_t previousMillis = 0;  // will store last time LED was updated

const int32_t interval = 500;  // interval at which to blink (milliseconds)

uint32_t previousSendCounter = 0;
const int32_t send_interval = 500;

#ifdef HAS_NUNCHUK
#include <nunchuk.h>
Nunchuk joystick;
#endif
#include <string.h>

const int8_t PL_SIZE = NRF24_DYNAMIC_PAYLOAD_SIZE;
SPIManager spimgr;
NRF24Manager radio(PL_SIZE);

uint8_t po = (*(volatile uint8_t *)(0x1B)) + 0x20;


void setup() {
    INT0_Init(PA7, INT0_SerialCommandMgr::serialInput);

    GPIO_OUTPUT(A, 0);
    GPIO_SET_LOW(A, 0);

    GPIO_OUTPUT(B, 0);
    GPIO_SET_LOW(B, 0);

    GPIO_OUTPUT(B, 1);
    GPIO_SET_LOW(B, 1);

    INT0_WriteString("Welcome ATtiny84\n");

    #ifdef HAS_NUNCHUK
    // init nunchuk
    if (!joystick.initialize()) {
        INT0_WriteString("Nunchuk not found.\n");
    } else {
        INT0_WriteString("Nunchuk initialized.\n");
    }
    #endif
     
    // init SPI bus to control NRF24
    INT0_WriteString("Init SPI.\n");
    spimgr.startMaster();

    // init NRF24
    _delay_ms(1500); // give some time to NRF24 module to start

    //radio.info();

    INT0_WriteString("Init radio.\n");

    GPIO_OUTPUT(A, 2);
    GPIO_SET_HIGH(A, 2);

    radio.setDestinationAddress("amg32");
    radio.setMyAddress("atn84");
    radio.init(&spimgr, PA1, PA2);  // Initialize NRF24 radio with SPI manager and CE pin
    
    INT0_WriteString("Starting in TRANSMITTER mode.\n");

    radio.flushRX();
    radio.flushTX();
    radio.info();
}

#ifdef HAS_NUNCHUK
void display(Nunchuk &joystick) {
    INT0_WriteString("Nunchuk info:\n");

    INT0_WriteString("joystick: ");
    INT0_WriteUInt(joystick.joystick_x());
    INT0_WriteString(", ");
    INT0_WriteUInt(joystick.joystick_y());
    INT0_WriteString("\n");

    INT0_WriteString("accel: ");
    INT0_WriteUInt(joystick.x_acceleration());
    INT0_WriteString(", ");
    INT0_WriteUInt(joystick.y_acceleration());
    INT0_WriteString(", ");
    INT0_WriteUInt(joystick.z_acceleration());
    INT0_WriteString("\n");

    INT0_WriteString("Button: ");
    INT0_WriteString(joystick.z_button() ? "Z" : "z");
    INT0_WriteString(" ");
    INT0_WriteString(joystick.c_button() ? "C" : "c");
    INT0_WriteString("\n");
}
#endif

int32_t counter = 0;
char sendBuffer[NRF24_MAX_MESSAGE_SIZE+1];

void loop() {
    unsigned long currentMillis = milliseconds();

    #ifdef HAS_NUNCHUK
    joystick.update();
    if (joystick.z_button()) {
        display(joystick);
    }
    #endif

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
            GPIO_SET_HIGH(B, 0);
            INT0_WriteString("Received command: ");
            INT0_WriteString(command);
            INT0_WriteString("\n");
            radio.info();
            GPIO_SET_LOW(B, 0);
        }
    }

    if (currentMillis - previousSendCounter >=  send_interval) {
        GPIO_SET_HIGH(B, 0);
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
        
        uint8_t length = strlen(sendBuffer);
        uint8_t* response = radio.send_binary((uint8_t*)sendBuffer, length);
        if (length == 33) {
            INT0_WritePString(PSTR(" => Send failed."));
        } else if (response) {
            response[length] = '\0'; // Ensure null-termination
            INT0_WritePString(PSTR(" => Ack msg: "));
            INT0_WriteString((char*)response);
        } else {
            INT0_WritePString(PSTR(" => Acked."));
        }

        INT0_WriteString("\n");

        GPIO_SET_LOW(B, 0);
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
        }
        INT0_WriteString("\n");
    }

}

#ifndef ARDUINO
#include <main.cpp.h>
#endif