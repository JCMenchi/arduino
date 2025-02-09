

#include <SSD1306Display.h>
#include <avr/io.h>
#include <gpio.h>
#include <millisec.h>
#include <pwm.h>
#include <shiftregister.h>
#include <stdlib.h>
#include <string.h>
#include <usart_serial.h>
#include <util/delay.h>

#include "tank.h"

#ifndef LED_PORTID
#define LED_PORTID A
#endif
#include <avr/pgmspace.h>
#define ON_LED_PIN 0

#define ON_LED SR_Q0

Tank tank;

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
    if (cmd == NULL || strlen(cmd) == 0)
        return;

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
        //display.drawPage(1, 0x00);
    } else {
        GPIO_SET_HIGH(LED_PORTID, ON_LED_PIN);
        // USART_WritePString(PSTR("ON\n"));
        //display.drawPage(1, 0x00);
    }

    const uint8_t base = 0;
    const uint8_t line = 0;

    if (hour >= 10) {
      display.drawInt(base, line, hour, 10);
    } else {
      display.drawString(base, line, "0");
      display.drawInt(base + 5, line, hour, 10);
    }
    display.drawString(base + 2*5, line, ":");
    
    if (min >= 10) {
      display.drawInt(base + 3*5, line, min, 10);
    } else {
      display.drawString(base + 3*5, line, "0");
      display.drawInt(base + 4*5, line, min, 10);
    }

    display.drawString(base + 5*5, line, ":");

    if (sec >= 10) {
      display.drawInt(base + 6*5, line, sec, 10);
    } else {
      display.drawString(base + 6*5, line, "0");
      display.drawInt(base + 7*5, line, sec, 10);
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

    _delay_ms(10);
}

#include <main.cpp.h>