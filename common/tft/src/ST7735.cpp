/**
 * @file ST7735.cpp
 * @brief Implementation of ST7735 TFT Display Driver
 *
 * This file contains the complete implementation of the TFT_ST7735 class,
 * including initialization, graphics primitives, text rendering, and
 * hardware control methods for the Sitronix ST7735 TFT display controller.
 *
 * ### Initialization Sequence
 *
 * The display initializes through the following process:
 * 1. Hardware reset pulse (100 ms low, 200 ms stabilize)
 * 2. Software reset command (ST7735_SWRESET)
 * 3. Sleep mode exit (ST7735_SLPOUT) with 500 ms delay
 * 4. Frame rate configuration (normal, idle, partial modes)
 * 5. Power supply configuration (voltage levels, biasing)
 * 6. VCOM voltage control
 * 7. Color mode set to 16-bit RGB565
 * 8. Normal display mode enabled
 *
 * ### Display Addressing
 *
 * The ST7735 uses column (X) and row (Y) addressing controlled by the
 * CASET (Column Address Set, 0x2A) and RASET (Row Address Set, 0x2B)
 * commands. Data is written sequentially to GRAM (Graphics RAM) via
 * the RAMWR command (0x2C).
 *
 * ### Color Format
 *
 * Display operates in RGB565 mode (16-bit):
 * - Bits 15-11: Red (5 bits, 0-31)
 * - Bits 10-5: Green (6 bits, 0-63)
 * - Bits 4-0: Blue (5 bits, 0-31)
 *
 * Data is transmitted in big-endian byte order: high byte first, low byte second.
 *
 * ### SPI Communication
 *
 * - Command/Data pin (DC) selects mode: LOW = command, HIGH = data
 * - All bytes transmitted MSB-first
 * - SPI Clock: typically 1-8 MHz
 * - Chip Select: typically tied to GND or controlled by SPIManager
 *
 * @see ST7735.h for public interface
 * @see SPIManager for SPI communication details
 */

#include "ST7735.h"

#include "SPIManager.h"
#include "bitmap_font.h"

#include <string.h>

/**
 * @brief Constructs the TFT_ST7735 display driver.
 *
 * Initializes member variables with default values but does NOT perform
 * hardware initialization. Call init() to initialize the display.
 *
 * @param[in] dc Data/Command pin number on PORTB (0-7)
 * @param[in] rst Reset pin number on PORTB (-1 if not used)
 * @param[in] spi Pointer to SPIManager instance for SPI communication
 *
 * @note SPI manager must remain valid for the lifetime of this object
 * @note Default dimensions: 128×160 pixels
 *
 * @see init(), setRotation()
 */
TFT_ST7735::TFT_ST7735(int8_t dc, int8_t rst, SPIManager *spi)
    : _portb_rst_pin(rst), _portb_dc_pin(dc) 
{
    _spi = spi;
    _width = 128;
    _height = 160;

    _nbStartWrite = 0;
}

/**
 * @name ST7735 Initialization Sequence
 * Default initialization command sequence for ST7735 display controller.
 * This configures all power supply settings, frame rates, and display modes.
 * @{
 */

/** @brief Initialization command sequence loaded from program memory
 *
 * Format: count, [command, arg_count | delay_flag, args..., delay_ms (if flag set)]
 * Includes hardware reset, power control, frame rate, and color mode setup.
 * Proper sequencing is critical for display stability and image quality.
 */
static const uint8_t PROGMEM RcmdBlue[] = {  // 7735 init
    17,                                      // 15 commands in list:
    ST7735_SWRESET,
    ST_CMD_DELAY,  //  1: Software reset, 0 args, w/delay
    250,           //     150 ms delay
    ST7735_SLPOUT,
    ST_CMD_DELAY,  //  2: Out of sleep mode, 0 args, w/delay
    255,           //     500 ms delay

    ST7735_FRMCTR1,
    3,  //  3: Framerate ctrl - normal mode, 3 arg:
    0x01,
    0x2C,
    0x2D,  //     Rate = fosc/(1x2+40) * (LINE+2C+2D)
    ST7735_FRMCTR2,
    3,  //  4: Framerate ctrl - idle mode, 3 args:
    0x01,
    0x2C,
    0x2D,  //     Rate = fosc/(1x2+40) * (LINE+2C+2D)
    ST7735_FRMCTR3,
    6,  //  5: Framerate - partial mode, 6 args:
    0x01,
    0x2C,
    0x2D,  //     Dot inversion mode
    0x01,
    0x2C,
    0x2D,  //     Line inversion mode
    ST7735_INVCTR,
    1,     //  6: Display inversion ctrl, 1 arg:
    0x00,  //     No inversion
    ST7735_PWCTR1,
    3,  //  7: Power control, 3 args, no delay:
    0xA2,
    0x02,  //     -4.6V
    0x84,  //     AUTO mode
    ST7735_PWCTR2,
    1,     //  8: Power control, 1 arg, no delay:
    0xC5,  //     VGH25=2.4C VGSEL=-10 VGH=3 * AVDD
    ST7735_PWCTR3,
    2,     //  9: Power control, 2 args, no delay:
    0x0A,  //     Opamp current small
    0x00,  //     Boost frequency
    ST7735_PWCTR4,
    2,     // 10: Power control, 2 args, no delay:
    0x8A,  //     BCLK/2,
    0x2A,  //     opamp current small & medium low
    ST7735_PWCTR5,
    2,  // 11: Power control, 2 args, no delay:
    0x8A,
    0xEE,
    ST7735_VMCTR1,
    1,  // 12: Power control, 1 arg, no delay:
    0x0E,

    ST7735_INVOFF,
    0,  // 13: Don't invert display, no args
    ST7735_MADCTL,
    1,     // 14: Mem access ctl (directions), 1 arg:
    0x08,  //
    ST7735_COLMOD,
    1,     // 15: set color mode, 1 arg, no delay:
    0x55,  //     16-bit color
    ST7735_NORON,
    ST_CMD_DELAY,  //  3: Normal display on, no args, w/delay
    10};

/** @} */  // End of initialization sequence

#include <Arduino.h>

/**
 * @brief Initializes the display with default configuration.
 *
 * Performs the following operations:
 * 1. Starts SPI master mode via SPIManager
 * 2. Configures DC (Data/Command) GPIO pin if specified
 * 3. Performs hardware reset if RST pin is configured:
 *    - 100 ms pulse LOW to reset
 *    - 100 ms settling after reset
 *    - 200 ms additional delay for stability
 * 4. Sends default initialization sequence (RcmdBlue) from program memory
 * 5. Sets initial rotation to portrait (0°)
 *
 * @return void
 *
 * @note Must be called before any drawing operations
 * @note Blocking operation; approximately 900 ms due to delays
 * @note If RST pin is -1, hardware reset is skipped
 * @note Display is enabled in portrait mode after init
 *
 * @see setRotation(), displayInit()
 */
void TFT_ST7735::init() {
    _spi->startMaster();

    if (_portb_dc_pin >= 0) {
        GPIO_OUTPUT(B, _portb_dc_pin);
        //pinMode(_portb_dc_pin, OUTPUT);
        GPIO_SET_HIGH(B, _portb_dc_pin);
        //digitalWrite(_portb_dc_pin, HIGH);
    }

    if (_portb_rst_pin >= 0) {
        // Toggle _portb_rst_pin low to reset
        //pinMode(_portb_rst_pin, OUTPUT);
        GPIO_OUTPUT(B, _portb_rst_pin);
        //digitalWrite(_portb_rst_pin, HIGH);
        GPIO_SET_HIGH(B, _portb_dc_pin);
        delay(100);
        GPIO_SET_LOW(B, _portb_dc_pin);
        //digitalWrite(_portb_rst_pin, LOW);
        delay(100);
        //digitalWrite(_portb_rst_pin, HIGH);
        GPIO_SET_HIGH(B, _portb_dc_pin);
        delay(200);
    }

    startWrite();
    displayInit(RcmdBlue);
    endWrite();
}

