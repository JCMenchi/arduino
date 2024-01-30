
#include <avr/pgmspace.h>
#include <stdlib.h>

#include "SSD1306Display.h"
#include "TinyI2CMaster.h"
#include "bitmap_font.h"

//#define HAS_SERIAL

#ifdef HAS_SERIAL
#include <usart_serial.h>
#endif

#define SSD1306_MEMORYMODE 0x20         
#define SSD1306_COLUMNADDR 0x21         
#define SSD1306_PAGEADDR 0x22           
#define SSD1306_SETCONTRAST 0x81        
#define SSD1306_CHARGEPUMP 0x8D         
#define SSD1306_SEGREMAP 0xA0           
#define SSD1306_DISPLAYALLON_RESUME 0xA4
#define SSD1306_DISPLAYALLON 0xA5       
#define SSD1306_NORMALDISPLAY 0xA6      
#define SSD1306_INVERTDISPLAY 0xA7      
#define SSD1306_SETMULTIPLEX 0xA8       
#define SSD1306_DISPLAYOFF 0xAE         
#define SSD1306_DISPLAYON 0xAF          
#define SSD1306_COMSCANINC 0xC0         
#define SSD1306_COMSCANDEC 0xC8         
#define SSD1306_SETDISPLAYOFFSET 0xD3   
#define SSD1306_SETDISPLAYCLOCKDIV 0xD5 
#define SSD1306_SETPRECHARGE 0xD9       
#define SSD1306_SETCOMPINS 0xDA         
#define SSD1306_SETVCOMDETECT 0xDB      

#define SSD1306_SETSTARTLINE 0x40 ///< See datasheet

#define SSD1306_SWITCHCAPVCC 0x02 ///< Gen. display voltage from 3.3V

#define SSD1306_DEACTIVATE_SCROLL 0x2E                    ///< Stop scroll
#define SSD1306_ACTIVATE_SCROLL 0x2F                      ///< Start scroll
#define SSD1306_SET_VERTICAL_SCROLL_AREA 0xA3             ///< Set scroll range


#define SSD1306_Swap(a, b)                                                      \
  {                                                                            \
    uint8_t t = a;                                                             \
    a = b;                                                                     \
    b = t;                                                                     \
  }

SSD1306Display::SSD1306Display(uint8_t w, uint8_t h) {
  _width = w;
  _height = h;
}

SSD1306Display::~SSD1306Display() {
}

#define SSD1306_STATUS 0x15

