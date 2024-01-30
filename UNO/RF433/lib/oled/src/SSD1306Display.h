#ifndef _SSD1306_DISPLAY_H
#define _SSD1306_DISPLAY_H

#include <stdint.h>

// Default I2C address is 0x3C or 0x3D if SA0 input is set high
#define SSD1306_I2C_ADDRESS 0x3C
#define SSD1306_DEVICE_ID 0x15

// Display Pixel colours  definition
#define SSD1306_WHITE_COLOR 0
#define SSD1306_BLACK_COLOR 1
#define SSD1306_INVERSE_COLOR 2

#define SSD1306_OFF 0
#define SSD1306_ON 1

// Scrolling definition
#define SSD1306_SCROLL_RIGHT 0
#define SSD1306_SCROLL_LEFT 1
#define SSD1306_SCROLL_VERTICAL_LEFT 2
#define SSD1306_SCROLL_VERTICAL_RIGHT 3

#define SSD1306_SCROLL_ON 1
#define SSD1306_SCROLL_OFF 0

// define values for frame number as defined in datasheet
#define SSD1306_SCROLL_2FRAMES 0x07
#define SSD1306_SCROLL_3FRAMES 0x04
#define SSD1306_SCROLL_4FRAMES 0x05
#define SSD1306_SCROLL_5FRAMES 0x00
#define SSD1306_SCROLL_25FRAMES 0x06
#define SSD1306_SCROLL_64FRAMES 0x01
#define SSD1306_SCROLL_128FRAMES 0x02
#define SSD1306_SCROLL_256FRAMES 0x03

#define OVERWRITE_MODE 0
#define OR_MODE 1
#define XOR_MODE 2
#define AND_MODE 3


class SSD1306Display {
public:
  SSD1306Display(uint8_t w, uint8_t h);
  ~SSD1306Display();

  void init(uint8_t contrast = 0x80);

  // Basic screen setup
  void enable(uint8_t on);
  void invert(uint8_t on);
  void flip(uint8_t on);
  void contrast(uint8_t contrast);

  // Special Effects
  void scrollArea(uint8_t startPage, uint8_t endPage, uint8_t startCol, uint8_t endCol, uint8_t dir, uint8_t nbFrame);
  void scroll(uint8_t mode);
  void scrollOnce(uint8_t startPage, uint8_t endPage, uint8_t startCol, uint8_t endCol, uint8_t dir);

  // Full screen update
  void drawScreen(uint8_t pattern, bool border = false);
  void drawPage(uint8_t p, uint8_t pattern);
  void drawPage(uint8_t p, uint8_t startcol, uint8_t nbcol, uint8_t pattern);

  // Drawing
  uint8_t updatePagePixel(uint8_t y, uint8_t colour);
  uint8_t updatePageColumn(uint8_t pattern, uint8_t mode, uint8_t mask = 0xFF);

  void drawString(uint8_t x, uint8_t y, const char *pText);
  
  void drawPixel(uint8_t x, uint8_t y, uint8_t color);
  void drawLine(uint8_t x0, uint8_t y0, uint8_t x1, uint8_t y1, uint8_t color);
  void drawSprite(uint8_t x, uint8_t y, uint8_t w, uint8_t h,
                  const uint8_t *data, uint8_t mode);

private:
  void setAddress(uint8_t x, uint8_t y);

  void drawHLine(uint8_t x0, uint8_t x1, uint8_t y0, uint8_t color);
  void drawVLine(uint8_t x0, uint8_t y1, uint8_t y0, uint8_t color);

  void drawString2(uint8_t x, uint8_t y, const char *pText);
  
  void send_data(uint8_t data);
  void start_data(uint8_t byte);
  void add_data(uint8_t byte);
  void stop_data(uint8_t byte);
  void send_command(uint8_t command);

  uint8_t _height;
  uint8_t _width;
};

#endif
