/**
 * @file SSD1306Display.h
 * @brief Driver for SSD1306 OLED display controller via I2C
 * 
 * This header defines the SSD1306Display class for controlling OLED displays
 * with the SSD1306 controller IC using I2C interface. Supports pixel drawing,
 * text rendering, scrolling effects, and various display modes.
 * 
 */

#ifndef _SSD1306_DISPLAY_H
#define _SSD1306_DISPLAY_H

#include <stdint.h>

/** @defgroup SSD1306_I2C I2C Configuration */
/** @{ */
/** @brief Default I2C address (0x3C; 0x3D if SA0 is high) */
#define SSD1306_I2C_ADDRESS 0x3C
/** @brief SSD1306 device identifier */
#define SSD1306_DEVICE_ID 0x15
/** @} */

/** @defgroup SSD1306_Colors Pixel Color Definitions */
/** @{ */
/** @brief White pixel (displays as ON) */
#define SSD1306_WHITE_COLOR 0
/** @brief Black pixel (displays as OFF) */
#define SSD1306_BLACK_COLOR 1
/** @brief Inverted pixel color */
#define SSD1306_INVERSE_COLOR 2
/** @} */

/** @defgroup SSD1306_OnOff Display Enable/Disable */
/** @{ */
/** @brief Display or feature OFF state */
#define SSD1306_OFF 0
/** @brief Display or feature ON state */
#define SSD1306_ON 1
/** @} */

/** @defgroup SSD1306_LinePositions Text Line Positions */
/** @{ */
/** @brief Y coordinate for line 0 (top) */
#define SSD1306_LINE0 0
/** @brief Y coordinate for line 1 */
#define SSD1306_LINE1 8
/** @brief Y coordinate for line 2 */
#define SSD1306_LINE2 16
/** @brief Y coordinate for line 3 (bottom for 32-pixel display) */
#define SSD1306_LINE3 24
/** @} */

/** @defgroup SSD1306_Scrolling Scrolling Modes and Configuration */
/** @{ */
/** @brief Scroll direction: right */
#define SSD1306_SCROLL_RIGHT 0
/** @brief Scroll direction: left */
#define SSD1306_SCROLL_LEFT 1
/** @brief Scroll direction: vertical and left */
#define SSD1306_SCROLL_VERTICAL_LEFT 2
/** @brief Scroll direction: vertical and right */
#define SSD1306_SCROLL_VERTICAL_RIGHT 3

/** @brief Scrolling active */
#define SSD1306_SCROLL_ON 1
/** @brief Scrolling inactive */
#define SSD1306_SCROLL_OFF 0

/** @brief Frame interval: 2 frames */
#define SSD1306_SCROLL_2FRAMES 0x07
/** @brief Frame interval: 3 frames */
#define SSD1306_SCROLL_3FRAMES 0x04
/** @brief Frame interval: 4 frames */
#define SSD1306_SCROLL_4FRAMES 0x05
/** @brief Frame interval: 5 frames */
#define SSD1306_SCROLL_5FRAMES 0x00
/** @brief Frame interval: 25 frames */
#define SSD1306_SCROLL_25FRAMES 0x06
/** @brief Frame interval: 64 frames */
#define SSD1306_SCROLL_64FRAMES 0x01
/** @brief Frame interval: 128 frames */
#define SSD1306_SCROLL_128FRAMES 0x02
/** @brief Frame interval: 256 frames */
#define SSD1306_SCROLL_256FRAMES 0x03
/** @} */

/** @defgroup SSD1306_DrawModes Drawing Modes */
/** @{ */
/** @brief Overwrite mode: replace existing pixels */
#define OVERWRITE_MODE 0
/** @brief OR mode: logical OR with existing pixels */
#define OR_MODE 1
/** @brief XOR mode: logical XOR with existing pixels */
#define XOR_MODE 2
/** @brief AND mode: logical AND with existing pixels */
#define AND_MODE 3
/** @} */


/**
 * @class SSD1306Display
 * @brief I2C driver for SSD1306 OLED display controller
 * 
 * Provides complete control of 128x32 or 128x64 OLED displays with SSD1306 controller.
 * Supports full graphics drawing, text rendering, scrolling, and various
 * display effects through I2C interface.
 * 
 * @note This driver uses the TinyI2C library for I2C communication
 * @note Display must be connected to I2C at address 0x3C or 0x3D
 */
class SSD1306Display {
public:
  /**
   * @brief Constructor for SSD1306Display
   * @param w Display width in pixels (typically 128)
   * @param h Display height in pixels (typically 32 or 64)
   */
  SSD1306Display(uint8_t w, uint8_t h);

  /**
   * @brief Destructor for SSD1306Display
   */
  ~SSD1306Display();

  /**
   * @brief Initialize the display and I2C interface
   * @param contrast Contrast level (0-255, default 0x80)
   * 
   * Initializes I2C communication and applies initial configuration including
   * contrast, addressing mode, and display orientation.
   */
  void init(uint8_t contrast = 0x80);

