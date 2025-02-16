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

#define WDG_LED_PORTID A
#define WDG_LED_PIN 5

// RX/TX are inverted on bluetooth chip, (cross cable, like a null modem)
#define BT_TX PIND4
#define BT_STATE_PORT B
#define BT_STATE_PIN 2
#include <int0_serial.h>
uint8_t curBTState = 0;
uint32_t lastBTStateOn = 0;

#include "tank.h"
Tank tank;

#include <SSD1306Display.h>
SSD1306Display display(128, 32);

void setup() {
    // init debug LED
    GPIO_OUTPUT(LED_PORTID, ON_LED_PIN);

    GPIO_OUTPUT(WDG_LED_PORTID, WDG_LED_PIN);

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


    GPIO_INPUT(BT_STATE_PORT, BT_STATE_PIN);
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

uint8_t on = 0;

uint32_t prev = 0;

uint8_t watchdog = 10;

void execATCommand(const char *cmd) {
    INT0_WriteString(cmd);
    INT0_WriteString("\r\n");
    _delay_ms(100);
    if (INT0_SerialCommandMgr::hasCommand()) {
        USART_WriteString(INT0_SerialCommandMgr::command());
        USART_WriteString("\n");
    }
}

void execCommand(uint32_t now, const char *cmd) {
    // check if command is defined
    if (cmd == NULL || strlen(cmd) == 0) {
        return;
    }

    if (cmd[0] == 'A' && cmd[1] == 'T') {
        execATCommand(cmd);
    }

    //INT0_WriteString(cmd);
    // USART_WriteString(cmd);
    display.drawPage(2, 0x00);
    display.drawString(0, 16, ">");
    display.drawString(10, 16, cmd);

    if (strcmp(cmd, STATUS_CMD) == 0) {
        tank.info();
    } else if (strcmp(cmd, FORWARD_CMD) == 0) {
        watchdog = (watchdog>0)?watchdog:10;
        tank.forward();
    } else if (strcmp(cmd, BACKWARD_CMD) == 0) {
        watchdog = (watchdog>0)?watchdog:10;
        tank.backward();
    } else if (strcmp(cmd, LEFT_CMD) == 0) {
        watchdog = (watchdog>0)?watchdog:10;
        tank.left();
    } else if (strcmp(cmd, RIGHT_CMD) == 0) {
        watchdog = (watchdog>0)?watchdog:10;
        tank.right();
    } else if (strcmp(cmd, STOP_CMD) == 0) {
        tank.stop();
    } else if (strcmp(cmd, ACCEL_CMD) == 0) {
        tank.accel();
    } else if (strcmp(cmd, DECEL_CMD) == 0) {
        tank.decel();
    } else if (cmd[0] == 'W') {
        watchdog = atoi(cmd+1);
    } else if (cmd[0] == 'H') {
        tank.shiftregistry()->allHigh();
        _delay_ms(2000);
    }
    tank.info();
    tank.display(&display);
}



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

    if (curBTState == 1) {
        display.drawString(5, 8, "BT: ON ");
    } else {
        display.drawString(5, 8, "BT: OFF");
    }

    const uint8_t base = 0;
    const uint8_t width = 6;
    const uint8_t line = 0;

    if (watchdog < 10) {
        display.drawString(80, 8, "00");
        display.drawInt(80 + 2*width, 8, watchdog, 10);
    } else if (watchdog < 100) {
        display.drawString(80, 8, "0");
        display.drawInt(80 + width, 8, watchdog, 10);
    } else {
        display.drawInt(80, 8, watchdog, 10);
    }

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
        if (hour < 10) {
            INT0_WriteChar('0');
        }
        INT0_WriteUInt(hour);
        INT0_WriteString(":");
        if (min < 10) {
            INT0_WriteChar('0');
        }
        INT0_WriteUInt(min);
        INT0_WriteString(":");
        if (sec < 10) {
            INT0_WriteChar('0');
        }
        INT0_WriteUInt(sec);
        INT0_WriteString(" W");
        INT0_WriteUInt(watchdog);
        INT0_WriteChar('\n');
    }
}

void loop() {
    uint32_t now = milliseconds();

    if ((now - prev) > 1000) {
        on = (on == 0) ? 1 : 0;
        showState(now, on);
        prev = now;

        if (watchdog > 1) {
            GPIO_SET_HIGH(WDG_LED_PORTID, WDG_LED_PIN);
            watchdog--;
        } else if (watchdog == 1) {
            watchdog = 0;
            GPIO_SET_LOW(WDG_LED_PORTID, WDG_LED_PIN);
            execCommand(now, "S");
        }
    }


    // check BT com
    uint8_t btstate = GPIO_READ(BT_STATE_PORT, BT_STATE_PIN);
    if (btstate == 0 && curBTState != 0) {
        curBTState = 0;
    } if (btstate == 1 && curBTState == 0) {
        lastBTStateOn = now;
        curBTState = 2;
    } else if (btstate == 1 && ((now - lastBTStateOn) > 1000)) {
        curBTState = 1;
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