/**
 * @file ST7735.h
 * @brief Driver for Sitronix ST7735 TFT LCD Display Controller
 *
 * This module provides a C++ class interface for the ST7735 TFT display controller,
 * commonly used in 128x160 pixel color LCD modules. The ST7735 is a popular, low-cost
 * display controller for small TFT screens in embedded systems and IoT applications.
 *
 * ### Display Specifications
 *
 * | Parameter | Value |
 * |-----------|-------|
 * | Resolution | 128 × 160 pixels |
 * | Color Depth | 16-bit RGB565 (65,536 colors) |
 * | Communication | SPI |
 * | Pixel Size | ~0.6 mm (typical) |
 * | Operating Temperature | -20°C to +70°C |
 *
 * ### Features
 *
 * - Multiple rotation modes (0°, 90°, 180°, 270°)
 * - Display on/off and sleep mode control
 * - Display color inversion
 * - Graphics primitives (lines, circles, rectangles, triangles)
 * - Text rendering with scalable fonts
 * - RGB565 color support with 65,536 colors
 * - Hardware SPI communication
 *
 * ### Quick Start
 *
 * @code
 * SPIManager spi;
 * TFT_ST7735 display(0, 1, &spi);  // DC=PB0, RST=PB1
 * display.init();
 * display.fillRect(0, 0, 128, 160, ST7735_BLUE);
 * display.drawText(20, 75, "Hello!", ST7735_WHITE, ST7735_BLUE);
 * @endcode
 *
 * @see SPIManager for SPI interface
 * @note Coordinates (0,0) at top-left corner
 */

#ifndef _TFT_ST7735H_
#define _TFT_ST7735H_

#include <avr/common.h>
#include <stdlib.h>

#include <gpio.h>

/**
 * @name Display Dimensions
 * @{
 */
#define ST7735_TFTWIDTH_128 128   ///< Display width in pixels
#define ST7735_TFTHEIGHT_160 160  ///< Display height in pixels
/** @} */

/**
 * @name Command Special Codes
 * @{
 */
#define ST_CMD_DELAY 0x80  ///< Command list special code for delay
/** @} */

/**
 * @name ST7735 Command Codes
 * Register and command definitions from the ST7735 datasheet.
 * @{
 */
#define ST7735_NOP           0x00   ///< No operation
#define ST7735_SWRESET       0x01   ///< Software reset
#define ST7735_RDDID         0x04   ///< Read display ID
#define ST7735_RDDST         0x09   ///< Read display status
#define ST7735_RDDMADCTL     0x0B   ///< Read MADCTL register
#define ST7735_RDDCOLMOD     0x0C   ///< Read color mode
#define ST7735_RDDIM         0x0D   ///< Read image mode
#define ST7735_SLPIN         0x10   ///< Sleep in
#define ST7735_SLPOUT        0x11   ///< Sleep out
#define ST7735_PTLON         0x12   ///< Partial mode on
#define ST7735_NORON         0x13   ///< Normal display mode on
#define ST7735_INVOFF        0x20   ///< Inversion off
#define ST7735_INVON         0x21   ///< Inversion on
#define ST7735_DISPOFF       0x28   ///< Display off
#define ST7735_DISPON        0x29   ///< Display on
#define ST7735_CASET         0x2A   ///< Column address set
#define ST7735_RASET         0x2B   ///< Row address set
#define ST7735_RAMWR         0x2C   ///< RAM write
#define ST7735_RAMRD         0x2E   ///< RAM read
#define ST7735_PTLAR         0x30   ///< Partial area
#define ST7735_TEOFF         0x34   ///< Tearing effect off
#define ST7735_TEON          0x35   ///< Tearing effect on
#define ST7735_MADCTL        0x36   ///< Memory access control
#define ST7735_COLMOD        0x3A   ///< Interface pixel format
#define ST7735_FRMCTR1       0xB1   ///< Frame rate control (normal)
#define ST7735_FRMCTR2       0xB2   ///< Frame rate control (idle)
#define ST7735_FRMCTR3       0xB3   ///< Frame rate control (partial)
#define ST7735_INVCTR        0xB4   ///< Display inversion control
#define ST7735_DISSET5       0xB6   ///< Display function set
#define ST7735_PWCTR1        0xC0   ///< Power control 1
#define ST7735_PWCTR2        0xC1   ///< Power control 2
#define ST7735_PWCTR3        0xC2   ///< Power control 3 (normal)
#define ST7735_PWCTR4        0xC3   ///< Power control 4 (idle)
#define ST7735_PWCTR5        0xC4   ///< Power control 5 (partial)
#define ST7735_VMCTR1        0xC5   ///< VCOM control 1
#define ST7735_PWCTR6        0xFC   ///< Power control 6
#define ST7735_GMCTRP1       0xE0   ///< Gamma positive polarity
#define ST7735_GMCTRN1       0xE1   ///< Gamma negative polarity
#define ST7735_RDID1         0xDA   ///< Read ID1
#define ST7735_RDID2         0xDB   ///< Read ID2
#define ST7735_RDID3         0xDC   ///< Read ID3
#define ST7735_RDID4         0xDD   ///< Read ID4
/** @} */

