#include "usart_serial.h"
#include <stddef.h>
#include <util/delay.h>
#include <avr/io.h>

#include "nrf24l01.h"
#include "nrf24l01-mnemonics.h"
#include "SPIManager.h"

/*
#include "SPI.h"
#include "nRF24L01.h"
#include "RF24.h"

#include <printf.h>

const uint64_t pipe = 0xF0F1F2F3F4LL;
RF24 radio(9,10);

void setup() {
  pinMode(A1, INPUT);
  Serial.begin(115200);

  radio.begin();

  if (radio.isValid()) {
    Serial.print("Radio OK: nRF24L01");
    if (radio.isPVariant()) {
      Serial.println("+");
    } else {
      Serial.println("");
    }
  } else {
    Serial.println("Radio KO");
  }

  if (radio.isChipConnected()) {
    Serial.println("Radio Connected");
  } else {
    Serial.println("Radio Not found");
  }

  radio.setChannel(0); // sélectionner le canal radio (0 à 127)

  // vitesse: RF24_250KBPS, RF24_1MBPS ou RF24_2MBPS
  radio.setDataRate(RF24_1MBPS);
  // puissance: RF24_PA_MIN=-18dBm, RF24_PA_LOW=-12dBm, RF24_PA_MED=-6dBM
  radio.setPALevel(RF24_PA_LOW);

  // printf_begin();
  // char buffer[1024];
  // radio.sprintfPrettyDetails(buffer);
  // Serial.print(buffer);
  // radio.printPrettyDetails();

  radio.openWritingPipe(pipe);
  radio.stopListening();
}
*/


void setup() {
  USART_Init(BAUD_RATE_115200, NULL);
  USART_WriteString("Ready\n");

  nrf24_init(&DDRB, &PORTB, 1, &DDRB, &PORTB, 2);
  nrf24_start_listening();
}

void loop() {

  //
  unsigned int r = nrf24_available();
  if (r) {
      const char* msg = nrf24_read_message();
      USART_WriteString("msg[");
      USART_WriteInt(r);
      USART_WriteString("]= '");
      USART_WriteString(msg);
      USART_WriteString("'\n");
  }
  

}


#include <main.cpp.h>