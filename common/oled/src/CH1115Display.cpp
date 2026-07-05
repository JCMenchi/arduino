
/**
 * @file CH1115Display.cpp
 * @brief Implementation of CH1115 OLED display driver
 * 
 * Provides I2C-based control for CH1115 OLED display controllers.
 * Includes graphics primitives, text rendering, and display effects.
 */

#include <avr/pgmspace.h>
#include <stdlib.h>

#include "CH1115Display.h"
#include "TinyI2CMaster.h"
#include "bitmap_font.h"


#ifdef HAS_SERIAL
#include <usart_serial.h>
#endif

/**
 * @brief Utility macro to swap two uint8_t values
 * @param a First value
 * @param b Second value
 */
#define CH1115_Swap(a, b)                                                      \
  {                                                                            \
    uint8_t t = a;                                                             \
    a = b;                                                                     \
    b = t;                                                                     \
  }

/**
 * @brief Constructor - Initialize display dimensions
 * Stores display width and height for coordinate calculations
 */
CH1115Display::CH1115Display(uint8_t w, uint8_t h) {
  _width = w;
  _height = h;
}

/**
 * @brief Destructor - Cleanup (currently no resources to release)
 */
CH1115Display::~CH1115Display() {
}

#define CH1115_STATUS 0x15  ///< Status register address

/**
 * @brief Initialize I2C interface and display controller
 * 
 * Sets up I2C communication, verifies device presence, and configures
 * the display with the specified contrast level. Sends initialization
 * commands per CH1115 datasheet.
 * 
 * @note I2C Error codes (from TinyI2C):
 *  - 0: success
 *  - 1: length too long for buffer
 *  - 2: address send, NACK received
 *  - 3: data send, NACK received
 *  - 4: other TWI error (lost arbitration, bus error)
 *  - 5: timeout
 */
void CH1115Display::init(uint8_t contrast) {
  // Init I2C com
  TinyI2C.init();

  // Read one bit from device
  bool con = TinyI2C.start(CH1115_I2C_ADDRESS, 1);
  if (!con) {
#if defined(HAS_SERIAL) && defined(OLED_DEBUG)
    USART_WriteString("I2C connect error\n");
#endif
    return;
  }

  // Check display type
  uint8_t u = TinyI2C.read();
  TinyI2C.stop();

#if defined(HAS_SERIAL) && defined(OLED_DEBUG)
  USART_WriteString("Screen status register: 0x");
  USART_WriteInt(u, 16);

  uint8_t id = u & 0x3F;
  USART_WriteString(" Device ID: ");
  USART_WriteInt(id, 2);
#endif

  uint8_t on = u & 0x40;
  if (on == 0) {
#if defined(HAS_SERIAL) && defined(OLED_DEBUG)
    USART_WriteString(" is ON\n");
#endif
  } else {
#if defined(HAS_SERIAL) && defined(OLED_DEBUG)
    USART_WriteString(" is OFF\n");
#endif
    // turn on
    send_command(0xAF);
  }

  // set normal display mode
  send_command(0xA4);
  // set contrast
  this->contrast(contrast);
}

/**
 * @brief Enable or disable display output
 * 
 * Sends command 0xAF to turn ON or 0xAE to turn OFF.
 * When OFF, enters power-saving sleep mode while preserving RAM contents.
 * 
 * Per CH1115 Datasheet Section 18 (Display OFF/ON):
 *  - OFF (0xAE): Stops oscillator and DC-DC circuits, reduces current consumption
 *  - ON (0xAF): Normal operation mode
 */
void CH1115Display::enable(uint8_t on) { send_command(on ? 0xAF : 0xAE); }