// Possible Wire error code for endTransmission()
//   0 .. success
//   1 .. length to long for buffer
//   2 .. address send, NACK received
//   3 .. data send, NACK received
//   4 .. other twi error (lost bus arbitration, bus error, ..)
//   5 .. timeout
void SSD1306Display::init(uint8_t contrast) {
  // Init I2C com
  TinyI2C.init();

  // Read one bit from device
  bool con = TinyI2C.start(SSD1306_I2C_ADDRESS, 1);
  if (!con) {
#ifdef HAS_SERIAL
    USART_WriteString("I2C connect error.\n");
#endif
    return;
  }

  // Check display type
  uint8_t u = TinyI2C.read();
  TinyI2C.stop();

#ifdef HAS_SERIAL
  USART_WriteString("Screen status register: ");
  USART_WriteInt(u);
  USART_WriteString("\n");

  uint8_t id = u & 0x3F;
  USART_WriteString(" Device ID: ");
  USART_WriteInt(id);
  USART_WriteString("\n");

  uint8_t on = u & 0x40;
  if (on == 0) {
    USART_WriteString(" is ON\n");
  } else {
    USART_WriteString(" is OFF\n");
  }
#endif

  // Init sequence
  TinyI2C.start(SSD1306_I2C_ADDRESS, 0);
  TinyI2C.write(0x00);
  TinyI2C.write(SSD1306_DISPLAYOFF);
  TinyI2C.write(SSD1306_SETDISPLAYCLOCKDIV);
  TinyI2C.write(0x80);
  TinyI2C.write(SSD1306_SETMULTIPLEX);
  TinyI2C.stop();

  send_command(_height-1);

  TinyI2C.start(SSD1306_I2C_ADDRESS, 0);
  TinyI2C.write(0x00);
  TinyI2C.write(SSD1306_SETDISPLAYOFFSET);
  TinyI2C.write(0x00);
  TinyI2C.write(SSD1306_SETSTARTLINE | 0x0);
  TinyI2C.write(SSD1306_CHARGEPUMP);
  TinyI2C.stop();

  send_command(0x14);

  TinyI2C.start(SSD1306_I2C_ADDRESS, 0);
  TinyI2C.write(0x00);
  TinyI2C.write(SSD1306_MEMORYMODE);
  TinyI2C.write(0x00);
  TinyI2C.write(SSD1306_SEGREMAP | 0x1);
  TinyI2C.write(SSD1306_COMSCANDEC);
  TinyI2C.stop();
  
  uint8_t comPins = 0x02;
  comPins = 0x02;

  send_command(SSD1306_SETCOMPINS);
  send_command(comPins);
  send_command(SSD1306_SETCONTRAST);
  send_command(contrast);

  send_command(SSD1306_SETPRECHARGE); // 0xd9
  send_command(0xF1);

  TinyI2C.start(SSD1306_I2C_ADDRESS, 0);
  TinyI2C.write(0x00);
  TinyI2C.write(SSD1306_SETVCOMDETECT);
  TinyI2C.write(0x40);
  TinyI2C.write(SSD1306_DISPLAYALLON_RESUME);
  TinyI2C.write(SSD1306_NORMALDISPLAY);
  TinyI2C.write(SSD1306_DEACTIVATE_SCROLL);
  TinyI2C.write(SSD1306_DISPLAYON);
  TinyI2C.stop();
}

// 10.1.12 Set Display ON/OFF (AEh/AFh)
// These single byte commands are used to turn the OLED panel display ON or OFF.
// When the display is ON, the selected circuits by Set Master Configuration command will be turned ON.
// When the display is OFF, those circuits will be turned OFF and the segment and common output are in VSS
// state and high impedance state, respectively. These commands set the display to one of the two states:
//   o AEh : Display OFF
//   o AFh : Display ON
void SSD1306Display::enable(uint8_t on) { send_command(on ? 0xAF : 0xAE); }


// 10.2.1 Horizontal Scroll Setup (26h/27h)
// This command consists of consecutive bytes to set up the horizontal scroll parameters and determines the
// scrolling start page, end page and scrolling speed.
// Before issuing this command the horizontal scroll must be deactivated (2Eh). Otherwise, RAM content may
// be corrupted.
//
// 10.2.2 Continuous Vertical and Horizontal Scroll Setup (29h/2Ah)
// This command consists of 6 consecutive bytes to set up the continuous vertical scroll parameters and
// determines the scrolling start page, end page, scrolling speed and vertical scrolling offset.
// The bytes B[2:0], C[2:0] and D[2:0] of command 29h/2Ah are for the setting of the continuous horizontal
// scrolling. The byte E[5:0] is for the setting of the continuous vertical scrolling offset. All these bytes together
// are for the setting of continuous diagonal (horizontal + vertical) scrolling. If the vertical scrolling offset byte
// E[5:0] is set to zero, then only horizontal scrolling is performed (like command 26/27h).
// Before issuing this command the scroll must be deactivated (2Eh). Otherwise, RAM content may be
// corrupted. The following figure (Figure 10-10 ) show the example of using the continuous vertical and
// horizontal scroll:
//
// 10.2.5 Set Vertical Scroll Area(A3h)
// This command consists of 3 consecutive bytes to set up the vertical scroll area. For the continuous vertical
// scroll function (command 29/2Ah), the number of rows that in vertical scrolling can be set smaller or equal to
// the MUX ratio.
void SSD1306Display::scrollArea(uint8_t startPage, uint8_t endPage, uint8_t startCol, uint8_t endCol, uint8_t dir, uint8_t nbFrame) 
{
  // Horizontal Scroll Setup
  if (dir == SSD1306_SCROLL_LEFT || dir == SSD1306_SCROLL_RIGHT) {
    TinyI2C.start(SSD1306_I2C_ADDRESS, 0);
    
    TinyI2C.write(0x00);
    TinyI2C.write((dir == SSD1306_SCROLL_LEFT)?0x27:0x26);
    TinyI2C.write(0x00);
    TinyI2C.write(startPage);
    TinyI2C.write(nbFrame);
    TinyI2C.write(endPage);
    TinyI2C.write(startCol);
    TinyI2C.write(endCol);
    TinyI2C.stop();
  } else if (dir == SSD1306_SCROLL_VERTICAL_LEFT || dir == SSD1306_SCROLL_VERTICAL_RIGHT) {

    
    TinyI2C.start(SSD1306_I2C_ADDRESS, 0);
    TinyI2C.write(0x00);
    TinyI2C.write(SSD1306_SET_VERTICAL_SCROLL_AREA);
    TinyI2C.write(0x0);
    TinyI2C.write(31);
    TinyI2C.stop();

    TinyI2C.start(SSD1306_I2C_ADDRESS, 0);
    TinyI2C.write(0x00);
    TinyI2C.write((dir == SSD1306_SCROLL_VERTICAL_LEFT)?0x2A:0x29);
    TinyI2C.write(0x00);
    TinyI2C.write(startPage);
    TinyI2C.write(nbFrame);
    TinyI2C.write(endPage);
    TinyI2C.write(0x01); // scroll by 3 row
    TinyI2C.stop();
  }
}