/**
 * @brief Sets display rotation (0°, 90°, 180°, 270°).
 *
 * Rotates the display coordinate system by adjusting the MADCTL
 * (Memory Access Control) register. This controls:
 * - Row address order (MY)
 * - Column address order (MX)
 * - Row/column exchange (MV)
 * - Pixel color order (RGB vs BGR)
 *
 * Rotation also swaps width and height to reflect new orientation.
 *
 * @param[in] m Rotation mode:
 *   - 0: Portrait (0°, default) - 128 wide × 160 tall
 *   - 1: Landscape (90° clockwise) - 160 wide × 128 tall
 *   - 2: Portrait upside-down (180°) - 128 wide × 160 tall
 *   - 3: Landscape (270° clockwise) - 160 wide × 128 tall
 *
 * @return void
 *
 * @note Values >3 are masked to 0-3 range
 * @note Origin (0,0) always at physical top-left after rotation
 * @note Affects all subsequent drawing operations
 * @note Blocking SPI operation
 *
 * Reference: ST7735 datasheet section 9.11.4 (MADCTL parameters)
 *
 * @see fillRect(), drawText()
 */
void TFT_ST7735::setRotation(uint8_t m) {
    uint8_t madctl = 0;

    m = m & 3;  // can't be higher than 3

    switch (m) {
        case 0:
            madctl = ST7735_MADCTL_MX | ST7735_MADCTL_MY | ST7735_MADCTL_BGR;

            _height = ST7735_TFTHEIGHT_160;
            _width = ST7735_TFTWIDTH_128;
            break;
        case 1:

            madctl = ST7735_MADCTL_MY | ST7735_MADCTL_MV | ST7735_MADCTL_BGR;

            _width = ST7735_TFTHEIGHT_160;
            _height = ST7735_TFTWIDTH_128;
            break;
        case 2:
            madctl = ST7735_MADCTL_BGR;

            _height = ST7735_TFTHEIGHT_160;
            _width = ST7735_TFTWIDTH_128;
            break;
        case 3:
            madctl = ST7735_MADCTL_MX | ST7735_MADCTL_MV | ST7735_MADCTL_RGB;

            _width = ST7735_TFTHEIGHT_160;
            _height = ST7735_TFTWIDTH_128;
            break;
    }

    startWrite();
    sendCommand(ST7735_MADCTL, &madctl, 1);
    endWrite();
}

/**
 * @brief Initializes display with command sequence from program memory.
 *
 * Executes a series of commands and optional delays stored in program memory (PROGMEM).
 * Format of command sequence:
 * - Byte 0: Number of commands to follow
 * - For each command:
 *   - Command byte
 *   - Argument count (high bit indicates if delay follows)
 *   - Argument bytes (if count > 0)
 *   - Delay in milliseconds (if high bit was set; 255 = 500 ms)
 *
 * This approach saves RAM by storing initialization sequences in flash.
 *
 * @param[in] addr Pointer to initialization sequence in program memory
 *
 * @return void
 *
 * @note Must be called within startWrite()/endWrite() block
 * @note Executes blocking delays as specified in sequence
 * @note Used for default initialization (RcmdBlue) and custom sequences
 *
 * @see init(), sendCommandFromPGM()
 */
void TFT_ST7735::displayInit(const uint8_t *addr) {
    uint8_t numCommands, cmd, numArgs;
    uint16_t ms;

    numCommands = pgm_read_byte(addr++);  // Number of commands to follow
    while (numCommands--) {               // For each command...
        cmd = pgm_read_byte(addr++);      // Read command
        numArgs = pgm_read_byte(addr++);  // Number of args to follow
        ms = numArgs & ST_CMD_DELAY;      // If hibit set, delay follows args
        numArgs &= ~ST_CMD_DELAY;         // Mask out delay bit
        sendCommandFromPGM(cmd, addr, numArgs);
        addr += numArgs;

        if (ms) {
            ms = pgm_read_byte(addr++);  // Read post-command delay time (ms)
            if (ms == 255)
                ms = 500;  // If 255, delay for 500 ms
            _delay_ms(ms);
        }
    }
}

/**
 * @brief Sets the display address window for drawing operations.
 *
 * Configures the column and row address ranges that will receive pixel data
 * on the next RAMWR (0x2C) command. Controls which rectangular region of
 * the display is affected by pixel writes.
 *
 * Sends two commands:
 * - CASET (0x2A): Sets column address range [x, x+w-1]
 * - RASET (0x2B): Sets row address range [y, y+h-1]
 * - RAMWR (0x2C): Prepares for RAM write
 *
 * @param[in] x Starting column address (0-127)
 * @param[in] y Starting row address (0-159)
 * @param[in] w Width in pixels
 * @param[in] h Height in pixels
 *
 * @return void
 *
 * @note Must be called within startWrite()/endWrite() block
 * @note Used internally by fillRect(), drawLine(), etc.
 * @note Coordinates should be within display bounds
 *
 * @see fillRect(), writePixel()
 */
void TFT_ST7735::setAddrWindow(uint8_t x, uint8_t y, uint8_t w, uint8_t h) {
    uint8_t col[4];
    col[0] = 0;
    col[1] = x;
    col[2] = 0;
    col[3] = x + w - 1;

    uint8_t row[4];
    row[0] = 0;
    row[1] = y;
    row[2] = 0;
    row[3] = y + h - 1;

    sendCommand(ST7735_CASET, col, 4);  // Column addr set

    sendCommand(ST7735_RASET, row, 4);  // Row addr set

    sendCommand(ST7735_RAMWR);  // write to RAM
}

/**
 * @brief Enables or disables the display output.
 *
 * Sends either DISPON (0x29) or DISPOFF (0x28) command to control
 * display visibility. When display is off, the screen appears black
 * but the driver remains operational.
 *
 * @param[in] enable 0 = display off (DISPOFF), non-zero = display on (DISPON)
 *
 * @return void
 *
 * @note Display is ON by default after init()
 * @note Power consumption remains high even when display is off
 * @note For power saving, use enableSleep() instead
 *
 * @see enableSleep(), init()
 */
void TFT_ST7735::enableDisplay(uint8_t enable) {
    startWrite();
    sendCommand(enable ? ST7735_DISPON : ST7735_DISPOFF);
    endWrite();
}

/**
 * @brief Enables or disables tearing effect signal (vsync).
 *
 * Sends either TEON (0x35) or TEOFF (0x34) command. When enabled,
 * the display generates a tearing signal that marks display refresh
 * boundaries. Useful for synchronizing animations to prevent visible tears.
 *
 * @param[in] enable 0 = tearing off (TEOFF), non-zero = tearing on (TEON)
 *
 * @return void
 *
 * @note Tearing is typically OFF by default
 * @note When enabled, synchronize drawing to tearing signal for best results
 * @note Requires application-level sync logic
 *
 * @see enableDisplay()
 */
void TFT_ST7735::enableTearing(uint8_t enable) {
    startWrite();
    sendCommand(enable ? ST7735_TEON : ST7735_TEOFF);
    endWrite();
}