/**
 * @brief Configure the scrolling area and parameters
 * 
 * Sets up horizontal scroll region and timing per CH1115 datasheet.
 * Configuration must be completed before calling scroll() to activate.
 * 
 * Per CH1115 Datasheet Sections 4-5:
 *  - Command 0x24: Set column range (start to end)
 *  - Command 0x26-0x27: Scroll direction (0x26=right, 0x27=left)
 *  - Followed by: startPage, timeInterval, endPage
 * 
 * @implementation Sends command sequence:
 *  1. 0x24 (Additional H-scroll setup)
 *       | D7 | D6 | D5 | D4 | D3 | D2 | D1 | D0 |
 *       |  0 |  0 |  1 |  0 |  0 |  1 |  0 |  0 | -> 0x24
 *       | A7 | A6 | A5 | A4 | A3 | A2 | A1 | A0 | -> start colum 0 to 127
 *       | B7 | B6 | B5 | B4 | B3 | B2 | B1 | B0 | -> end column  0 to 127
 *
 * 5. Horizontal Scroll Setup: (Four Bytes Command)
 * This command consists of 4 consecutive bytes to set up the horizontal scroll
 * parameters. It determined the number of horizontal scroll per step ,
 * scrolling start page, time interval and end page. Before issuing this
 * command, the horizontal scroll must be deactivated (2EH). Otherwise, ram
 * content may be corrupted.
 *
 * - Horizontal Scroll Setup Mode Set: (26H - 27H)
 *     | D7 | D6 | D5 | D4 | D3 | D2 | D1 | D0 |
 *     |  0 |  0 |  1 |  0 |  0 | 1  |  1 |  D | D is direction 0 -> right, 1 ->
 *     left |  * |  * |  * |  * |  * | A2 | A1 | A0 | start page address 0 to 7
 *     |  * |  * |  * |  * |  * | B2 | B1 | B0 | time interval number of frame
 *     |  * |  * |  * |  * |  * | C2 | C1 | C0 | end page address   0 to 7
 *
 *           000   6 frames(POR)
 *           001  32 frames
 *           010  64 frames
 *           011 128 frames
 *           100   3 frames
 *           101   4 frames
 *           110   5 frames
 *           111   2 frames
 */

void CH1115Display::scrollArea(uint8_t startPage, uint8_t endPage,
                               uint8_t startCol, uint8_t endCol, uint8_t dir,
                               uint8_t nbFrame) {
  // Additional Horizontal Scroll Setup
  send_command(0x24);
  send_command(startCol ? startCol : 0);
  send_command(endCol ? endCol : 127);

  // Horizontal Scroll Setup
  if (dir == CH1115_SCROLL_LEFT) {
    send_command(0x27);
  } else {
    send_command(0x26);
  }
  send_command(startPage < 8 ? startPage : 0);
  send_command(nbFrame);
  send_command(endPage < 8 ? endPage : 7);
}

// 6. Set Scroll Mode: (28H – 2BH)
// Control continuous or single screen scroll.
//         | D7 | D6 | D5 | D4 | D3 | D2 | D1 | D0 |
//         |  0 |  0 |  1 |  0 |  1 |  0 | D1 | D0 |
//   Scroll mode defined by D1D0
//      00 -> Continuous horizontal/vertical scroll(default)
//      01 -> Single Screen scroll
//      1X -> 1 Column scroll mode
//  Single column scroll mode
// The display scroll one column after the 2BH+2FH commands are written.
// The scroll is end after the 2EH command is written. 0x2B2F2E scroll one
// column
//
// 7. Set Deactivate /Activate Horizontal Scroll: (2EH - 2FH)
// Stop or start motion of horizontal scrolling. This command should only be
// issued after horizontal scroll setup parameters (24H/26H/27H/28H/29H/2CH/2DH)
// are defined.
//
// When D(bit0)=”L”, Stop motion of horizontal scroll. (POR) 0x2E
// When D(bit0)=”H”, Start motion of horizontal scroll. 0x2F
// Note: The following actions are prohibited after the horizontal scroll is
// activated  Changing additional horizontal scroll setup parameters. 
// Changing horizontal scroll setup parameters.  Changing scroll mode setup
// parameters. After the deactivate horizontal scroll issued, the display of
// screen is reset to original status.