/**
 * @name MADCTL Register Control Bits
 * Memory access control bits for display orientation.
 * @{
 */
#define ST7735_MADCTL_MY 0x80   ///< Row address order
#define ST7735_MADCTL_MX 0x40   ///< Column address order
#define ST7735_MADCTL_MV 0x20   ///< Row/column exchange
#define ST7735_MADCTL_ML 0x10   ///< Vertical refresh order
#define ST7735_MADCTL_RGB 0x00  ///< RGB pixel order
#define ST7735_MADCTL_BGR 0x08  ///< BGR pixel order
#define ST7735_MADCTL_MH 0x04   ///< Horizontal refresh order
/** @} */

/**
 * @name Predefined Color Palette (RGB565)
 * 16-bit color constants for common colors.
 * @{
 */
#define ST7735_BLACK 0x0000     ///< Black
#define ST7735_WHITE 0xFFFF     ///< White
#define ST7735_RED 0xF800       ///< Red
#define ST7735_GREEN 0x07E0     ///< Green
#define ST7735_BLUE 0x001F      ///< Blue
#define ST7735_CYAN 0x07FF      ///< Cyan
#define ST7735_MAGENTA 0xF81F   ///< Magenta
#define ST7735_YELLOW 0xFFE0    ///< Yellow
#define ST7735_ORANGE 0xFC00    ///< Orange
/** @} */

class SPIManager;

/**
 * @brief ST7735 TFT Display Driver Class
 *
 * Provides complete control over a 128x160 ST7735 TFT LCD display connected via SPI.
 * Supports graphics primitives (lines, circles, rectangles, triangles), text rendering,
 * and full display configuration including rotation, sleep mode, and color inversion.
 *
 * ### Coordinate System
 *
 * - Origin (0,0) is at the top-left corner
 * - X increases to the right (0-127)
 * - Y increases downward (0-159)
 * - Coordinates adjust based on rotation setting
 *
 * ### Color Format
 *
 * Colors use 16-bit RGB565 format (5 bits red, 6 bits green, 5 bits blue).
 * Use color565() to convert 8-bit RGB to 16-bit RGB565:
 * @code
 * uint16_t orange = display.color565(255, 165, 0);
 * uint16_t purple = display.color565(128, 0, 128);
 * @endcode
 *
 * @note Requires SPIManager for SPI communication
 * @note DC pin is hardcoded to PORTB
 * @see SPIManager for SPI interface
 */