//
// 10.2.3 Deactivate Scroll (2Eh)
//   This command stops the motion of scrolling. After sending 2Eh command to deactivate the scrolling action,
//   the ram data needs to be rewritten.
//
// 10.2.4 Activate Scroll (2Fh)
//   This command starts the motion of scrolling and should only be issued after the scroll setup parameters have
//   been defined by the scrolling setup commands :26h/27h/29h/2Ah . The setting in the last scrolling setup
//   command overwrites the setting in the previous scrolling setup commands.
//   The following actions are prohibited after the scrolling is activated
//     1. RAM access (Data write or read)
//     2. Changing the horizontal scroll setup parameters
//
void SSD1306Display::scroll(uint8_t mode) {
  if (mode == SSD1306_SCROLL_OFF) {
    send_command(SSD1306_DEACTIVATE_SCROLL);
  } else {
    send_command(SSD1306_ACTIVATE_SCROLL);
  }
}

void SSD1306Display::scrollOnce(uint8_t startPage, uint8_t endPage, uint8_t startCol, uint8_t endCol, uint8_t dir) 
{
  // Horizontal Scroll Setup
  if (dir == SSD1306_SCROLL_LEFT || dir == SSD1306_SCROLL_RIGHT) {
    TinyI2C.start(SSD1306_I2C_ADDRESS, 0);
    
    TinyI2C.write(0x00);
    TinyI2C.write((dir == SSD1306_SCROLL_LEFT)?0x2D:0x2C);
    TinyI2C.write(0x00);
    TinyI2C.write(startPage);
    TinyI2C.write(0x01);
    TinyI2C.write(endPage);
    TinyI2C.write(startCol);
    TinyI2C.write(endCol);
    TinyI2C.stop();
  }
}

// 10.1.7 Set Contrast Control for BANK0 (81h)
// This command sets the Contrast Setting of the display. The chip has 256 contrast steps from 00h to FFh. The
// segment output current increases as the contrast step value increases.
void SSD1306Display::contrast(uint8_t contrast) {
  send_command(SSD1306_SETCONTRAST);     // Contrast Control Mode Set
  send_command(contrast); // Contrast Data Register Set:
}