/**
 * @brief Start, stop, or configure scrolling mode
 * 
 * Activates scrolling in the region previously configured by scrollArea().
 * Commands sent: 0x2E (stop), 0x28+0x2F (continuous), 0x29+0x2F (once), 0x2A+0x2F (column)
 */
void CH1115Display::scroll(uint8_t mode) {
  if (mode == CH1115_SCROLL_OFF) {
    send_command(0x2E);
  } else if (mode == CH1115_SCROLL_CONTINUOUS) {
    send_command(0x28);
    send_command(0x2F);
  } else if (mode == CH1115_SCROLL_ONCE) {
    send_command(0x29);
    send_command(0x2F);
  } else if (mode == CH1115_SCROLL_ONE_COLUMN) {
    send_command(0x2A);
    send_command(0x2F);
  }
}

// 10. Set Contrast Control Register: (Double Bytes Command)
// This command is to set contrast setting of the display. The chip has 256
// contrast steps from 00 to FF. The segment output current increases as the
// contrast step value increases. Segment output current setting: ISEG =
// (α+1)/256 X IREF X scale factor Where: α is contrast step; Scale factor = 16.
//
// - The Contrast Control Mode Set: (81H)
// When this command is input, the contrast data register set command becomes
// enabled. Once the contrast control mode has been set, no other command except
// for the contrast data register command can be used. Once the contrast data
// set command has been used to set data into the register, then the contrast
// control mode is released.
//
// - Contrast Data Register Set: (00H – FFH)
// By using this command to set eight bits of data to the contrast data
// register; the OLED segment output assumes one of the 256 current levels. When
// this command is input, the contrast control mode is released after the
// contrast data register has been set.// When the contrast control function is
// not used, set the D7 - D0 to 10000000.

/**
 * @brief Set display contrast level
 * Adjusts segment output current (256 levels: 0x00-0xFF)
 */
void CH1115Display::contrast(uint8_t contrast) {
  send_command(0x81);     // Contrast Control Mode Set
  send_command(contrast); // Contrast Data Register Set:
}

// 20. Set Common Output Scan Direction: (C0H - C8H)
// This command sets the scan direction of the common output allowing layout
// flexibility in OLED module design. In addition, the display will have
// immediate effect once this command is issued. That is, if this command is
// sent during normal display, the graphic display will be vertically flipped.
// When D (bit3) = “L”, Scan from COM0 to COM [N -1]. (POR)
// When D (bit3) = “H”, Scan from COM [N -1] to COM0.
//
// 12. Set Segment Re-map: (A0H - A1H)
// Change the relationship between RAM column address and segment driver. The
// order of segment driver output pads can be reversed by software. This allows
// flexible IC layout during OLED module assembly. For details, refer to the
// column address section of Figure. 18. When display data is written or read,
// the column address is incremented by 1 as shown in Figure.18.
//
// When ADC (bit0) = “L”, the right rotates (normal direction). (POR)
// When ADC (bit0) = “H”, the left rotates (reverse direction).
/**
 * @brief Flip display vertically and horizontally
 * Rotates display 180 degrees by remapping COM scan and SEG output
 */void CH1115Display::flip(uint8_t on) {
  if (on) {
    send_command(0xC8); // Common Output Scan Direction
    send_command(0xA1); // SEG REMAP
  } else {
    send_command(0xC0); // Common Output Scan Direction
    send_command(0xA0); // SEG REMAP
  }
}

/**
 * @brief Enable or disable breathing (pulsing) display effect
 * 
 * Creates a fading in/out visual effect by modulating brightness.
 * Per CH1115 Datasheet Section 3:
 *  - Command 0x23: Breathing Light Set
 *  - Parameter 0x82 (ON): Max brightness 256, 3-frame interval
 *  - Parameter 0x00 (OFF): Breathing disabled
 */
