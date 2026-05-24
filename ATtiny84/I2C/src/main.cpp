
//#define HAS_NUNCHUK

#include <avr/io.h>
#include <gpio.h>
#include <int0_serial.h>
#include <millisec.h>
#include <avr/pgmspace.h>

#include <util/delay.h>

#include <SSD1306Display.h>
SSD1306Display oled(128, 32);

uint32_t previousSendCounter = 0;
const int32_t send_interval = 60000;

#define HAS_NUNCHUK

#ifdef HAS_NUNCHUK
#include <nunchuk.h>
Nunchuk joystick;
#endif
#include <string.h>

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
    joystick.display_calibration();

    oled.init(0x01);
    oled.invert(SSD1306_OFF);
    oled.flip(SSD1306_ON);
    oled.drawScreen(0x00, true);
    oled.drawString(3, 8, "Pos(X,Y): ");
    oled.drawString(3, 16, "Button: ");
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
const int32_t interval = 250;  // interval at which to blink (milliseconds)

void loop() {
    unsigned long currentMillis = milliseconds();

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

        joystick.update();
        oled.drawString(60, 8, "          ");
        oled.drawInt(60, 8, joystick.joystick_x(), 10);
        oled.drawInt(90, 8, joystick.joystick_y(), 10);
        
        if (joystick.c_button()) {
            oled.drawChar(50, 16, 'C');
        } else {
            oled.drawChar(50, 16, ' ');
        }
        if (joystick.z_button()) {
            oled.drawChar(60, 16, 'Z');
            display(joystick);
        } else {
            oled.drawChar(60, 16, ' ');
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