class TFT_ST7735 {
   public:
    /**
     * @brief Constructs a TFT_ST7735 display driver instance.
     * @param[in] dc Data/Command pin number (0-7 on PORTB)
     * @param[in] rst Reset pin number (-1 to disable)
     * @param[in] spi Pointer to initialized SPIManager instance
     * @note Call init() after construction
     */
    TFT_ST7735(int8_t dc, int8_t rst, SPIManager* spi);

    /**
     * @brief Initializes the display.
     * Performs hardware reset, sends init commands, and enables display.
     * @note Must be called before drawing operations
     */
    void init();

    /**
     * @brief Sets the display rotation (0, 90, 180, or 270 degrees).
     * @param[in] m Rotation: 0=portrait, 1=90°, 2=180°, 3=270°
     */
    void setRotation(uint8_t m);

    /**
     * @brief Enables or disables display output.
     * @param[in] enable 0=display off, non-zero=display on
     */
    void enableDisplay(uint8_t enable);

    /**
     * @brief Enables or disables tearing signal (vsync).
     * @param[in] enable 0=off, non-zero=on
     */
    void enableTearing(uint8_t enable);

    /**
     * @brief Enters or exits sleep mode (low-power standby).
     * @param[in] enable 0=sleep out, non-zero=sleep in
     */
    void enableSleep(uint8_t enable);

    /**
     * @brief Inverts display colors.
     * @param[in] i true=inversion on, false=inversion off
     */
    void invertDisplay(bool i);

    /**
     * @brief Fills a rectangular area with a solid color.
     * @param[in] x Left edge coordinate
     * @param[in] y Top edge coordinate
     * @param[in] w Rectangle width
     * @param[in] h Rectangle height
     * @param[in] color RGB565 color value
     */
    void fillRect(uint8_t x, uint8_t y, uint8_t w, uint8_t h, uint16_t color);

    /** @brief Sets a single pixel color. */
    void drawPixel(uint8_t x, uint8_t y, uint16_t color) {
        fillRect(x, y, 1, 1, color);
    }

    /** @brief Reads the color value of a pixel. */
    uint32_t readPixel(uint8_t x, uint8_t y);

    /** @brief Draws a horizontal line. */
    void drawHLine(uint8_t x, uint8_t y, uint8_t w, uint16_t color) {
        fillRect(x, y, w, 1, color);
    }

    /** @brief Draws a vertical line. */
    void drawVLine(uint8_t x, uint8_t y, uint8_t h, uint16_t color) {
        fillRect(x, y, 1, h, color);
    }

    /**
     * @brief Draws an RGB bitmap (raw pixel data).
     * @param[in] x Starting x-coordinate
     * @param[in] y Starting y-coordinate
     * @param[in] pcolors Pointer to RGB565 pixel array
     * @param[in] w Bitmap width
     * @param[in] h Bitmap height
     */
    void drawRGBBitmap(uint8_t x, uint8_t y, uint16_t* pcolors, uint8_t w, uint8_t h);

    /**
     * @brief Converts 8-bit RGB to 16-bit RGB565 color format.
     * @param[in] r Red (0-255)
     * @param[in] g Green (0-255)
     * @param[in] b Blue (0-255)
     * @return 16-bit RGB565 color value
     */
    uint16_t color565(uint8_t r, uint8_t g, uint8_t b);

    /** @brief Draws a line between two points. */
    void drawLine(uint8_t x0, uint8_t y0, uint8_t x1, uint8_t y1, uint16_t color);

    /** @brief Draws a circle outline. */
    void drawCircle(uint8_t x0, uint8_t y0, uint8_t r, uint16_t color);

    /** @brief Draws a partial circle (internal helper). */
    void drawCircleHelper(uint8_t x0, uint8_t y0, uint8_t r, uint8_t corners, uint16_t color);

    /** @brief Draws a filled circle. */
    void fillCircle(uint8_t x0, uint8_t y0, uint8_t r, uint16_t color);