void CH1115Display::breathingEffect(uint8_t on) {
  send_command(0x23);
  // when ON, maxBrightness=256, 3 frames
  send_command(on ? 0x82 : 0x00);
}

/**
 * @brief Invert display colors (ON pixels become OFF and vice versa)
 * 
 * Reverses display output without modifying RAM contents.
 * Per CH1115 Datasheet Section 15:
 *  - 0xA6: Normal mode (RAM high = pixel ON)
 *  - 0xA7: Reverse mode (RAM high = pixel OFF)
 */
void CH1115Display::invert(uint8_t on) { send_command(on ? 0xA7 : 0xA6); }

/**
 * @brief Set I2C RAM address for drawing operations
 * 
 * Display uses page-based addressing: 8 pages of 8 pixels height each.
 * Configures both page (y) and column (x) addresses via I2C.
 * 
 * Addressing scheme per CH1115 Datasheet:
 *  - Column: Split into LSB (0x00-0x0F) and MSB (0x10-0x1F) commands
 *  - Page: Single command 0xB0-0xB7 for pages 0-7
 * 
 * @implementation Sends 6 I2C bytes:
 *  1. 0x80 (data control)
 *  2. 0xB0 | (y/8) (page address)
 *  3. 0x80
 *  4. 0x00 | (x & 0x0F) (column LSB)
 *  5. 0x00
 *  6. 0x10 | ((x & 0xF0)>>4) (column MSB)
 */
#define CH1115_SET_COLADD_LSB 0x00 ///< Lower column address command base
#define CH1115_SET_COLADD_MSB 0x10 ///< Upper column address command base
#define CH1115_SET_PAGEADD 0xB0    ///< Page address command base (0xB0-0xB7)

void CH1115Display::setAddress(uint8_t x, uint8_t y) {
  TinyI2C.start(CH1115_I2C_ADDRESS, 0);
  TinyI2C.write(0x80);
  TinyI2C.write(CH1115_SET_PAGEADD | (y / 8));
  TinyI2C.write(0x80);
  TinyI2C.write(CH1115_SET_COLADD_LSB | (x & 0x0F));
  TinyI2C.write(0x00);
  TinyI2C.write(CH1115_SET_COLADD_MSB | ((x & 0xF0) >> 4));
  TinyI2C.stop();
}

/**
 * @brief Fill entire screen with pattern
 * 
 * Fills all display RAM with the specified pattern byte.
 * Optionally draws a border frame.
 * 
 * @implementation Iterates through each page (height/8) and fills
 * all columns with the pattern. Border mode sets first/last columns
 * and first/last bits of top/bottom pages to 0xFF.
 */
void CH1115Display::drawScreen(uint8_t pattern, bool border) {
  for (uint8_t page = 0; page < (_height / 8); page++) {
    setAddress(0, page * 8);
    TinyI2C.start(CH1115_I2C_ADDRESS, 0);
    for (uint8_t i = 0; i < _width; i++) {
      TinyI2C.write((i < (_width - 1)) ? 0xC0 : 0x40);
      if (border && i == 0) {
        TinyI2C.write(0xFF);
      } else if (border && i == (_width - 1)) {
        TinyI2C.write(0xFF);
      } else if (border && page == 0) {
        TinyI2C.write(pattern | 0x01);
      } else if (border && page == (_height / 8 - 1)) {
        TinyI2C.write(pattern | 0x80);
      } else {
        TinyI2C.write(pattern);
      }
    }
    TinyI2C.stop();
  }
}

/**
 * @brief Fill single page (8-pixel row) with pattern
 * 
 * Fills one complete page (all columns) with pattern byte.
 * Page height is 8 pixels; y-coordinate will be rounded to page boundary.
 */
void CH1115Display::drawPage(uint8_t p, uint8_t pattern) {
  setAddress(0, p * 8);
  TinyI2C.start(CH1115_I2C_ADDRESS, 0);
  for (uint8_t i = 0; i < _width; i++) {
    TinyI2C.write((i < (_width - 1)) ? 0xC0 : 0x40);
    TinyI2C.write(pattern);
  }
  TinyI2C.stop();
}

