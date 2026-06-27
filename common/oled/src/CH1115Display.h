/**
 * @file CH1115Display.h
 * @brief Driver for CH1115 OLED display controller via I2C
 * 
 * This header defines the CH1115Display class for controlling OLED displays
 * with the CH1115 controller IC using I2C interface. Supports pixel drawing,
 * text rendering, scrolling effects, and various display modes.
 * 
 */

#ifndef _CH1115_DISPLAY_H
#define _CH1115_DISPLAY_H

#include <stdint.h>

/** @defgroup CH1115_I2C I2C Configuration */
/** @{ */
/** @brief Default I2C address (0x3C; 0x3D if SA0 is high) */
#define CH1115_I2C_ADDRESS 0x3C
/** @brief CH1115 device identifier */
#define CH1115_DEVICE_ID 0x15
/** @} */

/** @defgroup CH1115_Colors Pixel Color Definitions */
/** @{ */
/** @brief White pixel (displays as ON) */
#define CH1115_WHITE_COLOR 0
/** @brief Black pixel (displays as OFF) */
#define CH1115_BLACK_COLOR 1
/** @brief Inverted pixel color */
#define CH1115_INVERSE_COLOR 2
/** @} */

/** @defgroup CH1115_OnOff Display Enable/Disable */
/** @{ */
/** @brief Display or feature OFF state */
#define CH1115_OFF 0
/** @brief Display or feature ON state */
#define CH1115_ON 1
/** @} */

/** @defgroup CH1115_Scrolling Scrolling Modes and Configuration */
/** @{ */
/** @brief Scroll direction: right */
#define CH1115_SCROLL_RIGHT 0
/** @brief Scroll direction: left */
#define CH1115_SCROLL_LEFT 1
/** @brief Deactivate all scrolling */
#define CH1115_SCROLL_OFF 0xFF
/** @brief Continuous horizontal/vertical scroll (default) */
#define CH1115_SCROLL_CONTINUOUS 0x00
/** @brief Single screen scroll mode */
#define CH1115_SCROLL_ONCE 0x01
/** @brief One column scroll mode */
#define CH1115_SCROLL_ONE_COLUMN 0x02
/** @brief Frame interval: 2 frames */
#define CH1115_SCROLL_2FRAMES 0x07
/** @brief Frame interval: 3 frames */
#define CH1115_SCROLL_3FRAMES 0x04
/** @brief Frame interval: 4 frames */
#define CH1115_SCROLL_4FRAMES 0x05
/** @brief Frame interval: 5 frames */
#define CH1115_SCROLL_5FRAMES 0x06
/** @brief Frame interval: 6 frames */
#define CH1115_SCROLL_6FRAMES 0x00
/** @brief Frame interval: 32 frames */
#define CH1115_SCROLL_32FRAMES 0x01
/** @brief Frame interval: 64 frames */
#define CH1115_SCROLL_64FRAMES 0x02
/** @brief Frame interval: 128 frames */
#define CH1115_SCROLL_128FRAMES 0x03
/** @} */

/** @defgroup CH1115_DrawModes Drawing Modes */
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
 * @class CH1115Display
 * @brief I2C driver for CH1115 OLED display controller
 * 
 * Provides complete control of 128x32 OLED displays with CH1115 controller.
 * Supports full graphics drawing, text rendering, scrolling, and various
 * display effects through I2C interface.
 * 
 * @note This driver uses the TinyI2C library for I2C communication
 * @note Display must be connected to I2C at address 0x3C or 0x3D
 */
class CH1115Display {
public:
  /**
   * @brief Constructor for CH1115Display
   * @param w Display width in pixels (typically 128)
   * @param h Display height in pixels (typically 32 or 64)
   */
  CH1115Display(uint8_t w, uint8_t h);

  /**
   * @brief Destructor for CH1115Display
   */
  ~CH1115Display();

  /**
   * @brief Initialize the display and I2C interface
   * @param contrast Contrast level (0-255, default 0x80)
   * 
   * Initializes I2C communication, verifies device presence, and applies
   * initial configuration including contrast settings.
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
   * @brief Enable or disable breathing (pulsing) display effect
   * @param on 1 to enable breathing effect, 0 to disable
   * 
   * Creates a fading in/out effect. When enabled, uses maximum brightness
   * of 256 with 3-frame intervals.
   */
  void breathingEffect(uint8_t on);

  /**
   * @brief Configure scrolling area parameters
   * @param startPage Starting page for scroll (0-7)
   * @param endPage Ending page for scroll (0-7)
   * @param startCol Starting column (0-127)
   * @param endCol Ending column (0-127)
   * @param dir Scroll direction: CH1115_SCROLL_RIGHT or CH1115_SCROLL_LEFT
   * @param nbFrame Frame interval for scrolling
   * 
   * Defines the region and behavior of scrolling. Must call scroll() to activate.
   */
  void scrollArea(uint8_t startPage, uint8_t endPage, uint8_t startCol,
                  uint8_t endCol, uint8_t dir, uint8_t nbFrame);

  /**
   * @brief Start, stop, or configure scrolling mode
   * @param mode Scroll mode: CH1115_SCROLL_OFF, CH1115_SCROLL_CONTINUOUS,
   *             CH1115_SCROLL_ONCE, or CH1115_SCROLL_ONE_COLUMN
   * 
   * Activates scrolling in the region configured by scrollArea().
   */
  void scroll(uint8_t mode);