// 10.1.14 Set COM Output Scan Direction (C0h/C8h)
// This command sets the scan direction of the COM output, allowing layout flexibility in the OLED module
// design. Additionally, the display will show once this command is issued. For example, if this command is
// sent during normal display then the graphic display will be vertically flipped immediately. Please refer to
// Table 10-3 for details.
//
// 10.1.8
// Set Segment Re-map (A0h/A1h)
// This command changes the mapping between the display data column address and the segment driver. It
// allows flexibility in OLED module design. Please refer to Table 9-1.
// This command only affects subsequent data input. Data already stored in GDDRAM will have no changes.
void SSD1306Display::flip(uint8_t on) {
  if (on) {
    send_command(0xC8); // Common Output Scan Direction
    send_command(0xA1); // SEG REMAP
  } else {
    send_command(0xC0); // Common Output Scan Direction
    send_command(0xA0); // SEG REMAP
  }
}

// 10.1.10 Set Normal/Inverse Display (A6h/A7h)
// This command sets the display to be either normal or inverse. In normal display a RAM data of 1 indicates an
// “ON” pixel while in inverse display a RAM data of 0 indicates an “ON” pixel.
void SSD1306Display::invert(uint8_t on) { send_command(on ? 0xA7 : 0xA6); }

// SSD1306 display addressing
// Display is divided in pages each page has a height of 8 pixels

// Specifies column address of display RAM. Divide the column address into 4
// higher bits and 4 lower bits. Set each of them into successions. When the
// microprocessor repeats to access to the display RAM, the column address
// counter is incremented during each access until address 127 is accessed. The
// page address is not changed during this time.
#define SSD1306_SET_COLADD_LSB 0x00 // 1. Set Lower Column Address: (00H - 0FH)
#define SSD1306_SET_COLADD_MSB 0x10 // 2. Set Higher Column Address: (10H – 1FH)
// 10.1.13 Set Page Start Address for Page Addressing Mode (B0h~B7h)
// This command positions the page start address from 0 to 7 in GDDRAM under Page Addressing Mode.
// Please refer to Table 9-1 and Section 10.1.3 for details.
#define SSD1306_SET_PAGEADD 0xB0
void SSD1306Display::setAddress(uint8_t x, uint8_t y) {
  TinyI2C.start(SSD1306_I2C_ADDRESS, 0);
  TinyI2C.write(0x80);
  TinyI2C.write(SSD1306_SET_PAGEADD | (y / 8));
  TinyI2C.write(0x80);
  TinyI2C.write(SSD1306_SET_COLADD_LSB | (x & 0x0F));
  TinyI2C.write(0x00);
  TinyI2C.write(SSD1306_SET_COLADD_MSB | ((x & 0xF0) >> 4));
  TinyI2C.stop();
}

void SSD1306Display::drawScreen(uint8_t pattern, bool border) {
  for (uint8_t page = 0; page < (_height / 8); page++) {
    setAddress(0, page * 8);
    TinyI2C.start(SSD1306_I2C_ADDRESS, 0);
    for (uint8_t i = 0; i < _width; i++) {
      TinyI2C.write((i < (_width - 1)) ? 0xC0 : 0x40);
      if (border && i == 0) {
        TinyI2C.write(0xFF);
      } else if (border && i == (_width - 1)) {
        TinyI2C.write(0xFF);
      } else if (border && page == 0) {
        TinyI2C.write(pattern | 0x01);
      } else if (border && page == (_height / 8)-1) {
        TinyI2C.write(pattern | 0x80);
      } else {
        TinyI2C.write(pattern);
      }
    }
    TinyI2C.stop();
  }
}

void SSD1306Display::drawPage(uint8_t p, uint8_t pattern) {
  setAddress(0, p * 8);
  TinyI2C.start(SSD1306_I2C_ADDRESS, 0);
  for (uint8_t i = 0; i < _width; i++) {
    TinyI2C.write((i < (_width - 1)) ? 0xC0 : 0x40);
    TinyI2C.write(pattern);
  }
  TinyI2C.stop();
}

void SSD1306Display::drawPage(uint8_t p, uint8_t startcol, uint8_t nbcol,
                             uint8_t pattern) {
  setAddress(startcol, p * 8);
  TinyI2C.start(SSD1306_I2C_ADDRESS, 0);
  for (uint8_t i = 0; i < nbcol; i++) {
    TinyI2C.write((i < (nbcol - 1)) ? 0xC0 : 0x40);
    TinyI2C.write(pattern);
  }
  TinyI2C.stop();
}