/**
 * @brief Fill portion of a page with pattern
 * 
 * Fills a partial page region (columns startcol to startcol+nbcol-1)
 * with the specified pattern byte.
 */
void CH1115Display::drawPage(uint8_t p, uint8_t startcol, uint8_t nbcol,
                             uint8_t pattern) {
  setAddress(startcol, p * 8);
  TinyI2C.start(CH1115_I2C_ADDRESS, 0);
  for (uint8_t i = 0; i < nbcol; i++) {
    TinyI2C.write((i < (nbcol - 1)) ? 0xC0 : 0x40);
    TinyI2C.write(pattern);
  }
  TinyI2C.stop();
}

/**
 * @brief Begin page-based pixel drawing sequence
 * 
 * Initiates Read-Modify-Write mode (command 0xE0) for efficient pixel updates.
 * Must be paired with endPageDrawing().
 * 
 * Per CH1115 Datasheet Section 27 (Read-Modify-Write):
 *  - Column address auto-increments on write but not on read
 *  - Allows multiple pixel updates without re-addressing
 *  - Column returns to initial address when End (0xEE) issued
 * 
 * @note Must call endPageDrawing() after all pixel updates complete.
 */
void CH1115Display::startPageDrawing(uint8_t x, uint8_t y) {
  setAddress(x, y);

  TinyI2C.start(CH1115_I2C_ADDRESS, 0);
  // start read modify write
  TinyI2C.write(0x80);
  TinyI2C.write(0xE0);
}

/**
 * @brief Update a single pixel within current page drawing operation
 * 
 * Reads current column byte, modifies single pixel bit, and writes back.
 * Uses Read-Modify-Write mode to minimize I2C transfers.
 * 
 * Pixel operations:
 *  - WHITE_COLOR: Set bit (pixel ON)
 *  - BLACK_COLOR: Clear bit (pixel OFF)
 *  - INVERSE_COLOR: Toggle bit
 * 
 * @return Previous pixel state (bit value)
 */
uint8_t CH1115Display::updatePagePixel(uint8_t y, uint8_t colour) {
  if (y >= this->_height) {
    return 0;
  }

  // request data
  TinyI2C.start(CH1115_I2C_ADDRESS, 0);
  TinyI2C.write(0x40); // say we read Data RAM and not status register

  TinyI2C.restart(CH1115_I2C_ADDRESS, 2);
  uint8_t r = TinyI2C.read(); // dummy bit start read response
  r = TinyI2C.read();

  uint8_t prev = r & (1 << (y % 8));

  // updat pix
  switch (colour) {
  case CH1115_WHITE_COLOR:
    r |= (1 << (y % 8));
    break;
  case CH1115_BLACK_COLOR:
    r &= ~(1 << (y % 8));
    break;
  case CH1115_INVERSE_COLOR:
    r ^= (1 << (y % 8));
    break;
  }

  // write new value
  TinyI2C.restart(CH1115_I2C_ADDRESS, 0);
  TinyI2C.write(0xC0);
  TinyI2C.write(r);

  // return previous
  return prev;
}

/**
 * @brief Update entire column (byte) in current page drawing operation
 * 
 * Reads current column byte, applies drawing mode with mask, writes back.
 * More efficient than pixel-by-pixel updates.
 * 
 * Drawing modes:
 *  - OVERWRITE_MODE: Replace masked bits with pattern
 *  - OR_MODE: Bitwise OR masked pattern
 *  - XOR_MODE: Bitwise XOR masked pattern
 *  - AND_MODE: Bitwise AND with (pattern | ~mask)
 * 
 * @param pattern Pixel pattern byte to apply
 * @param mode Drawing mode (OVERWRITE, OR, XOR, AND)
 * @param mask Bit mask (0xFF = all bits, 0x0F = lower 4 bits only)
 * @return Previous column value (masked portion)
 */
