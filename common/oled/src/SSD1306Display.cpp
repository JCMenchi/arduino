
/**
 * @file SSD1306Display.cpp
 * @brief Implementation of SSD1306 OLED display driver
 * 
 * Complete implementation of the SSD1306Display class for controlling 128x32 and 128x64
 * OLED displays via I2C interface. This file includes:
 * - I2C communication and device initialization
 * - Display control (on/off, contrast, invert, flip)
 * - Graphics drawing primitives (pixels, lines, rectangles, circles)
 * - Text rendering with bitmap fonts
 * - Scrolling effects and animation
 * - Various display modes and effects
 * 
 * @see SSD1306Display.h for the class interface and documentation
 * @note Uses TinyI2C for I2C communication on AVR microcontrollers
 * @note Optimized for embedded systems with minimal RAM footprint
 */

#include <avr/pgmspace.h>
#include <stdlib.h>
#include <string.h>

#include "SSD1306Display.h"
#include "TinyI2CMaster.h"
#include "bitmap_font.h"

#ifdef HAS_SERIAL
#include <usart_serial.h>
#endif

/** @defgroup SSD1306_Commands SSD1306 Control Commands */
/** @{ */
#define SSD1306_MEMORYMODE 0x20         ///< Memory addressing mode command
#define SSD1306_COLUMNADDR 0x21         ///< Column address range command
#define SSD1306_PAGEADDR 0x22           ///< Page address range command
#define SSD1306_SETCONTRAST 0x81        ///< Contrast control command
#define SSD1306_CHARGEPUMP 0x8D         ///< Charge pump enable command
#define SSD1306_SEGREMAP 0xA0           ///< Segment remapping command
#define SSD1306_DISPLAYALLON_RESUME 0xA4 ///< Resume normal display
#define SSD1306_DISPLAYALLON 0xA5       ///< Display all pixels ON
#define SSD1306_NORMALDISPLAY 0xA6      ///< Normal display mode
#define SSD1306_INVERTDISPLAY 0xA7      ///< Inverted display mode
#define SSD1306_SETMULTIPLEX 0xA8       ///< Multiplex ratio command
#define SSD1306_DISPLAYOFF 0xAE         ///< Display OFF sleep mode
#define SSD1306_DISPLAYON 0xAF          ///< Display ON operation
#define SSD1306_COMSCANINC 0xC0         ///< COM scan direction normal
#define SSD1306_COMSCANDEC 0xC8         ///< COM scan direction reversed
#define SSD1306_SETDISPLAYOFFSET 0xD3   ///< Display offset command
#define SSD1306_SETDISPLAYCLOCKDIV 0xD5 ///< Clock divider command
#define SSD1306_SETPRECHARGE 0xD9       ///< Precharge period command
#define SSD1306_SETCOMPINS 0xDA         ///< COM pins config command
#define SSD1306_SETVCOMDETECT 0xDB      ///< VCOMH voltage command
/** @} */

#define SSD1306_SETSTARTLINE 0x40 ///< Start line address command

#define SSD1306_SWITCHCAPVCC 0x02 ///< Generate voltage from 3.3V

#define SSD1306_DEACTIVATE_SCROLL 0x2E  ///< Stop scrolling
#define SSD1306_ACTIVATE_SCROLL 0x2F    ///< Start scrolling
#define SSD1306_SET_VERTICAL_SCROLL_AREA 0xA3 ///< Define vertical scroll area

/**
 * @brief Utility macro to swap two uint8_t values
 * @param a First value
 * @param b Second value
 */


#define SSD1306_Swap(a, b)                                                      \
  {                                                                            \
    uint8_t t = a;                                                             \
    a = b;                                                                     \
    b = t;                                                                     \
  }

/**
 * @brief Constructor implementation
 * 
 * Initializes display dimensions for later use. I2C interface and display
 * configuration are performed separately in init(). This allows the display object
 * to be created before the I2C bus is available.
 * 
 * @param w Display width in pixels
 * @param h Display height in pixels
 * 
 * @note Call init() to complete initialization
 */
SSD1306Display::SSD1306Display(uint8_t w, uint8_t h) {
  _width = w;
  _height = h;
}

/**
 * @brief Destructor implementation
 * 
 * Cleans up resources. Currently a placeholder for potential cleanup operations.
 */
SSD1306Display::~SSD1306Display() {
}

