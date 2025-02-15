#include <avr/io.h>
#include <avr/pgmspace.h>
#include <gpio.h>
#include <millisec.h>
#include <pwm.h>
#include <stdlib.h>
#include <string.h>
#include <usart_serial.h>
#include <util/delay.h>

#ifndef LED_PORTID
#define LED_PORTID A
#endif
#define ON_LED_PIN 0

// RX/TX are inverted on bluetooth chip, (cross cable, like a null modem)
#define BT_TX PIND4
#include <int0_serial.h>

#include "tank.h"
Tank tank;

#include <SSD1306Display.h>
SSD1306Display display(128, 32);

void setup() {
    // init debug LED
    GPIO_OUTPUT(LED_PORTID, ON_LED_PIN);

    // startup blinking of ON LED
    GPIO_SET_HIGH(LED_PORTID, ON_LED_PIN);
    _delay_ms(200);
    GPIO_SET_LOW(LED_PORTID, ON_LED_PIN);
    _delay_ms(500);
    GPIO_SET_HIGH(LED_PORTID, ON_LED_PIN);
    _delay_ms(200);
    GPIO_SET_LOW(LED_PORTID, ON_LED_PIN);
    _delay_ms(500);
    GPIO_SET_HIGH(LED_PORTID, ON_LED_PIN);

    // init serial com; only 9600 for ATmega8535 (is it a bug?)
    USART_Init(BAUD_RATE_9600, SerialCommandMgr::serialInput);
    USART_WritePString(PSTR("ATmega8535 serial com ready\n"));

    display.init();
    display.invert(SSD1306_OFF);
    display.flip(SSD1306_OFF);

    display.drawScreen(0x00, false);
    display.drawPString(60, 0, PSTR("ATmega8535"));
    tank.setup();

    // init Bluetooth
    INT0_Init(BT_TX, INT0_SerialCommandMgr::serialInput);
}

#define STATUS_CMD "status"
#define LEFT_CMD "l"
#define RIGHT_CMD "r"
#define FORWARD_CMD "f"
#define BACKWARD_CMD "b"
#define STOP_CMD "S"
#define ACCEL_CMD "+"
#define DECEL_CMD "-"

void execCommand(uint32_t now, const char *cmd) {
    // check if command is defined
    if (cmd == NULL || strlen(cmd) == 0) {
        return;
    }

    //INT0_WriteString(cmd);
    // USART_WriteString(cmd);
    display.drawPage(2, 0x00);
    display.drawString(0, 16, ">");
    display.drawString(10, 16, cmd);

    if (strcmp(cmd, STATUS_CMD) == 0) {
        tank.info();
    } else if (strcmp(cmd, FORWARD_CMD) == 0) {
        tank.forward();
    } else if (strcmp(cmd, BACKWARD_CMD) == 0) {
        tank.backward();
    } else if (strcmp(cmd, LEFT_CMD) == 0) {
        tank.left();
    } else if (strcmp(cmd, RIGHT_CMD) == 0) {
        tank.right();
    } else if (strcmp(cmd, STOP_CMD) == 0) {
        tank.stop();
    } else if (strcmp(cmd, ACCEL_CMD) == 0) {
        tank.accel();
    } else if (strcmp(cmd, DECEL_CMD) == 0) {
        tank.decel();
    }
    tank.info();
    tank.display(&display);
}

uint8_t on = 0;

uint32_t prev = 0;

void showState(uint32_t now, uint8_t isOn) {
    uint8_t sec = (now / 1000) % 60;
    uint8_t min = ((now / 1000) / 60) % 60;
    uint8_t hour = (((now / 1000) / 60) / 60);

    if (isOn == 0) {
        GPIO_SET_LOW(LED_PORTID, ON_LED_PIN);
        // USART_WritePString(PSTR("OFF\n"));
        // display.drawPage(1, 0x00);
    } else {
        GPIO_SET_HIGH(LED_PORTID, ON_LED_PIN);
        // USART_WritePString(PSTR("ON\n"));
        // display.drawPage(1, 0x00);
    }

    const uint8_t base = 0;
    const uint8_t width = 6;
    const uint8_t line = 0;

    if (hour >= 10) {
        display.drawInt(base, line, hour, 10);
    } else {
        display.drawString(base, line, "0");
        display.drawInt(base + width, line, hour, 10);
    }
    display.drawString(base + 2 * width, line, ":");

    if (min >= 10) {
        display.drawInt(base + 3 * width, line, min, 10);
    } else {
        display.drawString(base + 3 * width, line, "0");
        display.drawInt(base + 4 * width, line, min, 10);
    }

    display.drawString(base + 5 * width, line, ":");

    if (sec >= 10) {
        display.drawInt(base + 6 * width, line, sec, 10);
    } else {
        display.drawString(base + 6 * width, line, "0");
        display.drawInt(base + 7 * width, line, sec, 10);
    }

    if ((sec % 10) == 0) {
        INT0_WriteString("Time: ");
        INT0_WriteUInt(hour);
        INT0_WriteString(":");
        INT0_WriteUInt(min);
        INT0_WriteString(":");
        INT0_WriteUInt(sec);
        INT0_WriteChar('\n');
    }
}

void loop() {
    uint32_t now = milliseconds();

    if ((now - prev) > 1000) {
        on = (on == 0) ? 1 : 0;
        showState(now, on);
        prev = now;
    }

    // exec user commands
    if (SerialCommandMgr::hasCommand()) {
        execCommand(now, SerialCommandMgr::command());
    }

    if (INT0_SerialCommandMgr::hasCommand()) {
        execCommand(now, INT0_SerialCommandMgr::command());
    }

    _delay_ms(1);
}

#include <main.cpp.h>