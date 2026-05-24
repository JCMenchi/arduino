#include "usart_serial.h"
#include "SSD1306Display.h"

#include <MyVirtualWire.h>

#include <stdlib.h>

#include <util/delay.h>

SSD1306Display display(128, 32);


void setup()
{
  USART_Init(BAUD_RATE_115200, NULL);
  USART_WriteString("Welcome\n");

  display.enable(1);
  display.init(0x01);
  display.flip(SSD1306_OFF);

  display.drawScreen(0x0, true);

  display.drawLine(3,3,125,30, SSD1306_WHITE_COLOR);
  _delay_ms(2000);

  display.drawString(32,16, "1234567890");
  display.drawString(2,0,"Line1: ");
  display.drawString(2,8,"Line2: Hello");
  display.drawString(2,24,"Line4: World");

  //display.scrollArea(1, 2, 2, 126, SSD1306_SCROLL_VERTICAL_LEFT, SSD1306_SCROLL_2FRAMES);
  //display.scroll(SSD1306_SCROLL_ON);

  _delay_ms(2000);
  for(uint8_t i = 0; i < 16; ++i) {
    display.scrollOnce(0, 3, 0, 127, SSD1306_SCROLL_LEFT);
    _delay_ms(10);
  }
 
  _delay_ms(50000);
  display.scroll(SSD1306_SCROLL_OFF);

  display.scrollArea(0, 3, 50, 100, SSD1306_SCROLL_VERTICAL_LEFT, SSD1306_SCROLL_128FRAMES);
  display.scroll(SSD1306_SCROLL_ON);
  
  _delay_ms(5);
  display.scroll(SSD1306_SCROLL_OFF);

  _delay_ms(50000);
  display.scroll(SSD1306_SCROLL_OFF);
  display.drawScreen(0x0);
  display.drawPixel(5, 5, SSD1306_WHITE_COLOR);
  display.drawPixel(127, 31, SSD1306_WHITE_COLOR);

  _delay_ms(30000);
  display.drawScreen(0x0);

  display.drawLine(0,16, 127,16, SSD1306_WHITE_COLOR);
  display.drawLine(0,24, 127,24, SSD1306_WHITE_COLOR);

  display.drawLine(32,0,32,31, SSD1306_WHITE_COLOR);
  display.drawLine(64,0,64,31, SSD1306_WHITE_COLOR);
  display.drawLine(100,0,100,31, SSD1306_WHITE_COLOR);

  _delay_ms(30000);
  // Initialise the IO and ISR
  myvw_setup(500); // Bits per sec
  myvw_rx_start();
}

uint8_t ypos = 0;

void loop()
{
  uint8_t buf[VW_MAX_MESSAGE_LEN];
  uint8_t buflen = VW_MAX_MESSAGE_LEN;

  if (myvw_get_message(buf, &buflen)) // Non-blocking
  {
    // Message with a good checksum received, dump it.
    USART_WriteString("Got: ");

    USART_WriteString((const char*)buf);
    USART_WriteString("\n");
    display.drawString(0,ypos,(const char*)buf);
    ypos = (ypos + 8) % 32;
    USART_WriteString("Good: ");
    USART_WriteInt(myvw_get_rx_good());
    USART_WriteString(" Bad: ");
    USART_WriteInt(myvw_get_rx_bad());
    USART_WriteString("\n");
  }
}

#include <main.cpp.h>