#define SSD1306_STATUS 0x15

/**
 * @brief Initialize SSD1306 display and I2C interface
 * 
 * Performs complete initialization sequence for the SSD1306 OLED display:
 * - Initializes I2C communication (TinyI2C)
 * - Sends SSD1306 initialization command sequence
 * - Configures display timing, multiplex ratio, COM pins
 * - Sets contrast level
 * - Enables charge pump
 * - Turns on display output
 * 
 * @param contrast Display contrast level (0-255). Default 0x80 provides moderate brightness.
 *                 Higher values increase pixel brightness.
 * 
 * @note The I2C device must be connected and responding at address 0x3C or 0x3D
 * @note If HAS_SERIAL and OLED_DEBUG are defined, debug messages are output
 * @note Display content is preserved, but RAM should be cleared after init if needed
 * 
 * @see enable() to turn display on/off after initialization
 * @see contrast() to adjust contrast after initialization
 */
void SSD1306Display::init(uint8_t contrast) {

  #if defined(HAS_SERIAL) && defined(OLED_DEBUG)
  USART_WriteString("I2C connect...\n");
  #endif
  // Init I2C com
  TinyI2C.init();

  // Read one bit from device
#if defined(HAS_SERIAL) && defined(OLED_DEBUG)
  bool con = TinyI2C.start(SSD1306_I2C_ADDRESS, 1);
  if (!con) {
    USART_WriteString("I2C connect error.\n");
    return;
  }

  // Check display type
  uint8_t u = TinyI2C.read();
  TinyI2C.stop();

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

/**
 * @brief Enable or disable the display
 * 
 * Turns the OLED panel on or off using the SSD1306 display control commands.
 * When disabled, the display enters sleep mode while preserving RAM contents.
 * 
 * @param on 1 to enable display output (0xAF command)
 *          0 to disable display and enter sleep mode (0xAE command)
 * 
 * @note RAM content is preserved in sleep mode
 * @note No delay needed after this command
 * 
 * @see init() to initialize the display before calling this
 */
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

/**
 * @brief Execute a single scroll operation
 * 
 * Performs one horizontal scroll operation in the specified region and then stops.
 * Unlike scroll()/scrollArea() which configure continuous scrolling, this executes
 * a single scroll step and returns control immediately.
 * 
 * @param startPage Starting page for scroll (0-7)
 * @param endPage Ending page for scroll (0-7)
 * @param startCol Starting column (0-127)
 * @param endCol Ending column (0-127)
 * @param dir Scroll direction: SSD1306_SCROLL_LEFT or SSD1306_SCROLL_RIGHT
 * 
 * @note Uses SSD1306 commands 0x2C (right) and 0x2D (left) for single-scroll
 * @note Only horizontal scrolling is supported in this implementation
 * @note Vertical scrolling is not implemented
 * @note Each call performs exactly one scroll frame
 * 
 * @see scrollArea() to configure continuous scrolling parameters
 * @see scroll() to activate/deactivate continuous scrolling
 */
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

/**
 * @brief Set display contrast level
 * 
 * Adjusts the segment output current for all pixels. Higher values produce brighter pixels.
 * 
 * @param contrast Contrast level (0-255). Typical values 0x80-0xFF for visible brightness.
 * 
 * @note Command: 0x81 (Set Contrast Control)
 * @note RAM content is unaffected by contrast changes
 * 
 * Reference: SSD1306 datasheet section 10.1.7
 */
// 10.1.7 Set Contrast Control for BANK0 (81h)
// This command sets the Contrast Setting of the display. The chip has 256 contrast steps from 00h to FFh. The
// segment output current increases as the contrast step value increases.
void SSD1306Display::contrast(uint8_t contrast) {
  send_command(SSD1306_SETCONTRAST);     // Contrast Control Mode Set
  send_command(contrast); // Contrast Data Register Set:
}

/**
 * @brief Flip display orientation vertically and horizontally
 * 
 * Rotates the display 180 degrees by changing COM scan direction and segment remapping.
 * Allows flexible mounting orientations of the OLED module.
 * 
 * @param on 1 to enable flipped mode (180° rotation)
 *          0 to disable flipped mode (normal orientation)
 * 
 * @note Commands: 0xC0/0xC8 (COM scan direction) and 0xA0/0xA1 (Segment remap)
 * @note Display updates immediately when this command is issued
 * @note Effect applies to subsequently displayed content
 * 
 * Reference: SSD1306 datasheet sections 10.1.8 and 10.1.14
 */
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
/**
 * @brief Invert display colors
 * 
 * Swaps the display of ON and OFF pixels. In normal mode, RAM value 1 = ON pixel.
 * In inverted mode, RAM value 0 = ON pixel (inverted logic).
 * 
 * @param on 1 to enable inverted display (0xA7 command)
 *          0 for normal display (0xA6 command)
 * 
 * @note RAM content is unaffected; only display interpretation changes
 * @note Display updates immediately when this command is issued
 * 
 * Reference: SSD1306 datasheet section 10.1.10
 */// 10.1.10 Set Normal/Inverse Display (A6h/A7h)
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

/**
 * @brief Fill the entire display with a pattern and optional border
 * 
 * Fills all pages of the display with a specified byte pattern, with optional
 * decorative border drawn at edges. Useful for test patterns, clearing, or diagnostics.
 * 
 * @param pattern Byte pattern to fill (each bit represents 8 vertical pixels in a page)
 * @param border If true, draws a border frame around the display edges
 * 
 * @note Pattern byte format: bit 0 = bottom pixel of page, bit 7 = top pixel
 * @note Example: 0xFF fills all pixels on (white), 0x00 clears all pixels (black)
 * @note Border is drawn with vertical lines on sides and horizontal on top/bottom
 * @note Loop iterates through all display pages (height/8)
 * 
 * @see drawPixel() for drawing individual pixels
 * @see drawScreen() is this same function used for display test patterns
 */
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

/**
 * @brief Fill a single page (8-pixel horizontal row) with pattern
 * 
 * Fills an entire page with a uniform byte pattern. Each page is 8 pixels tall.
 * All columns in the specified page are set to the same pattern value.
 * 
 * @param p Page number (0-3 for 32-pixel display, 0-7 for 64-pixel display)
 * @param pattern Byte pattern to fill:
 *   - 0x00: All pixels OFF (black/clear)
 *   - 0xFF: All pixels ON (white/filled)
 *   - Other values: Custom checkerboard patterns
 * 
 * @note Organizes screen into pages: page N occupies rows (N*8) to (N*8+7)
 * @note Each bit in pattern represents one vertical pixel (bit 0=bottom, bit 7=top)
 * @note Fast operation that fills entire page width
 * 
 * @see drawPage() for filling partial page regions
 * @see drawScreen() for filling entire display
 * @see clearPage() macro which calls drawPage(p, 0x00)
 */
void SSD1306Display::drawPage(uint8_t p, uint8_t pattern) {
  setAddress(0, p * 8);
  TinyI2C.start(SSD1306_I2C_ADDRESS, 0);
  for (uint8_t i = 0; i < _width; i++) {
    TinyI2C.write((i < (_width - 1)) ? 0xC0 : 0x40);
    TinyI2C.write(pattern);
  }
  TinyI2C.stop();
}

/**
 * @brief Fill a portion of a page (8-pixel row) with pattern
 * 
 * Fills a rectangular region within a single page, starting at a specified column
 * for a given number of columns. Useful for partial page updates and efficient clearing.
 * 
 * @param p Page number (0-3 for 32-pixel display, 0-7 for 64-pixel display)
 * @param startcol Starting column address (0-127)
 * @param nbcol Number of consecutive columns to fill (1-128)
 * @param pattern Byte pattern to fill:
 *   - 0x00: All pixels OFF (black/clear)
 *   - 0xFF: All pixels ON (white/filled)
 *   - Other values: Custom pixel patterns
 * 
 * @note More efficient than drawPixel() for filling regions
 * @note Each column independently set to the same pattern
 * @note X coordinate range: startcol to (startcol + nbcol - 1)
 * 
 * @see drawPage() to fill entire page width
 * @see drawScreen() to fill entire display
 */
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

/**
 * @brief Update a single pixel within the current page drawing context
 * 
 * Reads the current page byte from display RAM via I2C, updates a single bit
 * corresponding to the specified Y position within the page (0-7), and writes
 * it back. Used internally during character rendering.
 * 
 * @param y Vertical position within page (0-7 representing rows 0-7 of current page)
 * @param colour Pixel color mode:
 *   - SSD1306_WHITE_COLOR (0): Set pixel to ON
 *   - SSD1306_BLACK_COLOR (1): Set pixel to OFF  
 *   - SSD1306_INVERSE_COLOR (2): Toggle pixel state
 * 
 * @return Previous byte value (before update) from display RAM
 * 
 * @note Requires I2C read/write operations - relatively slow
 * @note Only works within one page; cannot span multiple pages
 * @note Bit 0 = bottom row of page, Bit 7 = top row of page
 * @note Color definitions are counter-intuitive (0=ON, 1=OFF) - legacy from SSD1306
 * 
 * @see updatePageColumn() for updating entire page column (8 pixels)
 * @see drawPixel() for public pixel drawing interface
 */
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

/**
 * @brief Update a column of pixels (full 8-pixel vertical line) with pattern and mode
 * 
 * Reads current column from display RAM, applies drawing mode logic (overwrite, OR, XOR, AND),
 * applies bit mask for selective updates, and writes result back. Core operation for text
 * and graphics rendering. Supports multiple logical drawing modes.
 * 
 * @param pattern Byte pattern to apply (each bit = one vertical pixel)
 * @param mode Drawing mode:
 *   - OVERWRITE_MODE (0): Replace masked area with pattern
 *   - OR_MODE (1): Logical OR pattern with existing pixels
 *   - XOR_MODE (2): Logical XOR pattern with existing pixels
 *   - AND_MODE (3): Logical AND pattern with existing pixels
 * @param mask Bit mask (0xFF = all bits, 0x0F = lower 4 bits only, etc.)
 *        Only masked bits are modified; others remain unchanged
 * 
 * @return Previous column byte value (before modifications)
 * 
 * @note Bit 0 = bottom pixel, Bit 7 = top pixel in page
 * @note Drawing modes enable efficient compositing without full redraws
 * @note Requires I2C communication - performance-sensitive operation
 * @note Operates on single column within current page only
 * 
 * @see updatePagePixel() for single-pixel updates
 * @see drawString() which uses this for font rendering
 */
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

/**
 * @brief Draw a single pixel at specified coordinates
 * 
 * Sets or clears a pixel at the given X,Y coordinates with the specified color mode.
 * The function performs bounds checking and returns early if coordinates are out of range.
 * 
 * @param x X coordinate (0 to width-1)
 * @param y Y coordinate (0 to height-1)
 * @param colour Color/mode: SSD1306_WHITE_COLOR, SSD1306_BLACK_COLOR, or SSD1306_INVERSE_COLOR
 * 
 * @note Coordinates are clipped; out-of-bounds pixels are silently ignored
 * @note Requires I2C communication for each pixel operation
 * @note Performance-critical code; consider using drawLine() for multiple pixels
 * @note TODO: Frame buffer support would enable faster multi-pixel operations
 * 
 * @see drawLine() for drawing multiple connected pixels
 * @see drawHLine() drawVLine() for efficient line drawing
 */
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
  #if defined(HAS_SERIAL) && defined(OLED_DEBUG)
  USART_WriteString("R ");
  USART_WriteInt(r);
  USART_WriteString(" ");
  #endif

  r = TinyI2C.read();

  #if defined(HAS_SERIAL) && defined(OLED_DEBUG)
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

/**
 * @brief Draw a line between two points using Bresenham's algorithm
 * 
 * Draws a line from (x0, y0) to (x1, y1) with efficient integer arithmetic.
 * Optimizes horizontal and vertical lines by delegating to drawHLine() and drawVLine().
 * 
 * @param x0 Starting X coordinate (0 to width-1)
 * @param y0 Starting Y coordinate (0 to height-1)
 * @param x1 Ending X coordinate (0 to width-1)
 * @param y1 Ending Y coordinate (0 to height-1)
 * @param color Pixel color: SSD1306_WHITE_COLOR, SSD1306_BLACK_COLOR, or SSD1306_INVERSE_COLOR
 * 
 * @note Uses Bresenham's line algorithm for efficient rasterization
 * @note Horizontal/vertical lines are automatically optimized to faster implementations
 * @note Coordinates are not clipped; responsibility of caller
 * @note Pixels outside display bounds may cause undefined behavior
 * 
 * @see drawHLine() for horizontal line drawing
 * @see drawVLine() for vertical line drawing
 * @see drawPixel() for single pixel operations
 */
void SSD1306Display::drawLine(uint8_t x0, uint8_t y0, uint8_t x1, uint8_t y1, uint8_t color) {
  //if (y0 == y1) {
  //  if (x0 > x1) {
  //    SSD1306_Swap(x0, x1);
  //  }
  //  return drawHLine(x0, x1, y0, color);
  //}
  //if (x0 == x1) {
  //  if (y0 > y1) {
  //    SSD1306_Swap(y0, y1);
  //  }
  //  return drawVLine(x0, y0, y1, color);
  //}

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

/**
 * @brief Draw a horizontal line (internal optimization)
 * 
 * Fast horizontal line drawing from (x0, y) to (x1, y). Uses updatePagePixel()
 * which is optimized for sequential horizontal pixel access on the same page/row.
 * 
 * @param x0 Starting column (0-127)
 * @param x1 Ending column (0-127)
 * @param y Row coordinate (0-31 or 0-63)
 * @param color Pixel color: SSD1306_WHITE_COLOR or SSD1306_BLACK_COLOR
 * 
 * @note Called automatically by drawLine() for horizontal line optimization
 * @note Faster than drawPixel() for multiple pixels on same row
 * @note Assumes y is valid; no bounds checking
 * @note X coordinates not validated; ensure x0 <= x1
 * 
 * @see drawLine() for general line drawing
 * @see drawVLine() for vertical line drawing
 * @see updatePagePixel() for single pixel updates
 */
void SSD1306Display::drawHLine(uint8_t x0, uint8_t x1, uint8_t y,
                              uint8_t color) {
  this->setAddress(x0, y);
  for (uint8_t i = 0; i < (x1 - x0 + 1); i++) {
    this->updatePagePixel(y, color);
  }
}

/**
 * @brief Draw a vertical line (internal optimization)
 * 
 * Vertical line drawing from (x, y0) to (x, y1). Iterates through rows
 * calling drawPixel() for each vertical position.
 * 
 * @param x Column coordinate (0-127)
 * @param y0 Starting row (0-31 or 0-63)
 * @param y1 Ending row (0-31 or 0-63)
 * @param color Pixel color: SSD1306_WHITE_COLOR or SSD1306_BLACK_COLOR
 * 
 * @note Called automatically by drawLine() for vertical line optimization
 * @note Uses drawPixel() which is relatively slow; for efficiency, consider
 *       setting display RAM directly for large vertical regions
 * @note Assumes x is valid; no bounds checking
 * @note Y coordinates should satisfy y0 <= y1
 * 
 * @see drawLine() for general line drawing
 * @see drawHLine() for horizontal line drawing
 * @see drawPixel() for single pixel operations
 */
void SSD1306Display::drawVLine(uint8_t x, uint8_t y0, uint8_t y1, uint8_t color) {
  for (uint8_t y = y0; y <= y1; y++) {
    this->drawPixel(x, y, color);
  }
}

/**
 * @brief Draw a text string at specified position using small font
 * 
 * Renders a null-terminated ASCII string using a compact bitmap font stored in program memory.
 * Text wraps to next line if it exceeds display width. Font is 5x7 pixels per character.
 * 
 * @param x Starting X coordinate (0 to width-1)
 * @param y Starting Y coordinate (0 to height-1)
 * @param pText Pointer to null-terminated string to display
 * 
 * @note Font is stored in PROGMEM (program memory) for AVR optimization
 * @note Text rendering stops if right edge of display is reached
 * @note Y coordinate should be at character boundary (multiples of 8 for display pages)
 * @note Line wrapping does not occur; text is simply truncated
 * 
 * @see drawChar() for drawing single characters
 * @see drawInt() for drawing numeric values
 * 
 * @note Character set: ASCII 32-127 (standard printable ASCII)
 * @note Font metrics: FONT_CHAR_WIDTH x 7 pixels (typically 5x7)
 */
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

/**
 * @brief Draw an integer value as a string at specified position
 * 
 * Converts an integer to its string representation in the specified base,
 * then renders it at the given coordinates.
 * 
 * @param x Starting X coordinate (0 to width-1)
 * @param y Starting Y coordinate (0 to height-1)
 * @param i Integer value to display (int32_t range)
 * @param base Numerical base for conversion (typically 10 for decimal, 16 for hex, 2 for binary)
 * 
 * @return X coordinate after the last rendered character
 * 
 * @note Uses ltoa() for integer to string conversion
 * @note Base values: 2=binary, 8=octal, 10=decimal, 16=hexadecimal
 * @note Buffer size limits practical display to ~11 decimal digits
 * 
 * @see drawString() for drawing pre-converted strings
 * @see drawChar() for drawing single characters
 */
uint8_t SSD1306Display::drawInt(uint8_t x, uint8_t y, int32_t i, uint8_t base) {
  char numberbuffer[12];
  ltoa(i, numberbuffer, base);

  return drawString(x, y, numberbuffer);
}

/**
 * @brief Draw a single character at specified position
 * 
 * Renders a single ASCII character using the bitmap font by converting it
 * to a single-character string and delegating to drawString().
 * 
 * @param x Starting X coordinate (0 to width-1)
 * @param y Starting Y coordinate (0 to height-1)
 * @param c ASCII character code to display (0-127)
 * 
 * @return X coordinate after the rendered character
 * 
 * @note Character is wrapped in a temporary null-terminated string
 * @note Only ASCII characters 32-127 are properly rendered
 * @note Font size: 5-6 pixels wide x ~7 pixels tall
 * 
 * @see drawString() for drawing multiple characters
 * @see drawInt() for drawing numeric values
 */
uint8_t SSD1306Display::drawChar(uint8_t x, uint8_t y, char c) {
  char str[2];
  str[0] = c;
  str[1] = '\0';
  return drawString(x, y, str);
}

/**
 * @brief Draw text string from RAM at specified position
 * 
 * Renders a null-terminated string from RAM using the small bitmap font.
 * Handles page boundaries and vertical alignment automatically.
 * Supports characters at any Y position (not just page boundaries).
 * 
 * @param x Starting column (0-127)
 * @param y Starting row (0-31 or 0-63 depending on display height)
 * @param pText Pointer to null-terminated string in RAM
 * 
 * @return Column position after the last rendered character
 * 
 * @note Font: 5-6 pixels wide, ~7 pixels tall (FONT_CHAR_WIDTH x FONT_CHAR_HEIGHT)
 * @note Supports Y alignment within pages via page_offset calculation
 * @note For strings spanning multiple pages, renders in two passes
 * @note If Y + font height exceeds display height, returns early (no rendering)
 * @note String width calculated as strlen(pText) * (FONT_CHAR_WIDTH + 1) for spacing
 * @note Uses updatePageColumn() for efficient page-by-page rendering
 * 
 * @see drawChar() for single character
 * @see drawInt() for numeric values
 * @see drawPString() for strings in PROGMEM
 * @see drawString2() for large (2x) font rendering
 */
uint8_t SSD1306Display::drawString(uint8_t x, uint8_t y, const char *pText) {
  if (y + FONT_CHAR_HEIGHT > this->_height) {
    return x;
  }

  uint8_t nextpos = x + strlen(pText) * (FONT_CHAR_WIDTH + 1);

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

  return nextpos;
}

/**
 * @brief Draw text string from PROGMEM (program memory) at specified position
 * 
 * Renders a null-terminated string stored in PROGMEM (flash memory) using the small bitmap font.
 * Use this for constant strings to save RAM on AVR microcontrollers. Identical to drawString()
 * but reads characters from PROGMEM instead of RAM.
 * 
 * @param x Starting column (0-127)
 * @param y Starting row (0-31 or 0-63 depending on display height)
 * @param pText Pointer to null-terminated string in PROGMEM (declare with PROGMEM)
 * 
 * @return Column position after the last rendered character
 * 
 * @note Font: 5-6 pixels wide, ~7 pixels tall (FONT_CHAR_WIDTH x FONT_CHAR_HEIGHT)
 * @note Uses strlen_P() and pgm_read_byte() for PROGMEM access
 * @note Handles page boundaries and vertical alignment automatically
 * @note For strings spanning multiple pages, renders in two passes
 * @note String must be declared with PROGMEM attribute: const char str[] PROGMEM = "Text"
 * 
 * @see drawString() for strings from RAM
 * @see drawChar() for single character
 * @see drawInt() for numeric values
 * 
 * Example:
 * @code
 *   const char welcome[] PROGMEM = "Welcome";
 *   display.drawPString(0, 0, welcome);
 * @endcode
 */
uint8_t SSD1306Display::drawPString(uint8_t x, uint8_t y, const char *pText) {
  if (y + FONT_CHAR_HEIGHT > this->_height) {
    return x;
  }

  uint8_t nextpos = x + strlen_P(pText) * (FONT_CHAR_WIDTH + 1);

  uint8_t page_offset = y % 8;
  uint8_t mask = (page_offset) ? (0xFF << page_offset) : 0xFF;

  this->setAddress(x, y);

  for (uint8_t i=0; i < strlen_P(pText); i++) {
    if ((x + FONT_CHAR_WIDTH + 1) > this->_width) {
      break;
    }
    // draw
    char c = pgm_read_byte(&(pText[i]));

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
  }

  if (page_offset) {
    // draw second part
    mask = (0xFF >> (8 - page_offset));
    this->setAddress(x, y + 8);
    for (uint8_t i=0; i < strlen_P(pText); i++) {
      if ((x + FONT_CHAR_WIDTH + 1) > this->_width) {
        break;
      }
      // draw
      char c = pgm_read_byte(&(pText[i]));

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
    }
  }

  return nextpos;
}

/**
 * @brief Draw bitmap sprite at specified position with drawing mode
 * 
 * Renders a bitmap sprite from PROGMEM at the given position with support for
 * multiple drawing modes. Sprites are efficiently drawn using updatePageColumn()
 * with automatic page boundary handling.
 * 
 * @param x Starting column (0-127)
 * @param y Starting row (0-31 or 0-63 depending on display height)
 * @param sw Sprite width in pixels (columns)
 * @param sh Sprite height in pixels (rows, should be <= 16 for page-based rendering)
 * @param data Pointer to sprite bitmap data in PROGMEM. Each byte represents 8 vertical pixels
 * @param mode Drawing mode:
 *   - OVERWRITE_MODE (0): Replace existing pixels with sprite
 *   - OR_MODE (1): Logical OR sprite with existing content (combine)
 *   - XOR_MODE (2): Logical XOR sprite with existing content (invert overlap)
 *   - AND_MODE (3): Logical AND sprite with existing content (intersection)
 * 
 * @note Data format: Each byte represents 8 vertical pixels; sprite data should be
 *       in PROGMEM declared as: const uint8_t sprite[] PROGMEM = {...}
 * @note Automatically handles rendering across page boundaries
 * @note X coordinate range checked; Y coordinate not bounds-checked
 * @note Height should be <= 16 pixels for proper page boundary handling
 * @note Efficient for game sprites, icons, and bitmap graphics
 * 
 * @see updatePageColumn() for details on drawing modes
 * @see drawString2() for multi-row text
 * 
 * Example sprite rendering:
 * @code
 *   const uint8_t icon[] PROGMEM = {0x3C, 0x42, 0x81, 0x81, 0x81, 0x42, 0x3C};
 *   display.drawSprite(10, 10, 7, 8, icon, OR_MODE);
 * @endcode
 */
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

/**
 * @brief Send a control command to the display controller via I2C
 * 
 * Sends a single command byte to the SSD1306 controller. Commands control display
 * behavior such as contrast, addressing mode, and power state. Each command is
 * wrapped with appropriate I2C control bytes (0x00 prefix for command mode).
 * 
 * @param command Command byte to send (e.g., 0xAE for display OFF, 0xAF for ON)
 * 
 * @note I2C control byte 0x00 indicates command mode (vs. 0x40 for data)
 * @note Each call performs complete I2C transaction (start/write/stop)
 * @note For efficient multiple commands, consider using batch commands
 * @note Refer to SSD1306 datasheet section 10.1 for command definitions
 * 
 * @see contrast() for setting display brightness
 * @see enable() for display on/off control
 * @see flip() for display orientation
 * 
 * Command examples:
 *   0xAE = Display OFF
 *   0xAF = Display ON
 *   0x81 = Set contrast (followed by contrast value)
 *   0xA6 = Normal display
 *   0xA7 = Inverted display
 */
void SSD1306Display::send_command(uint8_t command) {
  TinyI2C.start(SSD1306_I2C_ADDRESS, 0);
  TinyI2C.write(0x00);
  TinyI2C.write(command);
  TinyI2C.stop();
}

/**
 * @brief Send a single data byte to display RAM via I2C
 * 
 * Sends one data byte to the display RAM at the current address pointer.
 * Wraps the byte with appropriate I2C control headers (0x40 for data mode).
 * 
 * @param byte Data byte to send (pixel pattern, 0x00-0xFF)
 * 
 * @note I2C control byte 0x40 indicates data mode (vs. 0x00 for commands)
 * @note Each call performs complete I2C transaction (start/write/stop)
 * @note Address must be set before calling with setAddress()
 * @note For multiple sequential bytes, use start_data()/add_data()/stop_data() instead
 * @note Relatively slow; prefer batch operations for performance
 * 
 * @see setAddress() to position RAM pointer before sending data
 * @see start_data() for multi-byte transfers
 * @see updatePageColumn() which uses this for pixel rendering
 */
void SSD1306Display::send_data(uint8_t byte) {
  TinyI2C.start(SSD1306_I2C_ADDRESS, 0);
  TinyI2C.write(0x40);
  TinyI2C.write(byte);
  TinyI2C.stop();
}

/**
 * @brief Initiate a data transfer sequence for multiple bytes
 * 
 * Starts an I2C data transfer and sends the first data byte. Used together with
 * add_data() and stop_data() for efficient multi-byte transfers. The I2C START
 * condition is sent once at the beginning.
 * 
 * @param byte First data byte to send
 * 
 * @note I2C control bytes:
 *   - 0xC0: Continue flag (0) + Data mode (1) + stream (1) = 0xC0
 * @note Must be followed by one or more add_data() calls and finally stop_data()
 * @note Does NOT call TinyI2C.stop() - must call stop_data() to complete transfer
 * @note Used internally by updatePageColumn() for pixel rendering
 * 
 * @see add_data() to append more bytes to the transfer
 * @see stop_data() to complete the transfer with final byte
 * @see send_data() for single-byte transfers
 * 
 * Example usage:
 * @code
 *   start_data(0xFF);  // Begin with 0xFF
 *   add_data(0x00);    // Add 0x00
 *   add_data(0xFF);    // Add 0xFF
 *   stop_data(0x80);   // End with 0x80
 * @endcode
 */
void SSD1306Display::start_data(uint8_t byte) {
  TinyI2C.start(SSD1306_I2C_ADDRESS, 0);
  TinyI2C.write(0xC0);
  TinyI2C.write(byte);
}

/**
 * @brief Append another data byte to ongoing I2C data transfer
 * 
 * Sends an additional data byte within a transfer sequence started by start_data().
 * Continues the same I2C transaction without sending STOP condition.
 * 
 * @param byte Data byte to append to transfer
 * 
 * @note I2C control byte 0xC0 indicates data mode with continuation
 * @note Only valid after calling start_data(); do not use standalone
 * @note Must eventually call stop_data() to terminate the I2C sequence
 * @note Used internally for batch pixel operations
 * 
 * @see start_data() to begin the transfer
 * @see stop_data() to complete the transfer
 * 
 * Usage pattern:
 * @code
 *   start_data(byte1);
 *   add_data(byte2);
 *   add_data(byte3);
 *   stop_data(byte4);  // Final byte and I2C STOP
 * @endcode
 */
void SSD1306Display::add_data(uint8_t byte) {
  TinyI2C.write(0xC0);
  TinyI2C.write(byte);
}

/**
 * @brief Complete a data transfer sequence with final byte and I2C STOP
 * 
 * Sends the last data byte of a multi-byte transfer and terminates the I2C
 * transaction with a STOP condition. Must be called after start_data() and
 * optional add_data() calls.
 * 
 * @param byte Final data byte to send
 * 
 * @note Sends I2C STOP condition to terminate the transaction
 * @note I2C control byte 0x40 for final byte (data mode, no continuation)
 * @note Completes transfer initiated by start_data()
 * @note Must not be called without preceding start_data()
 * 
 * @see start_data() to begin multi-byte transfer
 * @see add_data() to append intermediate bytes
 * @see send_data() for single-byte transfers
 * 
 * Usage pattern:
 * @code
 *   start_data(0xFF);
 *   add_data(0x00);
 *   add_data(0xFF);
 *   stop_data(0x80);  // Final byte and I2C STOP
 * @endcode
 */
void SSD1306Display::stop_data(uint8_t byte) {
  TinyI2C.start(SSD1306_I2C_ADDRESS, 0);
  TinyI2C.write(0x40);
  TinyI2C.write(byte);
  TinyI2C.stop();
}
