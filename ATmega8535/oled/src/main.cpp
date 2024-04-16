
#include <SSD1306Display.h>
#include <util/delay.h>
#include "TinyI2CMaster.h"

#include <avr/io.h>
#include <stdlib.h>
#include <string.h>

#include <gpio.h>
#include "usart_serial.h"

const uint8_t ON_LED_PIN = 0;
const uint8_t DEBUG_LED_PIN = 1;

SSD1306Display display(128, 32);

void setup() {
  // init debug LED
  GPIO_OUTPUT(A, ON_LED_PIN);
  GPIO_OUTPUT(A, DEBUG_LED_PIN);
  GPIO_SET_LOW(A, DEBUG_LED_PIN);

  // startup blinking of ON LED
  GPIO_SET_HIGH(A, ON_LED_PIN);
  _delay_ms(200);
  GPIO_SET_LOW(A, ON_LED_PIN);
  _delay_ms(500);
  GPIO_SET_HIGH(A, ON_LED_PIN);
  _delay_ms(200);
  GPIO_SET_LOW(A, ON_LED_PIN);
  _delay_ms(500);
  GPIO_SET_HIGH(A, ON_LED_PIN);
  
  // init serial com
  USART_Init(BAUD_RATE_57600, SerialCommandMgr::serialInput);
  USART_WriteString("ATmega8535 serial com ready\n");

  // init OLED display
  display.init(0x20);
  display.flip(SSD1306_OFF);
  display.drawScreen(0x00);

  display.drawString(0, 0, "init OK.");

  USART_WriteString("ATmega8535 ready\n");
}

void loop() {

}

#include <main.cpp.h>