uint8_t SSD1306Display::updatePagePixel(uint8_t y, uint8_t colour) {
  if (y >= this->_height) {
    return 0;
  }

  // request data
  TinyI2C.start(SSD1306_I2C_ADDRESS, 0);
  TinyI2C.write(0x40); // say we read Data RAM and not status register
  TinyI2C.stop();

  TinyI2C.start(SSD1306_I2C_ADDRESS, 2);
  uint8_t r = TinyI2C.read(); // dummy bit start read response
  r = TinyI2C.read();
  TinyI2C.stop();

  uint8_t prev = r & (1 << (y % 8));

  // updat pix
  switch (colour) {
  case SSD1306_WHITE_COLOR:
    r |= (1 << (y % 8));
    break;
  case SSD1306_BLACK_COLOR:
    r &= ~(1 << (y % 8));
    break;
  case SSD1306_INVERSE_COLOR:
    r ^= (1 << (y % 8));
    break;
  }

  // write new value
  TinyI2C.start(SSD1306_I2C_ADDRESS, 0);
  TinyI2C.write(0x40);
  TinyI2C.write(r);
  TinyI2C.stop();
  // return previous
  return prev;
}

uint8_t SSD1306Display::updatePageColumn(uint8_t pattern, uint8_t mode,
                                        uint8_t mask) {
  // request data
  TinyI2C.start(SSD1306_I2C_ADDRESS, 0);
  TinyI2C.write(0xC0); // say we read Data RAM and not status register
  TinyI2C.stop();

  TinyI2C.start(SSD1306_I2C_ADDRESS, 2);
  uint8_t r = TinyI2C.read(); // dummy bit start read response
  r = TinyI2C.read();
  TinyI2C.stop();

  uint8_t prev = r & mask;

  // update column
  pattern &= mask; // keep only masked part
  if (mode == OVERWRITE_MODE) {
    r &= (~mask);    // clear masked area
    r = r | pattern; // update masked part
  } else if (mode == OR_MODE) {
    r = r | pattern;
  } else if (mode == XOR_MODE) {
    r = r ^ pattern;
  } else if (mode == AND_MODE) {
    pattern |= (~mask);
    r = r & pattern;
  }

  // write new value
  TinyI2C.start(SSD1306_I2C_ADDRESS, 0);
  TinyI2C.write(0x40);
  TinyI2C.write(r);
  TinyI2C.stop();

  // return previous
  return prev;
}

/*
  Draw Pixel does not work without frame buffer because it is not possible to read GDRAM

  TODO: add framebuffer support
*/
void SSD1306Display::drawPixel(uint8_t x, uint8_t y, uint8_t colour) {
  if ((x >= this->_width) || (y >= this->_height)) {
    return;
  }

  setAddress(x, y);
  int r = 0;

  TinyI2C.start(SSD1306_I2C_ADDRESS, 0);
  // request data
  TinyI2C.write(0x40); // say we read Data RAM and not status register

  TinyI2C.start(SSD1306_I2C_ADDRESS, 2);
  r = TinyI2C.read(); // dummy bit start read response
  #ifdef HAS_SERIAL
  USART_WriteString("R ");
  USART_WriteInt(r);
  USART_WriteString(" ");
  #endif

  r = TinyI2C.read();

  #ifdef HAS_SERIAL
  USART_WriteInt(r);
  USART_WriteString("\n");
  TinyI2C.stop();
  #endif

  r = 0;
  switch (colour) {
  case SSD1306_WHITE_COLOR:
    r |= (1 << (y % 8));
    break;
  case SSD1306_BLACK_COLOR:
    r &= ~(1 << (y % 8));
    break;
  case SSD1306_INVERSE_COLOR:
    r ^= (1 << (y % 8));
    break;
  }

  TinyI2C.start(SSD1306_I2C_ADDRESS, 0);
  TinyI2C.write(0x40);
  TinyI2C.write(r);

  TinyI2C.stop();
}