    /** @brief Fills a partial circle (internal helper). */
    void fillCircleHelper(uint8_t x0, uint8_t y0, uint8_t r, uint8_t corners, int16_t delta, uint16_t color);

    /** @brief Draws a triangle outline. */
    void drawTriangle(uint8_t x0, uint8_t y0, uint8_t x1, uint8_t y1, uint8_t x2, uint8_t y2, uint16_t color);

    /** @brief Draws a filled triangle. */
    void fillTriangle(uint8_t x0, uint8_t y0, uint8_t x1, uint8_t y1, uint8_t x2, uint8_t y2, uint16_t color);

    /** @brief Draws a rectangle with rounded corners (outline). */
    void drawRoundRect(uint8_t x0, uint8_t y0, uint8_t w, uint8_t h, uint8_t radius, uint16_t color);

    /** @brief Draws a filled rectangle with rounded corners. */
    void fillRoundRect(uint8_t x0, uint8_t y0, uint8_t w, uint8_t h, uint8_t radius, uint16_t color);

    /** @brief Draws a single character. */
    void drawChar(uint8_t x, uint8_t y, unsigned char c, uint16_t color, uint16_t bg, uint8_t size = 1);

    /** @brief Draws a character with separate X/Y magnification. */
    void drawChar(uint8_t x, uint8_t y, unsigned char c, uint16_t color, uint16_t bg, uint8_t size_x, uint8_t size_y);

    /** @brief Draws a text string. */
    void drawText(uint8_t x, uint8_t y, const char* txt, uint16_t color, uint16_t bg, uint8_t size = 1);

    /** @brief Draws a text string with separate X/Y magnification. */
    void drawText(uint8_t x, uint8_t y, const char* txt, uint16_t color, uint16_t bg, uint8_t size_x, uint8_t size_y);

    /** @brief Starts a hardware SPI transaction. */
    void startWrite(void);

    /** @brief Ends a hardware SPI transaction. */
    void endWrite(void);

    /** @brief Reads an 8-bit response from a display command. */
    uint8_t readcommand8(uint8_t c);

    /** @brief Reads a 16-bit response from a display command. */
    uint16_t readcommand16(uint8_t c);

    /** @brief Reads a 32-bit response from a display command (display ID). */
    uint32_t readcommand32(uint8_t c);

   private:
    /** @brief Internal: Writes a single pixel at the specified location. */
    void writePixel(uint8_t x, uint8_t y, uint16_t color);

    /** @brief Internal: Sets the display address window for subsequent writes. */
    void setAddrWindow(uint8_t x, uint8_t y, uint8_t w, uint8_t h);

    /** @brief Internal: Sets Data/Command pin HIGH (data mode). */
    void SPI_DC_HIGH(void) {
        GPIO_SET_HIGH(B, _portb_dc_pin);
    }

    /** @brief Internal: Sets Data/Command pin LOW (command mode). */
    void SPI_DC_LOW(void) {
        GPIO_SET_LOW(B, _portb_dc_pin);
    }

    /** @brief Internal: Sends a command with optional data bytes. */
    void sendCommand(uint8_t commandByte, uint8_t* dataBytes = NULL, uint8_t numDataBytes = 0);

    /** @brief Internal: Sends a command with data from program memory. */
    void sendCommandFromPGM(uint8_t commandByte, const uint8_t* dataBytes, uint8_t numDataBytes);

    /** @brief Internal: Initializes display with command sequence from program memory. */
    void displayInit(const uint8_t* addr);

    SPIManager* _spi;                 ///< SPI manager pointer
    int8_t _portb_rst_pin;            ///< Reset pin number (or -1)
    int8_t _portb_dc_pin;             ///< Data/Command pin number
    int16_t _width;                   ///< Current display width
    int16_t _height;                  ///< Current display height
    uint8_t _nbStartWrite;            ///< SPI transaction nesting counter
};

#endif  // _TFT_ST7735H_