uint8_t CH1115Display::updatePageColumn(uint8_t pattern, uint8_t mode,
                                        uint8_t mask) {
  // request data
  TinyI2C.start(CH1115_I2C_ADDRESS, 0);
  TinyI2C.write(0x40); // say we read Data RAM and not status register

  TinyI2C.restart(CH1115_I2C_ADDRESS, 2);
  uint8_t r = TinyI2C.read(); // dummy bit start read response
  r = TinyI2C.read();

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
  TinyI2C.restart(CH1115_I2C_ADDRESS, 0);
  TinyI2C.write(0xC0);
  TinyI2C.write(r);

  // return previous
  return prev;
}

/**
 * @brief End page-based pixel drawing sequence
 * 
 * Terminates Read-Modify-Write mode (command 0xEE) and restores
 * column address to value when startPageDrawing() was called.
 * Per CH1115 Datasheet Section 28.
 */
void CH1115Display::endPageDrawing() {
  TinyI2C.restart(CH1115_I2C_ADDRESS, 0);
  // stop read modify write
  TinyI2C.write(0x00);
  TinyI2C.write(0xEE);
  TinyI2C.stop();
}

/**
 * @brief Draw a single pixel
 * 
 * Sets or clears a single pixel at (x, y) coordinate.
 * Uses Read-Modify-Write mode for efficient I2C communication.
 */
void CH1115Display::drawPixel(uint8_t x, uint8_t y, uint8_t colour) {
  if ((x >= this->_width) || (y >= this->_height)) {
    return;
  }

  setAddress(x, y);
  int r = 0;

  TinyI2C.start(CH1115_I2C_ADDRESS, 0);
  // start read modify write
  TinyI2C.write(0x80);
  TinyI2C.write(0xE0);

  // request data
  TinyI2C.write(0x40); // say we read Data RAM and not status register
  TinyI2C.restart(CH1115_I2C_ADDRESS, 2);
  r = TinyI2C.read(); // dummy bit start read response
  r = TinyI2C.read();

  switch (colour) {
  case CH1115_WHITE_COLOR:
    r |= (1 << (y % 8));
    break;
  case CH1115_BLACK_COLOR:
    r &= ~(1 << (y % 8));
    break;
  case CH1115_INVERSE_COLOR:
    r ^= (1 << (y % 8));
    break;
  }

  TinyI2C.restart(CH1115_I2C_ADDRESS, 0);
  TinyI2C.write(0xC0);
  TinyI2C.write(r);
  // stop read modify write
  TinyI2C.write(0x00);
  TinyI2C.write(0xEE);
  TinyI2C.stop();
}

/**
 * @brief Draw a line between two points
 * 
 * Implements Bresenham's line algorithm for efficient rasterization.
 * Detects horizontal, vertical, and diagonal lines for optimization.
 * 
 * Algorithm handles two cases:
 *  1. Steep lines (|dy| > |dx|): Iterates through y, updates x
 *  2. Shallow lines (|dy| <= |dx|): Iterates through x, updates y
 * 
 * For steep lines, uses page-based drawing for efficiency.
 * For shallow lines, uses per-pixel updates.
 */
