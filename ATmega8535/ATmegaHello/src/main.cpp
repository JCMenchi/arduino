#include <Arduino.h>

#include <RF24.h>
#include <SPI.h>

#include "BME280.h"
#include <SSD1306Display.h>
#include <millisec.h>
#include <stdlib.h>
#include <usart_serial.h>

#include <avr/io.h>

#define LED_DDR DDRB
#define LED_PORT PORTB
const int LED_PIN = PB1;       // PB1 leg 2 on chip
const uint8_t BUTTON_PIN = 16; // PC0 leg 21
int ledState = 1;              // ledState used to set the LED

unsigned long previousMillis = 0; // will store last time LED was updated

const unsigned long interval =
    5000; // interval at which to blink (milliseconds)

SSD1306Display display(128, 32);

#define pinCE 28           // PA2 "CE" du NRF24L01
#define pinCSN 26          // PA4 "CSN" du NRF24L01
RF24 radio(pinCE, pinCSN); // Instanciation du NRF24L01

volatile void USART_Data_received(uint8_t data, bool error) {
  // so far just echo it
  USART_WriteInt(data);
}

// Let these addresses be used for the pair
uint8_t address[][6] = { "1Node", "2Node" };
// It is very helpful to think of an address as a path instead of as
// an identifying device destination

// to use different addresses on a pair of radios, we need a variable to
// uniquely identify which address this radio will use to transmit
bool radioNumber = 1;  // 0 uses address[0] to transmit, 1 uses address[1] to transmit

char radiobuffer[16];

void setup() {
  LED_DDR |= (1 << LED_PIN);
  if (ledState) {
    LED_PORT |= (1 << LED_PIN);
  } else {
    LED_PORT &= ~(1 << LED_PIN);
  }
  pinMode(LED_PIN, OUTPUT);
  // digitalWrite(LED_PIN, ledState);

  // pinMode(BUTTON_PIN, INPUT_PULLUP);

  USART_Init(BAUD_RATE_9600, USART_Data_received);
  USART_WriteString("Welcome\n");

  display.init(0x01);
  display.invert(SSD1306_OFF);
  display.flip(SSD1306_OFF);
  display.drawScreen(0x00);

  BME280_begin();
  display.drawString(0, 0, BME280_chipModel());
  init_timer();

  //
  // initialize the transceiver on the SPI bus
  if (!radio.begin()) {
    USART_WriteString("radio hardware is not responding!!");
  } else {

    radioNumber = 1;

    // Set the PA Level low to try preventing power supply related problems
    // because these examples are likely run with nodes in close proximity to
    // each other.
    radio.setPALevel(RF24_PA_LOW); // RF24_PA_MAX is default.
    // save on transmission time by setting the radio to only transmit the
    // number of bytes we need to transmit a float
    radio.setPayloadSize(16); // float datatype occupies 4 bytes
    // set the TX address of the RX node into the TX pipe
    radio.openWritingPipe(address[radioNumber]); // always uses pipe 0
    // set the RX address of the TX node into a RX pipe
    radio.openReadingPipe(1, address[!radioNumber]); // using pipe 1
    // additional setup specific to the node's role
    radio.stopListening(); // put radio in TX mode
  }
}

static char numberbuffer[64];

void loop() {

  unsigned long currentMillis = milliseconds();

  uint8_t v = 1; // digitalRead(BUTTON_PIN);

  if (v == 0) {
    ledState = 1;
  } else if (currentMillis - previousMillis >= interval) {
    // save the last time you blinked the LED
    previousMillis = currentMillis;
    USART_WriteInt(currentMillis);
    ltoa(currentMillis / 1000, numberbuffer, 10);
    display.drawString(64, 0, numberbuffer);
    // if the LED is off turn it on and vice-versa:
    if (ledState == 0) {
      // USART_WriteInt(currentMillis);
      USART_WriteString(" Set LED HIGH\n");
      //display.drawString(30, 8, "Set LED HIGH  ");
      ledState = 1;
    } else {
      ledState = 0;
      USART_WriteString(" Set LED LOW\n");
      //display.drawString(30, 8, "Set LED LOW  ");
    }

    int32_t temperature;
    uint32_t pressure = BME280_pres(temperature);
    int16_t te = temperature / 100;
    uint16_t tf = temperature % 100;
    pressure = pressure / 100;
    snprintf(numberbuffer, 64, "%d.%dC %ldhPa", te, tf, pressure);
    display.drawString(0, 16, numberbuffer);
    bool ok = radio.write(numberbuffer, strlen(numberbuffer)+1);
    if (ok) {
      display.drawString(0, 24, "Radio com OK");
    } else {
      display.drawString(0, 24, "Radio com KO");
    }
    _delay_ms(1000);

  } else {
    // ltoa(currentMillis, numberbuffer, 10);
    // numberbuffer[strlen(numberbuffer) - 1] =
    //     numberbuffer[strlen(numberbuffer) - 2];
    // numberbuffer[strlen(numberbuffer) - 2] =
    //     numberbuffer[strlen(numberbuffer) - 3];
    // numberbuffer[strlen(numberbuffer) - 3] = '.';
    // // numberbuffer[strlen(numberbuffer)-1] = '\0';
    // display.drawString(0, 24, numberbuffer);
  }

  // set the LED with the ledState of the variable:
  // digitalWrite(LED_PIN, ledState);
  if (ledState) {
    LED_PORT |= (1 << LED_PIN);
  } else {
    LED_PORT &= ~(1 << LED_PIN);
  }
}

// #include <main.cpp.h>