/**
 * @brief Enters or exits low-power sleep mode.
 *
 * Sends either SLPIN (0x10) or SLPOUT (0x11) command. Sleep mode
 * reduces power consumption to ~200 µA by disabling internal
 * oscillators while preserving memory and settings.
 *
 * @param[in] enable 0 = sleep out (wake, SLPOUT), non-zero = sleep in (SLPIN)
 *
 * @return void
 *
 * @note Wakeup time from sleep is 10-100 ms
 * @note Display power consumption drops from ~50-80 mA to ~200 µA
 * @note Memory contents preserved during sleep
 * @note Useful for battery-powered applications
 *
 * @see enableDisplay()
 */
void TFT_ST7735::enableSleep(uint8_t enable) {
    startWrite();
    sendCommand(enable ? ST7735_SLPIN : ST7735_SLPOUT);
    endWrite();
}

/**
 * @brief Inverts display colors (black ↔ white).
 *
 * Sends either INVON (0x21) or INVOFF (0x20) command to enable or
 * disable display color inversion. When inverted, black pixels become
 * white and vice versa. No impact on drawn data; purely a display effect.
 *
 * @param[in] i true = inversion on (INVON), false = inversion off (INVOFF)
 *
 * @return void
 *
 * @note No performance penalty; only changes display mode
 * @note Can be toggled without affecting internal data
 * @note Useful for special display effects or accessibility
 *
 * @see enableDisplay()
 */
void TFT_ST7735::invertDisplay(bool i) {
    startWrite();
    sendCommand(i ? ST7735_INVON : ST7735_INVOFF);
    endWrite();
}

/**
 * @brief Reads an 8-bit response from a display command.
 *
 * Sends a command in command mode, then switches to data mode to read
 * one byte of response data. Converts MOSI line to input temporarily.
 *
 * Useful for reading display status and chip identification.
 *
 * @param[in] c Command code (e.g., ST7735_RDDST, ST7735_RDDIM)
 *
 * @return 8-bit response value from display
 *
 * @note Blocking operation; approximately 10-20 µs
 * @note Display must support read mode for this operation
 * @note Requires SPIManager with setMosiAsInput() support
 *
 * @see readcommand16(), readcommand32()
 */
uint8_t TFT_ST7735::readcommand8(uint8_t c) {
    _spi->begin();

    SPI_DC_LOW();
    _spi->send(c);
    // set MOSI to Hi-Z  (input no pullup)
    _spi->setMosiAsInput();
    SPI_DC_HIGH();

    uint8_t r = _spi->readFromMosi();

    _spi->setMosiAsOutput();
    _spi->end();

    return r;
}

/**
 * @brief Reads a 32-bit response from a display command.
 *
 * Sends a command in command mode, then reads four bytes of response data.
 * Typically used to read manufacturer ID (0xDA) and driver version.
 *
 * Response format depends on command:
 * - 0xDA (RDDID): Returns manufacturer and driver info
 * - Other commands: Varies by register
 *
 * @param[in] c Command code (typically ST7735_RDDID for ID reading)
 *
 * @return 32-bit response value (MSB first)
 *
 * @note Includes one dummy clock cycle before reading data
 * @note Blocking operation; approximately 40-60 µs
 * @note Display must support read mode
 *
 * @example
 * @code
 * uint32_t id = display.readcommand32(ST7735_RDDID);
 * // id format varies by display variant
 * @endcode
 *
 * @see readcommand8(), readcommand16()
 */
uint32_t TFT_ST7735::readcommand32(uint8_t c) {
    _spi->begin();

    SPI_DC_LOW();
    _spi->send(c);
    // set MOSI to Hi-Z  (input no pullup)
    _spi->setMosiAsInput();
    SPI_DC_HIGH();

    _spi->dummyClock();

    uint32_t r = _spi->readFromMosi();
    r <<= 8;
    r |= _spi->readFromMosi();
    r <<= 8;
    r |= _spi->readFromMosi();
    r <<= 8;
    r |= _spi->readFromMosi();

    _spi->setMosiAsOutput();
    _spi->end();

    return r;
}

/**
 * @brief Reads a 16-bit response from a display command.
 *
 * Sends a command in command mode, then reads two bytes of response data.
 *
 * @param[in] c Command code (e.g., ST7735_RDDID)
 *
 * @return 16-bit response value (MSB first)
 *
 * @note Includes one dummy clock cycle before reading data
 * @note Blocking operation; approximately 25-35 µs
 * @note Display must support read mode
 *
 * @see readcommand8(), readcommand32()
 */
uint16_t TFT_ST7735::readcommand16(uint8_t c) {
    _spi->begin();

    SPI_DC_LOW();
    _spi->send(c);
    // set MOSI to Hi-Z  (input no pullup)
    _spi->setMosiAsInput();
    SPI_DC_HIGH();

    _spi->dummyClock();

    uint16_t r = _spi->readFromMosi();
    r <<= 8;
    r |= _spi->readFromMosi();

    _spi->end();
    _spi->setMosiAsOutput();

    return r;
}

/**
 * @brief Fills a rectangular area with a solid color.
 *
 * Sets the address window to the specified coordinates and dimensions,
 * then writes the color value for each pixel in the rectangle. Colors
 * are transmitted in big-endian format (high byte first).
 *
 * This is the fundamental drawing operation; all other shapes use it.
 *
 * @param[in] x Left edge x-coordinate (0-127)
 * @param[in] y Top edge y-coordinate (0-159)
 * @param[in] w Rectangle width in pixels
 * @param[in] h Rectangle height in pixels
 * @param[in] color RGB565 16-bit color value
 *
 * @return void
 *
 * @note No clipping; coordinates should be within display bounds
 * @note (x, y) is the top-left corner
 * @note Zero width or height returns without drawing
 * @note Performance: ~1.5 µs per pixel @ 4 MHz SPI
 *
 * @code
 * display.fillRect(0, 0, 128, 160, ST7735_BLUE);   // Fill screen
 * display.fillRect(10, 10, 50, 50, ST7735_RED);    // Fill square
 * display.fillRect(0, 0, 64, 80, ST7735_WHITE);    // Half screen
 * @endcode
 *
 * @see fillCircle(), drawRoundRect(), color565()
 */
void TFT_ST7735::fillRect(uint8_t x, uint8_t y, uint8_t w, uint8_t h,
                          uint16_t color) {
    if (w == 0 || h == 0) {
        return;
    }

    startWrite();
    setAddrWindow(x, y, w, h);
    uint16_t nb = w * h;
    while (nb--) {
        // TFT display communicate in BigEndian
        _spi->send(color >> 8);
        _spi->send(color & 0xFF);
    }
    endWrite();
}

/**
 * @brief Reads the color value of a single pixel.
 *
 * Sets the address window to a 1×1 pixel, temporarily switches color
 * mode to 18-bit (0x66) to read the pixel, then restores 16-bit mode (0x55).
 *
 * @param[in] x X-coordinate (0-127)
 * @param[in] y Y-coordinate (0-159)
 *
 * @return 32-bit color value (format depends on color mode)
 *
 * @note Blocking operation; may take 50-100 µs
 * @note Display must support read mode
 * @note Temporarily changes color mode; automatically restored
 * @note Result format varies; typically RGB888 or RGB565 + padding
 *
 * @see fillRect(), setAddrWindow()
 */