void SSD1306Display::drawLine(uint8_t x0, uint8_t y0, uint8_t x1, uint8_t y1, uint8_t color) {
  if (y0 == y1) {
    if (x0 > x1) {
      SSD1306_Swap(x0, x1);
    }
    return drawHLine(x0, x1, y0, color);
  }
  if (x0 == x1) {
    if (y0 > y1) {
      SSD1306_Swap(y0, y1);
    }
    return drawVLine(x0, y0, y1, color);
  }

  uint8_t steep = abs(y1 - y0) > abs(x1 - x0);

  if (steep) {
    SSD1306_Swap(x0, y0);
    SSD1306_Swap(x1, y1);
  }

  if (x0 > x1) {
    SSD1306_Swap(x0, x1);
    SSD1306_Swap(y0, y1);
  }

  uint8_t dx, dy;
  dx = x1 - x0;
  dy = abs(y1 - y0);

  int8_t err = dx / 2;
  int8_t ystep = (y0 < y1) ? 1 : -1;

  if (steep) {
    // in this mode x is line addressing and y column
    uint8_t prevpage = x0 / 8;
    if (ystep == -1) {
      y0 -= 4;
    }
    this->setAddress(y0, x0);
    uint8_t pattern = 0x00;
    for (; x0 <= x1; x0++) {
      uint8_t curpage = x0 / 8;
      if (curpage != prevpage) {
        // flush
        this->updatePageColumn(pattern, 1);
        // change page
        prevpage = x0 / 8;
        this->setAddress(y0, x0);
        pattern = 0x00;
      }

      if (ystep == -1) {
        pattern |= (1 << (7 - (x0 % 8)));
      } else {
        pattern |= (1 << (x0 % 8));
      }

      err -= dy;
      if (err < 0) {
        this->updatePageColumn(pattern, 1);
        y0 += ystep;
        pattern = 0x00;
        err += dx;
      }
    }
  } else {
    uint8_t prevpage = y0 / 8;
    this->setAddress(x0, y0);
    for (; x0 <= x1; x0++) {
      uint8_t curpage = y0 / 8;
      if (curpage != prevpage) {
        // change page
        prevpage = y0 / 8;
        this->setAddress(x0, y0);
      }
      this->updatePagePixel(y0, color);

      err -= dy;
      if (err < 0) {
        y0 += ystep;
        err += dx;
      }
    }
  }
}

void SSD1306Display::drawHLine(uint8_t x0, uint8_t x1, uint8_t y,
                              uint8_t color) {
  this->setAddress(x0, y);
  for (uint8_t i = 0; i < (x1 - x0 + 1); i++) {
    this->updatePagePixel(y, color);
  }
}

void SSD1306Display::drawVLine(uint8_t x, uint8_t y0, uint8_t y1,
                              uint8_t color) {
  for (uint8_t y = y0; y <= y1; y++) {
    this->drawPixel(x, y, color);
  }
}

void SSD1306Display::drawString2(uint8_t x, uint8_t y, const char *pText) {
  setAddress(x, y);

  while (*pText != '\0') {
    if ((x + FONT_CHAR_WIDTH + 1) > this->_width) {
      break;
    }
    // draw
    char c = pText[0];

    uint8_t line = pgm_read_byte(small_font + ((c - 32) * FONT_CHAR_WIDTH));
    this->start_data(line);
    x++;
    for (int8_t i = 1; i < FONT_CHAR_WIDTH; i++) {
      line = pgm_read_byte(small_font + ((c - 32) * FONT_CHAR_WIDTH) + i);
      send_data(line);
      x++;
    }

    // draw empty vert line to separate char
    this->stop_data(0x00);
    x++;

    pText++;
  }
}