void CH1115Display::drawLine(uint8_t x0, uint8_t y0, uint8_t x1, uint8_t y1,
                             uint8_t color) {
  if (y0 == y1) {
    if (x0 > x1) {
      CH1115_Swap(x0, x1);
    }
    return drawHLine(x0, x1, y0, color);
  }
  if (x0 == x1) {
    if (y0 > y1) {
      CH1115_Swap(y0, y1);
    }
    return drawVLine(x0, y0, y1, color);
  }

  uint8_t steep = abs(y1 - y0) > abs(x1 - x0);

  if (steep) {
    CH1115_Swap(x0, y0);
    CH1115_Swap(x1, y1);
  }

  if (x0 > x1) {
    CH1115_Swap(x0, x1);
    CH1115_Swap(y0, y1);
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
    this->startPageDrawing(y0, x0);
    uint8_t pattern = 0x00;
    for (; x0 <= x1; x0++) {
      uint8_t curpage = x0 / 8;
      if (curpage != prevpage) {
        // flush
        this->updatePageColumn(pattern, 1);
        // change page
        this->endPageDrawing();
        prevpage = x0 / 8;
        this->startPageDrawing(y0, x0);
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
    this->endPageDrawing();
  } else {
    uint8_t prevpage = y0 / 8;
    this->startPageDrawing(x0, y0);
    for (; x0 <= x1; x0++) {
      uint8_t curpage = y0 / 8;
      if (curpage != prevpage) {
        // change page
        this->endPageDrawing();
        prevpage = y0 / 8;
        this->startPageDrawing(x0, y0);
      }
      this->updatePagePixel(y0, color);

      err -= dy;
      if (err < 0) {
        y0 += ystep;
        err += dx;
      }
    }
    this->endPageDrawing();
  }
}

/**
 * @brief Draw horizontal line (internal)
 * 
 * Draws a horizontal line from (x0,y) to (x1,y).
 * Used by drawLine() for horizontal line optimization.
 */
void CH1115Display::drawHLine(uint8_t x0, uint8_t x1, uint8_t y,
                              uint8_t color) {
  this->startPageDrawing(x0, y);
  for (uint8_t i = 0; i < (x1 - x0 + 1); i++) {
    this->updatePagePixel(y, color);
  }
  this->endPageDrawing();
}

/**
 * @brief Draw vertical line (internal)
 * 
 * Draws a vertical line from (x,y0) to (x,y1).
 * Used by drawLine() for vertical line optimization.
 */
void CH1115Display::drawVLine(uint8_t x, uint8_t y0, uint8_t y1,
                              uint8_t color) {
  for (uint8_t y = y0; y <= y1; y++) {
    this->drawPixel(x, y, color);
  }
}

/**
 * @brief Draw text string using large (2x) font (internal)
 * 
 * Renders text at 2x magnification using horizontal scrolling.
 * Only used internally within CH1115Display implementation.
 */
void CH1115Display::drawString2(uint8_t x, uint8_t y, const char *pText) {
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

/**
 * @brief Draw text string using standard font
 * 
 * Renders null-terminated string at (x, y) using 5-pixel-wide characters.
 * Characters are stored in PROGMEM (program memory).
 * 
 * @implementation
 *  - Handles page-crossing for characters spanning multiple pages
 *  - First part (upper) uses mask from page_offset to 0xFF
 *  - Second part (lower) uses mask from 0x00 to page_offset
 *  - Uses updatePageColumn() with OVERWRITE_MODE for clean rendering
 */
void CH1115Display::drawString(uint8_t x, uint8_t y, const char *pText) {
  if (y + FONT_CHAR_HEIGHT > this->_height) {
    return;
  }

  uint8_t page_offset = y % 8;
  uint8_t mask = (page_offset) ? (0xFF << page_offset) : 0xFF;
  const char *startText = pText;
  this->startPageDrawing(x, y);
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
  this->endPageDrawing();

  if (page_offset) {
    // draw second part
    mask = (0xFF >> (8 - page_offset));
    this->startPageDrawing(x, y + 8);
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
    this->endPageDrawing();
  }
}

/// @brief Temporary buffer for integer-to-string conversion
static char numberbuffer[12];

/**
 * @brief Draw integer number at specified position
 * 
 * Converts integer to decimal string and renders using drawString().
 * Supports full 32-bit signed range (-2147483648 to 2147483647).
 * 
 * @implementation Uses stdlib ltoa() for base-10 conversion.
 */
void CH1115Display::drawInt(uint8_t x, uint8_t y, int32_t num) {
  ltoa(num, numberbuffer, 10);
  drawString(x, y, numberbuffer);
}

/**
 * @brief Draw bitmap sprite at specified position
 * 
 * Renders a bitmap image stored in PROGMEM at (x, y).
 * Supports various drawing modes and handles page crossing.
 * 
 * @param sw Sprite width in pixels
 * @param sh Sprite height in pixels
 * @param data Pointer to bitmap data in PROGMEM
 * @param mode Drawing mode (OVERWRITE, OR, XOR, AND)
 * 
 * @implementation
 *  - Sprite data should be width bytes of column-oriented pixel data
 *  - Handles page-crossing similar to drawString()
 */
void CH1115Display::drawSprite(uint8_t x, uint8_t y, uint8_t sw, uint8_t sh,
                               const uint8_t *data, uint8_t mode) {
  if (y + sh > this->_height) {
    return;
  }

  uint8_t page_offset = y % 8;
  uint8_t mask = (page_offset) ? (0xFF << page_offset) : 0xFF;

  this->startPageDrawing(x, y);
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
  this->endPageDrawing();

  if (page_offset) {
    // draw second part
    mask = (0xFF >> (8 - page_offset));
    this->startPageDrawing(x, y + 8);
    for (uint8_t tx = 0; tx < sw; tx++) {
      if (x + tx >= this->_width) {
        continue;
      }
      uint8_t pattern = pgm_read_byte(&data[tx]);
      pattern = (pattern >> (8 - page_offset));
      this->updatePageColumn(pattern, mode, mask);
    }
    this->endPageDrawing();
  }
}

/**
 * @brief Send command byte to display controller via I2C
 * 
 * Formats and transmits a single command. I2C format:
 *  - Control byte: 0x00 (command mode)
 *  - Command byte: parameter
 * 
 * @implementation Performs complete I2C transaction (start/write/stop)
 */
void CH1115Display::send_command(uint8_t command) {
  TinyI2C.start(CH1115_I2C_ADDRESS, 0);
  TinyI2C.write(0x00);
  TinyI2C.write(command);
  TinyI2C.stop();
}

/**
 * @brief Send single data byte to display RAM via I2C
 * 
 * Transmits one byte of pixel data. I2C format:
 *  - Control byte: 0x40 (data mode)
 *  - Data byte: pixel pattern
 * 
 * @implementation Performs complete I2C transaction (start/write/stop)
 */
void CH1115Display::send_data(uint8_t byte) {
  TinyI2C.start(CH1115_I2C_ADDRESS, 0);
  TinyI2C.write(0x40);
  TinyI2C.write(byte);
  TinyI2C.stop();
}

/**
 * @brief Initiate data transfer sequence
 * 
 * Starts I2C transaction and sends first data byte.
 * Must be followed by add_data() and stop_data() calls.
 * I2C format:
 *  - Control byte: 0xC0 (data mode, more data coming)
 *  - First data byte
 */
void CH1115Display::start_data(uint8_t byte) {
  TinyI2C.start(CH1115_I2C_ADDRESS, 0);
  TinyI2C.write(0xC0);
  TinyI2C.write(byte);
}

/**
 * @brief Add data byte to ongoing transfer
 * 
 * Appends another byte to active I2C data transfer.
 * Must be called between start_data() and stop_data().
 * I2C format:
 *  - Control byte: 0xC0 (data mode, more data coming)
 *  - Data byte
 */
void CH1115Display::add_data(uint8_t byte) {
  TinyI2C.write(0xC0);
  TinyI2C.write(byte);
}

/**
 * @brief End data transfer sequence
 * 
 * Terminates data transfer and sends final byte.
 * Should follow start_data() and any add_data() calls.
 * Performs full I2C transaction (start/write/stop).
 * I2C format:
 *  - Control byte: 0x40 (data mode, last byte)
 *  - Final data byte
 */
void CH1115Display::stop_data(uint8_t byte) {
  TinyI2C.start(CH1115_I2C_ADDRESS, 0);
  TinyI2C.write(0x40);
  TinyI2C.write(byte);
  TinyI2C.stop();
}