uint32_t TFT_ST7735::readPixel(uint8_t x, uint8_t y) {
    uint32_t r = 0;
    startWrite();
    setAddrWindow(x, y, 1, 1);
    uint8_t colmod = 0x66;
    sendCommand(ST7735_COLMOD, &colmod, 1);
    r = readcommand32(ST7735_RAMRD);
    colmod = 0x55;
    sendCommand(ST7735_COLMOD, &colmod, 1);
    endWrite();
    return r;
}

/**
 * @brief Draws an RGB bitmap (raw pixel data) to the display.
 *
 * Copies a rectangular array of RGB565 pixel values to the display,
 * with automatic clipping at display boundaries. Skips regions outside
 * the display area.
 *
 * Bitmap data must be in row-major order (left-to-right, top-to-bottom).
 *
 * @param[in] x Starting x-coordinate (top-left corner)
 * @param[in] y Starting y-coordinate (top-left corner)
 * @param[in] pcolors Pointer to array of RGB565 pixel values
 * @param[in] w Bitmap width in pixels
 * @param[in] h Bitmap height in pixels
 *
 * @return void
 *
 * @note Automatic clipping at display boundaries
 * @note No performance penalty for off-screen bitmaps (clipped before write)
 * @note Bitmap data stays in RAM; use PROGMEM for large bitmaps
 * @note Useful for sprites, images, and patterns
 *
 * @code
 * uint16_t sprite[32*32];  // 32×32 sprite
 * // ... populate sprite array ...
 * display.drawRGBBitmap(48, 64, sprite, 32, 32);
 * @endcode
 *
 * @see fillRect(), drawPixel()
 */
void TFT_ST7735::drawRGBBitmap(uint8_t x, uint8_t y, uint16_t *pcolors, uint8_t w, uint8_t h) {
    int16_t x2, y2;                  // Lower-right coord
    if ((x >= _width) ||             // Off-edge right
        (y >= _height) ||            // " top
        ((x2 = (x + w - 1)) < 0) ||  // " left
        ((y2 = (y + h - 1)) < 0))
        return;  // " bottom

    int16_t bx1 = 0, by1 = 0,  // Clipped top-left within bitmap
        saveW = w;             // Save original bitmap width value
    if (x < 0) {               // Clip left
        w += x;
        bx1 = -x;
        x = 0;
    }
    if (y < 0) {  // Clip top
        h += y;
        by1 = -y;
        y = 0;
    }
    if (x2 >= _width)
        w = _width - x;  // Clip right
    if (y2 >= _height)
        h = _height - y;  // Clip bottom

    pcolors += by1 * saveW + bx1;  // Offset bitmap ptr to clipped top-left
    startWrite();
    setAddrWindow(x, y, w, h);  // Clipped area
    while (h--) {               // For each (clipped) scanline...
        // writePixels(pcolors, w); // Push one (clipped) row
        pcolors += saveW;  // Advance pointer by one full (unclipped) line
    }
    endWrite();
}

/**
 * @brief Converts 8-bit RGB color to 16-bit RGB565 format.
 *
 * Transforms three 8-bit color components (0-255 range) into the
 * 16-bit RGB565 format used by the display:
 * - Red: 5 bits (bits 15-11), divided by 8
 * - Green: 6 bits (bits 10-5), divided by 4
 * - Blue: 5 bits (bits 4-0), divided by 8
 *
 * Bit format: RRRRRGGGGGGBBBBB
 *
 * @param[in] red Red component (0-255)
 * @param[in] green Green component (0-255)
 * @param[in] blue Blue component (0-255)
 *
 * @return 16-bit RGB565 color value
 *
 * @note Fast operation; pure bitwise manipulation
 * @note Common use case: converting 8-bit RGB to display format
 * @note Predefined colors available: ST7735_RED, ST7735_GREEN, etc.
 *
 * @code
 * uint16_t orange = display.color565(255, 165, 0);
 * uint16_t purple = display.color565(128, 0, 128);
 * uint16_t cyan = display.color565(0, 255, 255);
 * display.fillRect(0, 0, 64, 80, orange);
 * @endcode
 *
 * @see fillRect(), drawText(), drawCircle()
 */
uint16_t TFT_ST7735::color565(uint8_t red, uint8_t green, uint8_t blue) {
    return ((red & 0xF8) << 8) | ((green & 0xFC) << 3) | (blue >> 3);
}

/**
 * @brief Starts a hardware SPI transaction block.
 *
 * Begins exclusive access to the SPI bus by calling SPIManager::begin().
 * Increments a nesting counter to support nested startWrite/endWrite pairs.
 * Useful when coordinating multiple devices on the same SPI bus.
 *
 * Must be paired with endWrite(). Multiple calls can be nested; the
 * SPI bus is only released when the nesting counter reaches zero.
 *
 * @return void
 *
 * @note Must be balanced with endWrite() calls
 * @note Allows efficient batching of multiple operations
 * @note Non-blocking if SPI is already available
 * @note Performance: approximately 1-2 µs overhead
 *
 * @code
 * display.startWrite();
 * display.fillRect(0, 0, 64, 80, ST7735_RED);
 * display.fillRect(64, 0, 64, 80, ST7735_BLUE);
 * display.endWrite();  // SPI released only here
 * @endcode
 *
 * @see endWrite()
 */
void TFT_ST7735::startWrite(void) {
    _spi->begin();
    _nbStartWrite++;
}

/**
 * @brief Ends a hardware SPI transaction block.
 *
 * Decrements the nesting counter and releases the SPI bus when counter
 * reaches zero. Allows proper cleanup after SPI operations.
 *
 * @return void
 *
 * @note Must be paired with startWrite() calls
 * @note SPI released only when nesting counter reaches zero
 * @note Safe to call even if startWrite() was never called (counter prevents underflow)
 * @note Performance: ~1-2 µs overhead
 *
 * @see startWrite()
 */
void TFT_ST7735::endWrite(void) {
    _nbStartWrite--;
    if (_nbStartWrite == 0) {
        _spi->end();
    }
}

/**
 * @brief Sends a display command with optional data bytes via SPI.
 *
 * Constructs and transmits a command frame:
 * 1. Sets DC pin LOW (command mode)
 * 2. Transmits command byte
 * 3. Sets DC pin HIGH (data mode)
 * 4. Transmits data bytes (if any)
 *
 * The SPIManager::begin() / end() should be called externally.
 *
 * @param[in] commandByte Command code (e.g., ST7735_MADCTL, ST7735_RAMWR)
 * @param[in] dataBytes Pointer to data byte array (may be NULL if no data)
 * @param[in] numDataBytes Number of data bytes to send (0 if no data)
 *
 * @return void
 *
 * @note Requires active SPI transaction (startWrite/endWrite)
 * @note No delay between command and data bytes
 * @note Data stays in RAM; use sendCommandFromPGM for PROGMEM data
 *
 * @code
 * uint8_t data[] = {0x08};  // MADCTL data
 * display.sendCommand(ST7735_MADCTL, data, 1);
 * @endcode
 *
 * @see sendCommandFromPGM(), startWrite()
 */
void TFT_ST7735::sendCommand(uint8_t commandByte, uint8_t *dataBytes,
                             uint8_t numDataBytes) {
    // send command
    SPI_DC_LOW();
    _spi->send(commandByte);
    SPI_DC_HIGH();

    // send command data
    for (int i = 0; i < numDataBytes; i++) {
        _spi->send(dataBytes[i]);
    }
}