void SSD1306Display::drawString(uint8_t x, uint8_t y, const char *pText) {
  if (y + FONT_CHAR_HEIGHT > this->_height) {
    return;
  }

  uint8_t page_offset = y % 8;
  uint8_t mask = (page_offset) ? (0xFF << page_offset) : 0xFF;
  const char *startText = pText;
  this->setAddress(x, y);
  while (*pText != '\0') {
    if ((x + FONT_CHAR_WIDTH + 1) > this->_width) {
      break;
    }
    // draw
    char c = pText[0];

    uint8_t line = pgm_read_byte(small_font + ((c - 32) * FONT_CHAR_WIDTH));
    if (page_offset) {
      line = (line << page_offset);
    }
    this->updatePageColumn(line, 0, mask);

    for (int8_t i = 1; i < FONT_CHAR_WIDTH; i++) {
      line = pgm_read_byte(small_font + ((c - 32) * FONT_CHAR_WIDTH) + i);
      if (page_offset) {
        line = (line << page_offset);
      }
      this->updatePageColumn(line, 0, mask);
    }

    // draw empty vert line to separate char
    this->updatePageColumn(0x00, 0, mask);

    pText++;
  }

  if (page_offset) {
    // draw second part
    mask = (0xFF >> (8 - page_offset));
    this->setAddress(x, y + 8);
    while (*startText != '\0') {
      if ((x + FONT_CHAR_WIDTH + 1) > this->_width) {
        break;
      }
      // draw
      char c = startText[0];

      uint8_t line = pgm_read_byte(small_font + ((c - 32) * FONT_CHAR_WIDTH));
      line = (line >> (8 - page_offset));
      this->updatePageColumn(line, 0, mask);

      for (int8_t i = 1; i < FONT_CHAR_WIDTH; i++) {
        line = pgm_read_byte(small_font + ((c - 32) * FONT_CHAR_WIDTH) + i);
        line = (line >> (8 - page_offset));
        this->updatePageColumn(line, 0, mask);
      }

      // draw empty vert line to separate char
      this->updatePageColumn(0x00, 0, mask);

      startText++;
    }
  }
}

void SSD1306Display::drawSprite(uint8_t x, uint8_t y, uint8_t sw, uint8_t sh,
                               const uint8_t *data, uint8_t mode) {
  if (y + sh > this->_height) {
    return;
  }

  uint8_t page_offset = y % 8;
  uint8_t mask = (page_offset) ? (0xFF << page_offset) : 0xFF;

  this->setAddress(x, y);
  for (uint8_t tx = 0; tx < sw; tx++) {
    if (x + tx >= this->_width) {
      continue;
    }
    uint8_t pattern = pgm_read_byte(&data[tx]);
    if (page_offset) {
      pattern = (pattern << page_offset);
    }
    this->updatePageColumn(pattern, mode, mask);
  }

  if (page_offset) {
    // draw second part
    mask = (0xFF >> (8 - page_offset));
    this->setAddress(x, y + 8);
    for (uint8_t tx = 0; tx < sw; tx++) {
      if (x + tx >= this->_width) {
        continue;
      }
      uint8_t pattern = pgm_read_byte(&data[tx]);
      pattern = (pattern >> (8 - page_offset));
      this->updatePageColumn(pattern, mode, mask);
    }
  }
}

void SSD1306Display::send_command(uint8_t command) {
  TinyI2C.start(SSD1306_I2C_ADDRESS, 0);
  TinyI2C.write(0x00);
  TinyI2C.write(command);
  TinyI2C.stop();
}

void SSD1306Display::send_data(uint8_t byte) {
  TinyI2C.start(SSD1306_I2C_ADDRESS, 0);
  TinyI2C.write(0x40);
  TinyI2C.write(byte);
  TinyI2C.stop();
}

void SSD1306Display::start_data(uint8_t byte) {
  TinyI2C.start(SSD1306_I2C_ADDRESS, 0);
  TinyI2C.write(0xC0);
  TinyI2C.write(byte);
}

void SSD1306Display::add_data(uint8_t byte) {
  TinyI2C.write(0xC0);
  TinyI2C.write(byte);
}

void SSD1306Display::stop_data(uint8_t byte) {
  TinyI2C.start(SSD1306_I2C_ADDRESS, 0);
  TinyI2C.write(0x40);
  TinyI2C.write(byte);
  TinyI2C.stop();
}
