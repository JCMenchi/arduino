
//#define HAS_NUNCHUK

#include <avr/io.h>
#include <gpio.h>
#include <int0_serial.h>
#include <millisec.h>
#include <avr/pgmspace.h>

#include <util/delay.h>

uint32_t previousSendCounter = 0;
const int32_t send_interval = 60000;

#define HAS_NUNCHUK

#ifdef HAS_NUNCHUK
#include <nunchuk.h>
Nunchuk joystick;
#endif
#include <string.h>

uint8_t po = (*(volatile uint8_t *)(0x1B)) + 0x20;

void setup() {
    INT0_Init(PA7, INT0_SerialCommandMgr::serialInput);

    GPIO_OUTPUT(A, 0);
    GPIO_SET_LOW(A, 0);

     // init NRF24
    _delay_ms(1500); // give some time to NRF24 module to start
    
    INT0_WritePString(PSTR("Welcome ATtiny84\n"));

    #ifdef HAS_NUNCHUK
    // init nunchuk
    if (!joystick.initialize()) {
        INT0_WritePString(PSTR("Nunchuk not found.\n"));
    } else {
        INT0_WritePString(PSTR("Nunchuk initialized.\n"));
    }
    #endif
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

int ledState = GPIO_LOW;  // ledState used to set the LED
uint32_t previousMillis = 0;  // will store last time LED was updated
const int32_t interval = 500;  // interval at which to blink (milliseconds)

void loop() {
    unsigned long currentMillis = milliseconds();

    #ifdef HAS_NUNCHUK
    //joystick.update();
    //if (joystick.z_button()) {
        //display(joystick);
    //}
    #endif

    if (currentMillis - previousMillis >=  interval) {
        // save the last time you blinked the LED
        previousMillis = currentMillis;

        // if the LED is off turn it on and vice-versa:
        if (ledState == GPIO_LOW) {
            ledState = GPIO_HIGH;
            GPIO_SET_HIGH(A, 0);
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

            joystick.update();
            display(joystick);
        }
    }

}

#ifndef ARDUINO
#include <main.cpp.h>
#endif