/**
 * @brief Sends a display command with optional data bytes from program memory.
 *
 * Like sendCommand(), but reads data bytes from program memory (PROGMEM).
 * Useful for initialization sequences and other static data stored in flash
 * to conserve RAM.
 *
 * Command flow:
 * 1. Sets DC pin LOW (command mode)
 * 2. Transmits command byte
 * 3. Sets DC pin HIGH (data mode)
 * 4. Transmits data bytes from PROGMEM
 *
 * @param[in] commandByte Command code
 * @param[in] dataBytes Pointer to data byte array in PROGMEM
 * @param[in] numDataBytes Number of data bytes to send
 *
 * @return void
 *
 * @note Requires active SPI transaction (startWrite/endWrite)
 * @note Data read from program memory via pgm_read_byte()
 * @note Used internally by displayInit() for initialization sequences
 * @note Preserves RAM by storing static data in flash
 *
 * @see sendCommand(), displayInit()
 */
void TFT_ST7735::sendCommandFromPGM(uint8_t commandByte,
                                    const uint8_t *dataBytes,
                                    uint8_t numDataBytes) {
    // send command
    SPI_DC_LOW();
    _spi->send(commandByte);
    SPI_DC_HIGH();

    // send command data
    for (int i = 0; i < numDataBytes; i++) {
        _spi->send(pgm_read_byte(dataBytes++));
    }
}

#ifndef _swap_uint8_t
#define _swap_uint8_t(a, b) \
    {                       \
        uint8_t t = a;      \
        a = b;              \
        b = t;              \
    }
#endif

/**
 * @brief Writes a single pixel to the display at specified coordinates.
 *
 * Sets the address window to a 1×1 pixel and transmits the color value
 * in big-endian format (high byte first).
 *
 * This is the lowest-level drawing primitive; all drawing functions
 * ultimately call this or fillRect().
 *
 * @param[in] x X-coordinate (0-127)
 * @param[in] y Y-coordinate (0-159)
 * @param[in] color RGB565 16-bit color value
 *
 * @return void
 *
 * @note Slow for drawing many pixels; use fillRect() or drawLine() instead
 * @note Performance: ~10-15 µs per pixel
 * @note No clipping; coordinates should be within display bounds
 *
 * @see fillRect(), drawLine()
 */
void TFT_ST7735::writePixel(uint8_t x, uint8_t y, uint16_t color) {
    setAddrWindow(x, y, 1, 1);
    // TFT display communicate in BigEndian
    _spi->send(color >> 8);
    _spi->send(color & 0xFF);
}

/**
 * @brief Draws a line from (x0, y0) to (x1, y1) using Bresenham algorithm.
 *
 * Implements the Bresenham line drawing algorithm for fast integer-only
 * line rasterization. Handles all octants and automatically swaps coordinates
 * to ensure consistent pixel patterns.
 *
 * Algorithm overview:
 * 1. Determines if line is steep (dy > dx) and swaps coordinates if so
 * 2. Ensures x0 <= x1 for consistent stepping
 * 3. Uses error term to accumulate rasterization decisions
 * 4. Writes pixels along the line
 *
 * @param[in] x0 Starting x-coordinate
 * @param[in] y0 Starting y-coordinate
 * @param[in] x1 Ending x-coordinate
 * @param[in] y1 Ending y-coordinate
 * @param[in] color RGB565 16-bit color
 *
 * @return void
 *
 * @note Endpoints are included
 * @note Performance: ~8-12 µs per pixel
 * @note No clipping; coordinates should be within display bounds
 * @note Handles all eight octants efficiently
 *
 * @code
 * display.drawLine(0, 0, 127, 159, ST7735_WHITE);      // Diagonal
 * display.drawLine(64, 0, 64, 159, ST7735_GREEN);      // Vertical
 * display.drawLine(0, 80, 127, 80, ST7735_BLUE);       // Horizontal
 * @endcode
 *
 * @see drawCircle(), fillRect()
 */
void TFT_ST7735::drawLine(uint8_t x0, uint8_t y0, uint8_t x1, uint8_t y1,
                          uint16_t color) {
    uint8_t steep = abs(y1 - y0) > abs(x1 - x0);
    if (steep) {
        _swap_uint8_t(x0, y0);
        _swap_uint8_t(x1, y1);
    }

    if (x0 > x1) {
        _swap_uint8_t(x0, x1);
        _swap_uint8_t(y0, y1);
    }

    uint8_t dx, dy;
    dx = x1 - x0;
    dy = abs(y1 - y0);

    int8_t err = dx / 2;
    int8_t ystep;

    if (y0 < y1) {
        ystep = 1;
    } else {
        ystep = -1;
    }
    startWrite();

    for (; x0 <= x1; x0++) {
        if (steep) {
            writePixel(y0, x0, color);
        } else {
            writePixel(x0, y0, color);
        }
        err -= dy;
        if (err < 0) {
            y0 += ystep;
            err += dx;
        }
    }

    endWrite();
}

/**
 * @brief Draws a circle outline using the midpoint circle algorithm.
 *
 * Implements the midpoint circle algorithm for efficient circle rasterization
 * with 8-way symmetry. Uses integer arithmetic only; no floating-point math.
 *
 * Algorithm:
 * 1. Initializes error term f = 1 - r
 * 2. Writes initial four cardinal points
 * 3. Steps through octant from angle 0° to 45°
 * 4. Uses 8-way symmetry to complete all eight octants
 * 5. Updates x and y using error term decisions
 *
 * @param[in] x0 Center x-coordinate
 * @param[in] y0 Center y-coordinate
 * @param[in] r Radius in pixels
 * @param[in] color RGB565 16-bit color
 *
 * @return void
 *
 * @note No clipping; circle may extend beyond display bounds
 * @note Performance: ~80-120 µs for r=40 (typical icon size)
 * @note 8-way symmetry ensures efficient drawing
 * @note For filled circles, use fillCircle()
 *
 * @code
 * display.drawCircle(64, 80, 40, ST7735_CYAN);   // Large circle
 * display.drawCircle(32, 32, 16, ST7735_YELLOW); // Small circle
 * display.drawCircle(96, 128, 25, ST7735_MAGENTA);
 * @endcode
 *
 * @see fillCircle(), drawLine()
 */
void TFT_ST7735::drawCircle(uint8_t x0, uint8_t y0, uint8_t r, uint16_t color) {
    int8_t f = 1 - r;
    int8_t ddF_x = 1;
    int8_t ddF_y = -2 * r;
    int8_t x = 0;
    int8_t y = r;

    startWrite();
    writePixel(x0, y0 + r, color);
    writePixel(x0, y0 - r, color);
    writePixel(x0 + r, y0, color);
    writePixel(x0 - r, y0, color);

    while (x < y) {
        if (f >= 0) {
            y--;
            ddF_y += 2;
            f += ddF_y;
        }
        x++;
        ddF_x += 2;
        f += ddF_x;

        writePixel(x0 + x, y0 + y, color);
        writePixel(x0 - x, y0 + y, color);
        writePixel(x0 + x, y0 - y, color);
        writePixel(x0 - x, y0 - y, color);
        writePixel(x0 + y, y0 + x, color);
        writePixel(x0 - y, y0 + x, color);
        writePixel(x0 + y, y0 - x, color);
        writePixel(x0 - y, y0 - x, color);
    }
    endWrite();
}

