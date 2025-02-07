

#include <util/delay.h>

#include <avr/io.h>
#include <stdlib.h>
#include <string.h>

#include <gpio.h>
#include "usart_serial.h"
#include <shiftregister.h>


#ifndef LED_PORTID
#define LED_PORTID A
#endif

const uint8_t ON_LED_PIN = 0;

const uint8_t DATA_PIN = 1;  // DS
const uint8_t LATCH_PIN = 2; // ST_CP
const uint8_t CLOCK_PIN = 3; // SH_CP

ShiftRegisterPort shiftreg(DATA_PIN, LATCH_PIN, CLOCK_PIN);

#define SR_Q0 0x01
#define SR_Q1 0x02
#define SR_Q2 0x04
#define SR_Q3 0x08
#define SR_Q4 0x10
#define SR_Q5 0x20
#define SR_Q6 0x40
#define SR_Q7 0x80

#define ON_LED SR_Q0

#define MOTOR1_FORWARD SR_Q1
#define MOTOR1_REVERSE SR_Q2
#define MOTOR1_ON SR_Q5

#define MOTOR2_FORWARD SR_Q3
#define MOTOR2_REVERSE SR_Q4
#define MOTOR2_ON SR_Q6

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
  
  // init serial com
  USART_Init(BAUD_RATE_57600, SerialCommandMgr::serialInput);
  USART_WriteString("ATmega8535 serial com ready\n");

  shiftreg.setup();
  shiftreg.allLow();

  //USART_WriteString("ATmega8535 ready\n");
}

void loop() {
  GPIO_SET_LOW(LED_PORTID, ON_LED_PIN);
  shiftreg.setMask(ON_LED);
  _delay_ms(500);

  GPIO_SET_HIGH(LED_PORTID, ON_LED_PIN);
  shiftreg.setMask(0);
  _delay_ms(500);
}

#include <main.cpp.h>