  /** @name Display Control */
  /** @{ */

  /**
   * @brief Enable or disable display output
   * @param on 1 to turn display ON, 0 to turn display OFF (enters sleep mode)
   * 
   * When OFF, the display enters power-saving mode while preserving RAM contents.
   */
  void enable(uint8_t on);

  /**
   * @brief Invert display colors (ON pixels become OFF and vice versa)
   * @param on 1 to enable inversion, 0 for normal display mode
   */
  void invert(uint8_t on);

  /**
   * @brief Flip display vertically and horizontally
   * @param on 1 to flip, 0 for normal orientation
   * 
   * Flips scan direction and segment remapping for rotated display layouts.
   */
  void flip(uint8_t on);

  /**
   * @brief Set display contrast
   * @param contrast Value from 0-255 (higher = brighter, typically 0x80-0xFF)
   * 
   * Adjusts the segment output current for all pixels.
   */
  void contrast(uint8_t contrast);

  /** @} */

  /** @name Special Effects */
  /** @{ */

  /**
   * @brief Configure scrolling area parameters
   * @param startPage Starting page for scroll (0-7)
   * @param endPage Ending page for scroll (0-7)
   * @param startCol Starting column (0-127)
   * @param endCol Ending column (0-127)
   * @param dir Scroll direction: SSD1306_SCROLL_RIGHT, SSD1306_SCROLL_LEFT,
   *            SSD1306_SCROLL_VERTICAL_LEFT, or SSD1306_SCROLL_VERTICAL_RIGHT
   * @param nbFrame Frame interval for scrolling
   * 
   * Defines the region and behavior of scrolling. Must call scroll() to activate.
   */
  void scrollArea(uint8_t startPage, uint8_t endPage, uint8_t startCol, uint8_t endCol, uint8_t dir, uint8_t nbFrame);

  /**
   * @brief Start or stop scrolling
   * @param mode SSD1306_SCROLL_ON to start, SSD1306_SCROLL_OFF to stop
   * 
   * Activates or deactivates scrolling in the region configured by scrollArea().
   */
  void scroll(uint8_t mode);

  /**
   * @brief Execute a single scroll operation
   * @param startPage Starting page for scroll (0-7)
   * @param endPage Ending page for scroll (0-7)
   * @param startCol Starting column (0-127)
   * @param endCol Ending column (0-127)
   * @param dir Scroll direction: SSD1306_SCROLL_LEFT or SSD1306_SCROLL_RIGHT
   * 
   * Performs one scroll operation and stops automatically.
   */
  void scrollOnce(uint8_t startPage, uint8_t endPage, uint8_t startCol, uint8_t endCol, uint8_t dir);

  /** @} */

  /** @name Screen Operations */
  /** @{ */

  /**
   * @brief Fill entire screen with pattern
   * @param pattern Pixel pattern to fill: SSD1306_BLACK_COLOR or SSD1306_WHITE_COLOR
   * @param border If true, draws a border around the display edge
   */
  void drawScreen(uint8_t pattern, bool border = false);

  /**
   * @brief Fill a single page (8-pixel row) with pattern
   * @param p Page number (0-3 for 32-pixel height, 0-7 for 64-pixel height)
   * @param pattern Pixel pattern: SSD1306_BLACK_COLOR or SSD1306_WHITE_COLOR
   */
  void drawPage(uint8_t p, uint8_t pattern);

  /**
   * @brief Fill a portion of a page with pattern
   * @param p Page number (0-3 for 32-pixel height, 0-7 for 64-pixel height)
   * @param startcol Starting column (0-127)
   * @param nbcol Number of columns to fill
   * @param pattern Pixel pattern: SSD1306_BLACK_COLOR or SSD1306_WHITE_COLOR
   */
  void drawPage(uint8_t p, uint8_t startcol, uint8_t nbcol, uint8_t pattern);

  /**
   * @brief Clear a single page (fill with black)
   * @param p Page number (0-3 for 32-pixel height, 0-7 for 64-pixel height)
   */
  void clearPage(uint8_t p) { drawPage(p, 0); }

  /** @} */

  /** @name Drawing Primitives */
  /** @{ */

  /**
   * @brief Update a single pixel within current page drawing
   * @param y Pixel row within page (0-7)
   * @param colour Color: SSD1306_WHITE_COLOR, SSD1306_BLACK_COLOR, or SSD1306_INVERSE_COLOR
   * @return Updated pixel byte value
   */
  uint8_t updatePagePixel(uint8_t y, uint8_t colour);

  /**
   * @brief Update entire column in current page drawing
   * @param pattern Pixel pattern for column
   * @param mode Drawing mode: OVERWRITE_MODE, OR_MODE, XOR_MODE, or AND_MODE
   * @param mask Bit mask for selective pixel updates (0xFF = all bits)
   * @return Updated column byte value
   */
  uint8_t updatePageColumn(uint8_t pattern, uint8_t mode, uint8_t mask = 0xFF);