/**
 * @brief Draws a quarter-circle arc for rounded corner primitives.
 *
 * Draws a portion of a circle determined by which quadrant is needed.
 * Used internally by fillCircleHelper() and drawRoundRect() to create
 * rounded rectangle corners and filled circles.
 *
 * Corner selection via cornername:
 * - 0x1 = top-left, 0x2 = top-right
 * - 0x4 = bottom-left, 0x8 = bottom-right
 * Combine flags for multiple corners: 0xF = all four corners
 *
 * @param[in] x0 Center x-coordinate
 * @param[in] y0 Center y-coordinate
 * @param[in] r Radius in pixels
 * @param[in] cornername Bitmask selecting which quadrants to draw
 * @param[in] color RGB565 16-bit color
 *
 * @return void
 *
 * @note Corner names: 1=TL, 2=TR, 4=BL, 8=BR
 * @note Used internally; rarely called directly
 * @note Performance: Depends on radius; ~40-80 µs typical
 *
 * @see fillCircleHelper(), drawCircle(), drawRoundRect()
 */
void TFT_ST7735::drawCircleHelper(uint8_t x0, uint8_t y0, uint8_t r,
                                  uint8_t cornername, uint16_t color) {
    int8_t f = 1 - r;
    int8_t ddF_x = 1;
    int8_t ddF_y = -2 * r;
    int8_t x = 0;
    int8_t y = r;

    while (x < y) {
        if (f >= 0) {
            y--;
            ddF_y += 2;
            f += ddF_y;
        }
        x++;
        ddF_x += 2;
        f += ddF_x;
        if (cornername & 0x4) {
            writePixel(x0 + x, y0 + y, color);
            writePixel(x0 + y, y0 + x, color);
        }
        if (cornername & 0x2) {
            writePixel(x0 + x, y0 - y, color);
            writePixel(x0 + y, y0 - x, color);
        }
        if (cornername & 0x8) {
            writePixel(x0 - y, y0 + x, color);
            writePixel(x0 - x, y0 + y, color);
        }
        if (cornername & 0x1) {
            writePixel(x0 - y, y0 - x, color);
            writePixel(x0 - x, y0 - y, color);
        }
    }
}

/**
 * @brief Draws a filled circle.
 *
 * Creates a solid filled circle by:
 * 1. Drawing a vertical line through the center to fill the middle band
 * 2. Using fillCircleHelper() to fill the top and bottom symmetric regions
 *
 * @param[in] x0 Center x-coordinate
 * @param[in] y0 Center y-coordinate
 * @param[in] r Radius in pixels
 * @param[in] color RGB565 16-bit color
 *
 * @return void
 *
 * @note No clipping; circle may extend beyond display bounds
 * @note Performance: ~500-2000 µs depending on radius
 * @note Uses drawVLine() and fillCircleHelper() internally
 *
 * @code
 * display.fillCircle(64, 80, 30, ST7735_RED);      // Solid red circle
 * display.fillCircle(32, 32, 20, ST7735_GREEN);    // Green dot
 * @endcode
 *
 * @see drawCircle(), fillCircleHelper()
 */
void TFT_ST7735::fillCircle(uint8_t x0, uint8_t y0, uint8_t r, uint16_t color) {
    drawVLine(x0, y0 - r, 2 * r + 1, color);
    fillCircleHelper(x0, y0, r, 3, 0, color);
}

/**
 * @brief Fills the interior of a circular region by drawing horizontal lines.
 *
 * Used internally by fillCircle() to fill top and bottom halves of circles.
 * Draws horizontal scan lines to fill the circular region with integer-only
 * midpoint circle algorithm calculations.
 *
 * @param[in] x0 Center x-coordinate
 * @param[in] y0 Center y-coordinate
 * @param[in] r Radius in pixels
 * @param[in] corners Bitmask selecting which regions to fill (0x3 = both)
 * @param[in] delta Y-offset for scan line adjustment
 * @param[in] color RGB565 16-bit color
 *
 * @return void
 *
 * @note Used internally by fillCircle(); rarely called directly
 * @note Used in fillRoundRect() for corner filling
 * @note Performance: Depends on radius; ~200-1000 µs typical
 *
 * @see fillCircle(), fillRoundRect()
 */
void TFT_ST7735::fillCircleHelper(uint8_t x0, uint8_t y0, uint8_t r,
                                  uint8_t corners, int16_t delta,
                                  uint16_t color) {
    int8_t f = 1 - r;
    int8_t ddF_x = 1;
    int8_t ddF_y = -2 * r;
    int8_t x = 0;
    int8_t y = r;
    int8_t px = x;
    int8_t py = y;

    delta++;  // Avoid some +1's in the loop

    while (x < y) {
        if (f >= 0) {
            y--;
            ddF_y += 2;
            f += ddF_y;
        }
        x++;
        ddF_x += 2;
        f += ddF_x;
        // These checks avoid double-drawing certain lines, important
        // for the SSD1306 library which has an INVERT drawing mode.
        if (x < (y + 1)) {
            if (corners & 1)
                drawVLine(x0 + x, y0 - y, 2 * y + delta, color);
            if (corners & 2)
                drawVLine(x0 - x, y0 - y, 2 * y + delta, color);
        }
        if (y != py) {
            if (corners & 1)
                drawVLine(x0 + py, y0 - px, 2 * px + delta, color);
            if (corners & 2)
                drawVLine(x0 - py, y0 - px, 2 * px + delta, color);
            py = y;
        }
        px = x;
    }
}

/**
 * @brief Draws a triangle outline defined by three vertices.
 *
 * Draws three lines connecting the three vertices. Efficient rasterization
 * of arbitrary triangles.
 *
 * @param[in] x0 First vertex x-coordinate
 * @param[in] y0 First vertex y-coordinate
 * @param[in] x1 Second vertex x-coordinate
 * @param[in] y1 Second vertex y-coordinate
 * @param[in] x2 Third vertex x-coordinate
 * @param[in] y2 Third vertex y-coordinate
 * @param[in] color RGB565 16-bit color
 *
 * @return void
 *
 * @note No clipping; vertices may extend beyond display bounds
 * @note Performance: Depends on side lengths; typically 100-300 µs
 * @note For filled triangles, use fillTriangle()
 *
 * @see fillTriangle(), drawLine()
 */
void TFT_ST7735::drawTriangle(uint8_t x0, uint8_t y0, uint8_t x1, uint8_t y1,
                              uint8_t x2, uint8_t y2, uint16_t color) {
    drawLine(x0, y0, x1, y1, color);
    drawLine(x1, y1, x2, y2, color);
    drawLine(x2, y2, x0, y0, color);
}

/**
 * @brief Fills a triangle with a solid color.
 *
 * Uses horizontal scan-line rasterization with integer-based edge equations.
 * Coordinates are automatically sorted by Y to optimize rasterization.
 *
 * Algorithm:
 * 1. Sorts three vertices by Y-coordinate (bottom to top)
 * 2. Handles degenerate case (all points collinear)
 * 3. Rasterizes upper half and lower half separately
 * 4. Draws horizontal lines between left and right edges
 *
 * @param[in] x0 First vertex x-coordinate
 * @param[in] y0 First vertex y-coordinate
 * @param[in] x1 Second vertex x-coordinate
 * @param[in] y1 Second vertex y-coordinate
 * @param[in] x2 Third vertex x-coordinate
 * @param[in] y2 Third vertex y-coordinate
 * @param[in] color RGB565 16-bit color
 *
 * @return void
 *
 * @note No clipping; triangle may extend beyond display bounds
 * @note Performance: Depends on triangle size; typically 200-1000 µs
 * @note Uses edge-walking for pixel-perfect rasterization
 * @note Handles all triangle orientations
 *
 * @code
 * display.fillTriangle(64, 0, 0, 159, 127, 159, ST7735_MAGENTA);
 * display.fillTriangle(32, 40, 96, 40, 64, 120, ST7735_CYAN);
 * @endcode
 *
 * @see drawTriangle(), drawLine()
 */
