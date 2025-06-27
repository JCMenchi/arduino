
#define HAS_NUNCHUK

#include <avr/io.h>
#include <gpio.h>
#include <int0_serial.h>
#include <millisec.h>

#include <util/delay.h>
#include <tinyspi.h>
#include <nrf24mgr.h>

int ledState = GPIO_LOW;  // ledState used to set the LED

unsigned long previousMillis = 0;  // will store last time LED was updated

const long interval = 500;  // interval at which to blink (milliseconds)

#ifdef HAS_NUNCHUK
#include <nunchuk.h>
Nunchuk joystick;
#endif

SPIManager spimgr;
NRF24Manager radio(true);

//#define __SFR_OFFSET 0x20
//#define _MMIO_BYTE(mem_addr) (*(volatile uint8_t *)(mem_addr))
//#define _SFR_IO8(io_addr) _MMIO_BYTE((io_addr) + __SFR_OFFSET)
//#define PORTA   (*(volatile uint8_t *)(0x1B)) + 0x20

uint8_t po = (*(volatile uint8_t *)(0x1B)) + 0x20;

void setup() {
    INT0_Init(PA7, INT0_SerialCommandMgr::serialInput);

    GPIO_OUTPUT(A, 0);
    GPIO_SET_LOW(A, 0);

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
    _delay_ms(100); // give some time to NRF24 module to start
    INT0_WriteString("Init radio.\n");

    GPIO_OUTPUT(A, 2);
    GPIO_SET_HIGH(A, 2);
    radio.init(&spimgr, PA1, PA2);  // Initialize NRF24 radio with SPI manager and CE pin
    radio.listen();
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


void loop() {
    unsigned long currentMillis = milliseconds();

    #ifdef HAS_NUNCHUK
    joystick.update();
    if (joystick.z_button()) {
        display(joystick);
    }
    #endif

    if (currentMillis - previousMillis >= joystick.joystick_x() * interval / 255) {
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

        if (INT0_SerialCommandMgr::hasCommand()) {
            const char *command = INT0_SerialCommandMgr::command();
            if (command) {
                INT0_WriteString("Received command: ");
                INT0_WriteString(command);
                INT0_WriteString("\n");
                radio.send(command);
                radio.info();
            }
        }
    }
    
    //_delay_ms(10);  // Small delay to prevent excessive CPU usage
}

#ifndef ARDUINO
#include <main.cpp.h>
#endif