  /**
   * @brief Draw integer number at specified position
   * @param x Starting column (0-127)
   * @param y Starting row (0-31 or 0-63 depending on display height)
   * @param i Integer value to display
   * @param base Number base (10 for decimal, 16 for hexadecimal)
   * @return Number of columns used by the rendered number
   */
  uint8_t drawInt(uint8_t x, uint8_t y, int32_t i, uint8_t base);

  /**
   * @brief Draw a single character
   * @param x Starting column (0-127)
   * @param y Starting row (0-31 or 0-63 depending on display height)
   * @param c Character to draw
   * @return Number of columns used by the character
   */
  uint8_t drawChar(uint8_t x, uint8_t y, char c);

  /**
   * @brief Draw text string using font
   * @param x Starting column (0-127)
   * @param y Starting row (0-31 or 0-63 depending on display height)
   * @param pText Pointer to null-terminated string (from RAM)
   * @return Number of columns used by the string
   */
  uint8_t drawString(uint8_t x, uint8_t y, const char *pText);

  /**
   * @brief Draw text string stored in program memory (PROGMEM)
   * @param x Starting column (0-127)
   * @param y Starting row (0-31 or 0-63 depending on display height)
   * @param pText Pointer to null-terminated string in PROGMEM
   * @return Number of columns used by the string
   */
  uint8_t drawPString(uint8_t x, uint8_t y, const char *pText);

  /**
   * @brief Draw a single pixel
   * @param x Column coordinate (0-127)
   * @param y Row coordinate (0-31 or 0-63 depending on display height)
   * @param color Pixel color: SSD1306_WHITE_COLOR, SSD1306_BLACK_COLOR, or SSD1306_INVERSE_COLOR
   */
  void drawPixel(uint8_t x, uint8_t y, uint8_t color);

  /**
   * @brief Draw a line between two points
   * @param x0 Starting column coordinate
   * @param y0 Starting row coordinate
   * @param x1 Ending column coordinate
   * @param y1 Ending row coordinate
   * @param color Line color: SSD1306_WHITE_COLOR or SSD1306_BLACK_COLOR
   * 
   * Uses Bresenham's line algorithm for efficient rasterization.
   */
  void drawLine(uint8_t x0, uint8_t y0, uint8_t x1, uint8_t y1, uint8_t color);

  /**
   * @brief Draw bitmap sprite at specified position
   * @param x Starting column (0-127)
   * @param y Starting row (0-31 or 0-63 depending on display height)
   * @param w Sprite width in pixels
   * @param h Sprite height in pixels
   * @param data Pointer to sprite bitmap data (stored in PROGMEM)
   * @param mode Drawing mode: OVERWRITE_MODE, OR_MODE, XOR_MODE, or AND_MODE
   */
  void drawSprite(uint8_t x, uint8_t y, uint8_t w, uint8_t h,
                  const uint8_t *data, uint8_t mode);

  /** @} */

    /**
   * @brief Draw horizontal line (internal)
   * @param x0 Starting column
   * @param x1 Ending column
   * @param y0 Row coordinate
   * @param color Line color
   */
  void drawHLine(uint8_t x0, uint8_t x1, uint8_t y0, uint8_t color);

  /**
   * @brief Draw vertical line (internal)
   * @param x0 Column coordinate
   * @param y1 Ending row
   * @param y0 Starting row
   * @param color Line color
   */
  void drawVLine(uint8_t x0, uint8_t y1, uint8_t y0, uint8_t color);
  
private:
  /**
   * @brief Set I2C address for RAM access
   * @param x Column coordinate (0-127)
   * @param y Row coordinate (0-31 or 0-63)
   * 
   * Internal helper to set page and column addressing for subsequent I2C transfers.
   */
  void setAddress(uint8_t x, uint8_t y);



  /**
   * @brief Draw text string using large (2x) font (internal)
   * @param x Starting column (0-127)
   * @param y Starting row
   * @param pText Pointer to null-terminated string
   */
  void drawString2(uint8_t x, uint8_t y, const char *pText);

  /**
   * @brief Send data byte to display
   * @param data Byte value to send
   * 
   * Performs I2C write with appropriate control headers.
   */
  void send_data(uint8_t data);

  /**
   * @brief Start data transfer sequence
   * @param byte First data byte to send
   * 
   * Initiates I2C transfer for multiple data bytes.
   */
  void start_data(uint8_t byte);

  /**
   * @brief Add data byte to ongoing transfer
   * @param byte Data byte to append
   */
  void add_data(uint8_t byte);

  /**
   * @brief End data transfer sequence
   * @param byte Final data byte to send
   */
  void stop_data(uint8_t byte);

  /**
   * @brief Send command to display controller
   * @param command Command byte to send
   * 
   * Sends controller command with appropriate I2C framing.
   */
  void send_command(uint8_t command);

  /** @name Member Variables */
  /** @{ */
  
  uint8_t _height;  /**< Display height in pixels */
  uint8_t _width;   /**< Display width in pixels */
  
  /** @} */
};

#endif