void TFT_ST7735::fillTriangle(uint8_t x0, uint8_t y0, uint8_t x1, uint8_t y1,
                              uint8_t x2, uint8_t y2, uint16_t color) {
    uint8_t a, b, y, last;

    // Sort coordinates by Y order (y2 >= y1 >= y0)
    if (y0 > y1) {
        _swap_uint8_t(y0, y1);
        _swap_uint8_t(x0, x1);
    }
    if (y1 > y2) {
        _swap_uint8_t(y2, y1);
        _swap_uint8_t(x2, x1);
    }
    if (y0 > y1) {
        _swap_uint8_t(y0, y1);
        _swap_uint8_t(x0, x1);
    }

    if (y0 == y2) {  // Handle awkward all-on-same-line case as its own thing
        a = b = x0;
        if (x1 < a)
            a = x1;
        else if (x1 > b)
            b = x1;
        if (x2 < a)
            a = x2;
        else if (x2 > b)
            b = x2;
        drawHLine(a, y0, b - a + 1, color);
        return;
    }

    int8_t dx01 = x1 - x0, dy01 = y1 - y0, dx02 = x2 - x0, dy02 = y2 - y0,
           dx12 = x2 - x1, dy12 = y2 - y1;
    int32_t sa = 0, sb = 0;

    // For upper part of triangle, find scanline crossings for segments
    // 0-1 and 0-2.  If y1=y2 (flat-bottomed triangle), the scanline y1
    // is included here (and second loop will be skipped, avoiding a /0
    // error there), otherwise scanline y1 is skipped here and handled
    // in the second loop...which also avoids a /0 error here if y0=y1
    // (flat-topped triangle).
    if (y1 == y2)
        last = y1;  // Include y1 scanline
    else
        last = y1 - 1;  // Skip it

    for (y = y0; y <= last; y++) {
        a = x0 + sa / dy01;
        b = x0 + sb / dy02;
        sa += dx01;
        sb += dx02;
        /* longhand:
        a = x0 + (x1 - x0) * (y - y0) / (y1 - y0);
        b = x0 + (x2 - x0) * (y - y0) / (y2 - y0);
        */
        if (a > b)
            _swap_uint8_t(a, b);
        drawHLine(a, y, b - a + 1, color);
    }

    // For lower part of triangle, find scanline crossings for segments
    // 0-2 and 1-2.  This loop is skipped if y1=y2.
    sa = (int32_t)dx12 * (y - y1);
    sb = (int32_t)dx02 * (y - y0);
    for (; y <= y2; y++) {
        a = x1 + sa / dy12;
        b = x0 + sb / dy02;
        sa += dx12;
        sb += dx02;
        /* longhand:
        a = x1 + (x2 - x1) * (y - y1) / (y2 - y1);
        b = x0 + (x2 - x0) * (y - y0) / (y2 - y0);
        */
        if (a > b)
            _swap_uint8_t(a, b);
        drawHLine(a, y, b - a + 1, color);
    }
    endWrite();
}

/**
 * @brief Draws a single character with uniform scaling.
 *
 * Renders a character from the bitmap font at specified coordinates.
 * The character is magnified by the given scale factor. Uses background
 * fill for the bounding box.
 *
 * @param[in] x Starting x-coordinate (top-left)
 * @param[in] y Starting y-coordinate (top-left)
 * @param[in] c Character to draw (ASCII code)
 * @param[in] color RGB565 foreground color
 * @param[in] bg RGB565 background color
 * @param[in] size Magnification factor (1=5×8 pixels, 2=10×16, etc.)
 *
 * @return void
 *
 * @note Font characters are 5×8 pixels before scaling
 * @note Clipping applied at all edges
 * @note Performance: ~200-800 µs depending on size
 * @note Font data stored in program memory (bitmap_font.h)
 *
 * @code
 * display.drawChar(0, 0, 'A', ST7735_WHITE, ST7735_BLACK, 2);
 * display.drawChar(64, 80, '5', ST7735_YELLOW, ST7735_BLUE, 3);
 * @endcode
 *
 * @see drawText(), drawChar()
 */
void TFT_ST7735::drawChar(uint8_t x, uint8_t y, unsigned char c, uint16_t color,
                          uint16_t bg, uint8_t size) {
    drawChar(x, y, c, color, bg, size, size);
}

/**
 * @brief Draws a single character with independent X and Y scaling.
 *
 * Renders a character from the bitmap font with separate magnification
 * factors for width and height. Supports non-uniform scaling and rotation.
 * Applies automatic clipping at all edges.
 *
 * Font metrics:
 * - Base character: 5×8 pixels
 * - Scaled: 5*size_x × 8*size_y pixels
 * - Background fill covers entire scaled area
 *
 * @param[in] x Starting x-coordinate (top-left)
 * @param[in] y Starting y-coordinate (top-left)
 * @param[in] c Character to draw (ASCII code, typically 32-126)
 * @param[in] color RGB565 foreground color
 * @param[in] bg RGB565 background color
 * @param[in] size_x Horizontal magnification factor
 * @param[in] size_y Vertical magnification factor
 *
 * @return void
 *
 * @note Clipping applied at all boundaries
 * @note Performance: ~200-1200 µs depending on scale
 * @note Font stored in program memory (bitmap_font.h)
 * @note Uses writePixel() for each font pixel (slow but flexible)
 *
 * @see drawText()
 */
void TFT_ST7735::drawChar(uint8_t x, uint8_t y, unsigned char c, uint16_t color,
                          uint16_t bg, uint8_t size_x, uint8_t size_y) {
    if ((x >= _width) ||               // Clip right
        (y >= _height) ||              // Clip bottom
        ((x + 6 * size_x - 1) < 0) ||  // Clip left
        ((y + 8 * size_y - 1) < 0))    // Clip top
        return;

    startWrite();
    for (int8_t i = 0; i < FONT_CHAR_WIDTH; i++) {  // Char bitmap = 5 columns
        uint8_t line = pgm_read_byte(&small_font[(c - 32) * FONT_CHAR_WIDTH + i]);
        for (int8_t j = 0; j < FONT_CHAR_HEIGHT; j++, line >>= 1) {
            if (line & 1) {
                if (size_x == 1 && size_y == 1) {
                    writePixel(x + i, y + j, color);
                } else {
                    fillRect(x + i * size_x, y + j * size_y, size_x, size_y, color);
                }
            } else if (bg != color) {
                if (size_x == 1 && size_y == 1) {
                    writePixel(x + i, y + j, bg);
                } else {
                    fillRect(x + i * size_x, y + j * size_y, size_x, size_y, bg);
                }
            }
        }
    }

    if (bg != color) {  // If opaque, draw vertical line for last column
        if (size_x == 1 && size_y == 1) {
            drawVLine(x + FONT_CHAR_WIDTH, y, FONT_CHAR_HEIGHT, bg);
        } else {
            fillRect(x + FONT_CHAR_WIDTH * size_x, y, size_x,
                     FONT_CHAR_HEIGHT * size_y, bg);
        }
    }
    endWrite();
}

