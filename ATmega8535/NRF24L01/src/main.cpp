#include "usart_serial.h"
#include <stddef.h>
#include <util/delay.h>
#include <avr/io.h>

#include "nrf24l01.h"
#include "nrf24l01-mnemonics.h"
#include "SPIManager.h"

void setup() {
  USART_Init(BAUD_RATE_9600, NULL);
  
  nrf24_init(&DDRA, &PORTA, 4, &DDRA, &PORTA, 2);
}

int32_t count = 0;

void loop() {
  count++;
  USART_WriteString("data[");
  USART_WriteInt(count);
  USART_WriteString("]\n");

  uint8_t ack = nrf24_send_message("Hello");

  if (ack) {
    USART_WriteString("ACK\n");
  } else {
    USART_WriteString("NO ACK\n");
  }
  
  _delay_ms(5000);
}


#include <main.cpp.h>