  /** @} */

  /** @name Screen Operations */
  /** @{ */

  /**
   * @brief Fill entire screen with pattern
   * @param pattern Pixel pattern to fill: CH1115_BLACK_COLOR or CH1115_WHITE_COLOR
   * @param border If true, draws a border around the display edge
   */
  void drawScreen(uint8_t pattern, bool border = false);

  /**
   * @brief Fill a single page (8-pixel row) with pattern
   * @param p Page number (0-3 for 32-pixel height)
   * @param pattern Pixel pattern: CH1115_BLACK_COLOR or CH1115_WHITE_COLOR
   */
  void drawPage(uint8_t p, uint8_t pattern);

  /**
   * @brief Fill a portion of a page with pattern
   * @param p Page number (0-3 for 32-pixel height)
   * @param startcol Starting column (0-127)
   * @param nbcol Number of columns to fill
   * @param pattern Pixel pattern: CH1115_BLACK_COLOR or CH1115_WHITE_COLOR
   */
  void drawPage(uint8_t p, uint8_t startcol, uint8_t nbcol, uint8_t pattern);

  /** @} */

  /** @name Drawing Primitives */
  /** @{ */

  /**
   * @brief Begin page-based pixel drawing at specified coordinates
   * @param x Starting column (0-127)
   * @param y Starting row (0-31), will be rounded to page boundary
   * 
   * Initializes I2C transfer for efficient sequential pixel writes.
   * Follow with updatePagePixel() and end with endPageDrawing().
   */
  void startPageDrawing(uint8_t x, uint8_t y);

  /**
   * @brief Update a single pixel within current page drawing operation
   * @param y Pixel row within page (0-7)
   * @param colour Color: CH1115_WHITE_COLOR, CH1115_BLACK_COLOR, or CH1115_INVERSE_COLOR
   * @return Updated pixel byte value
   * 
   * Must be called between startPageDrawing() and endPageDrawing().
   */
  uint8_t updatePagePixel(uint8_t y, uint8_t colour);

  /**
   * @brief Update entire column in current page drawing operation
   * @param pattern Pixel pattern for column
   * @param mode Drawing mode: OVERWRITE_MODE, OR_MODE, XOR_MODE, or AND_MODE
   * @param mask Bit mask for selective pixel updates (0xFF = all bits)
   * @return Updated column byte value
   * 
   * Must be called between startPageDrawing() and endPageDrawing().
   */
  uint8_t updatePageColumn(uint8_t pattern, uint8_t mode, uint8_t mask = 0xFF);

  /**
   * @brief End page-based pixel drawing operation
   * 
   * Finalizes the I2C transfer started by startPageDrawing().
   */
  void endPageDrawing();

  /**
   * @brief Draw text string using small font
   * @param x Starting column (0-127)
   * @param y Starting row (0-31)
   * @param pText Pointer to null-terminated string
   */
  void drawString(uint8_t x, uint8_t y, const char *pText);

  /**
   * @brief Draw text string using large (2x) font
   * @param x Starting column (0-127)
   * @param y Starting row (0-31)
   * @param pText Pointer to null-terminated string
   */
  void drawString2(uint8_t x, uint8_t y, const char *pText);

  /**
   * @brief Draw integer number at specified position
   * @param x Starting column (0-127)
   * @param y Starting row (0-31)
   * @param num Integer value to display (-2147483648 to 2147483647)
   */
  void drawInt(uint8_t x, uint8_t y, int32_t num);

  /**
   * @brief Draw a single pixel
   * @param x Column coordinate (0-127)
   * @param y Row coordinate (0-31)
   * @param color Pixel color: CH1115_WHITE_COLOR, CH1115_BLACK_COLOR, or CH1115_INVERSE_COLOR
   */
  void drawPixel(uint8_t x, uint8_t y, uint8_t color);

  /**
   * @brief Draw a line between two points
   * @param x0 Starting column coordinate
   * @param y0 Starting row coordinate
   * @param x1 Ending column coordinate
   * @param y1 Ending row coordinate
   * @param color Line color: CH1115_WHITE_COLOR or CH1115_BLACK_COLOR
   * 
   * Uses Bresenham's line algorithm for efficient rasterization.
   */
  void drawLine(uint8_t x0, uint8_t y0, uint8_t x1, uint8_t y1, uint8_t color);

  /**
   * @brief Draw bitmap sprite at specified position
   * @param x Starting column (0-127)
   * @param y Starting row (0-31)
   * @param w Sprite width in pixels
   * @param h Sprite height in pixels
   * @param data Pointer to sprite bitmap data (stored in PROGMEM)
   * @param mode Drawing mode: OVERWRITE_MODE, OR_MODE, XOR_MODE, or AND_MODE
   */
  void drawSprite(uint8_t x, uint8_t y, uint8_t w, uint8_t h,
                  const uint8_t *data, uint8_t mode);

  /** @} */

private:
  /**
   * @brief Set I2C address for RAM access
   * @param x Column coordinate (0-127)
   * @param y Row coordinate (0-31)
   * 
   * Internal helper to set page and column addressing for subsequent I2C transfers.
   */
  void setAddress(uint8_t x, uint8_t y);

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