/**
 * @brief Draws a text string with uniform character scaling.
 *
 * Renders a null-terminated string at the specified location. Handles
 * newlines (\\n) and carriage returns (\\r) for multi-line text.
 * Automatically wraps to next line if text exceeds display width.
 *
 * @param[in] x Starting x-coordinate (top-left)
 * @param[in] y Starting y-coordinate (top-left)
 * @param[in] txt Pointer to null-terminated string
 * @param[in] color RGB565 foreground color
 * @param[in] bg RGB565 background color
 * @param[in] size Uniform magnification factor for width and height
 *
 * @return void
 *
 * @note Automatic line wrapping at display width
 * @note Supports \\n (newline) and \\r (ignored)
 * @note Performance: ~50-300 µs per character
 * @note Font: 5×8 pixels per character (before scaling)
 *
 * @code
 * display.drawText(0, 0, "Hello", ST7735_WHITE, ST7735_BLACK, 1);
 * display.drawText(10, 40, "Large\\nText", ST7735_RED, ST7735_BLUE, 2);
 * @endcode
 *
 * @see drawText(), drawChar()
 */
void TFT_ST7735::drawText(uint8_t x, uint8_t y, const char *txt, uint16_t color,
                          uint16_t bg, uint8_t size) {
    drawText(x, y, txt, color, bg, size, size);
}

/**
 * @brief Draws a text string with independent X and Y character scaling.
 *
 * Renders a null-terminated string with separate magnification for width
 * and height. Supports multi-line text with \\n and automatic wrapping.
 *
 * Line wrapping:
 * - If character would exceed display width, cursor moves to next line
 * - Starting x position is maintained for new lines
 * - Line spacing is 8*size_y pixels
 *
 * Special characters:
 * - \\n (ASCII 10): Move to next line
 * - \\r (ASCII 13): Ignored
 *
 * @param[in] x Starting x-coordinate (top-left)
 * @param[in] y Starting y-coordinate (top-left)
 * @param[in] txt Pointer to null-terminated string
 * @param[in] color RGB565 foreground color
 * @param[in] bg RGB565 background color
 * @param[in] size_x Horizontal character magnification
 * @param[in] size_y Vertical character magnification
 *
 * @return void
 *
 * @note Automatic line wrapping at display width boundary
 * @note Performance: ~50-300 µs per character
 * @note Uses drawChar() for each character
 * @note String must be null-terminated
 *
 * @code
 * display.drawText(0, 0, "Width\\nHeight", ST7735_CYAN, ST7735_BLACK, 1, 2);
 * display.drawText(10, 80, "Multi-line\\nScaled Text", ST7735_YELLOW, ST7735_MAGENTA, 2, 2);
 * @endcode
 *
 * @see drawChar(), drawText()
 */
void TFT_ST7735::drawText(uint8_t x, uint8_t y, const char *txt, uint16_t color,
                          uint16_t bg, uint8_t size_x, uint8_t size_y) {
    uint8_t cursor_x = x;
    uint8_t cursor_y = y;

    for (uint8_t i = 0; i < strlen(txt); i++) {
        char c = txt[i];

        if (c == '\n') {                                                 // Newline?
            cursor_x = x;                                                // Reset x to zero,
            cursor_y += size_y * FONT_CHAR_HEIGHT;                       // advance y one line
        } else if (c != '\r') {                                          // Ignore carriage returns
            if ((cursor_x + size_x * (FONT_CHAR_WIDTH + 1)) > _width) {  // Off right?
                cursor_x = x;                                            // Reset x to zero,
                cursor_y += size_y * FONT_CHAR_HEIGHT;                   // advance y one line
            }
            drawChar(cursor_x, cursor_y, c, color, bg, size_x, size_y);
            cursor_x += size_x * (FONT_CHAR_WIDTH + 1);  // Advance x one char
        }
    }
}

/**
 * @brief Draws a rectangle with rounded corners.
 *
 * Draws an outlined rectangle with circular corner arcs. Corner radius
 * is automatically clamped to 1/2 of the smaller dimension.
 *
 * Structure:
 * - Top and bottom horizontal lines (excluding corners)
 * - Left and right vertical lines (excluding corners)
 * - Four quarter-circles for each corner using drawCircleHelper()
 *
 * @param[in] x Top-left x-coordinate
 * @param[in] y Top-left y-coordinate
 * @param[in] w Rectangle width
 * @param[in] h Rectangle height
 * @param[in] r Corner radius (clamped to min(w,h)/2)
 * @param[in] color RGB565 16-bit color
 *
 * @return void
 *
 * @note Radius automatically clamped to prevent errors
 * @note No clipping; shape may extend beyond display
 * @note Performance: ~500-1500 µs depending on size
 * @note Uses drawCircleHelper() for corner arcs
 *
 * @code
 * display.drawRoundRect(10, 10, 50, 50, 8, ST7735_GREEN);    // Square corners
 * display.drawRoundRect(20, 80, 88, 40, 10, ST7735_CYAN);    // Wide corners
 * @endcode
 *
 * @see fillRoundRect(), drawCircleHelper()
 */
void TFT_ST7735::drawRoundRect(uint8_t x, uint8_t y, uint8_t w, uint8_t h, uint8_t r, uint16_t color) {
    int16_t max_radius = ((w < h) ? w : h) / 2;  // 1/2 minor axis
    if (r > max_radius)
        r = max_radius;
    // smarter version
    startWrite();
    drawHLine(x + r, y, w - 2 * r, color);          // Top
    drawHLine(x + r, y + h - 1, w - 2 * r, color);  // Bottom
    drawVLine(x, y + r, h - 2 * r, color);          // Left
    drawVLine(x + w - 1, y + r, h - 2 * r, color);  // Right
    // draw four corners
    drawCircleHelper(x + r, y + r, r, 1, color);
    drawCircleHelper(x + w - r - 1, y + r, r, 2, color);
    drawCircleHelper(x + w - r - 1, y + h - r - 1, r, 4, color);
    drawCircleHelper(x + r, y + h - r - 1, r, 8, color);
    endWrite();
}

/**
 * @brief Draws a filled rectangle with rounded corners.
 *
 * Creates a solid rectangle with circular corner arcs. More efficient
 * than drawRoundRect() + fill combination. Radius is automatically
 * clamped to 1/2 of the smaller dimension.
 *
 * Rendering strategy:
 * 1. Fills main rectangular body (width full, height reduced)
 * 2. Uses fillCircleHelper() for top and bottom corner regions
 * 3. Applies 4-way symmetry for corner filling
 *
 * @param[in] x Top-left x-coordinate
 * @param[in] y Top-left y-coordinate
 * @param[in] w Rectangle width
 * @param[in] h Rectangle height
 * @param[in] r Corner radius (clamped to min(w,h)/2)
 * @param[in] color RGB565 16-bit color
 *
 * @return void
 *
 * @note Radius automatically clamped to prevent errors
 * @note No clipping; shape may extend beyond display
 * @note Performance: ~1000-3000 µs depending on size
 * @note Uses fillCircleHelper() for corner blending
 *
 * @code
 * display.fillRoundRect(0, 0, 128, 160, 12, ST7735_BLUE);       // Full screen
 * display.fillRoundRect(30, 50, 68, 60, 5, ST7735_RED);         // Centered panel
 * @endcode
 *
 * @see drawRoundRect(), fillCircleHelper()
 */
void TFT_ST7735::fillRoundRect(uint8_t x, uint8_t y, uint8_t w, uint8_t h, uint8_t r, uint16_t color) {
    int16_t max_radius = ((w < h) ? w : h) / 2;  // 1/2 minor axis
    if (r > max_radius)
        r = max_radius;
    // smarter version
    startWrite();
    fillRect(x + r, y, w - 2 * r, h, color);
    // draw four corners
    fillCircleHelper(x + w - r - 1, y + r, r, 1, h - 2 * r - 1, color);
    fillCircleHelper(x + r, y + r, r, 2, h - 2 * r - 1, color);
    endWrite();
}
