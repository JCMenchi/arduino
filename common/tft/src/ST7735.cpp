#include "ST7735.h"

#include "SPIManager.h"
#include "bitmap_font.h"

#include <string.h>

TFT_ST7735::TFT_ST7735(int8_t dc, int8_t rst, SPIManager *spi)
    : _portb_rst_pin(rst), _portb_dc_pin(dc) 
{
    _spi = spi;
    _width = 128;
    _height = 160;

    _nbStartWrite = 0;
}

// SCREEN INITIALIZATION ***************************************************
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

#include <Arduino.h>

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

/*
  See datasheet p77
    9.11.4 Frame Data Write Direction According to the MADCTL Parameters (MV, MX
  and MY)
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

void TFT_ST7735::enableDisplay(uint8_t enable) {
    startWrite();
    sendCommand(enable ? ST7735_DISPON : ST7735_DISPOFF);
    endWrite();
}

void TFT_ST7735::enableTearing(uint8_t enable) {
    startWrite();
    sendCommand(enable ? ST7735_TEON : ST7735_TEOFF);
    endWrite();
}

void TFT_ST7735::enableSleep(uint8_t enable) {
    startWrite();
    sendCommand(enable ? ST7735_SLPIN : ST7735_SLPOUT);
    endWrite();
}

void TFT_ST7735::invertDisplay(bool i) {
    startWrite();
    sendCommand(i ? ST7735_INVON : ST7735_INVOFF);
    endWrite();
}

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

uint16_t TFT_ST7735::color565(uint8_t red, uint8_t green, uint8_t blue) {
    return ((red & 0xF8) << 8) | ((green & 0xFC) << 3) | (blue >> 3);
}

void TFT_ST7735::startWrite(void) {
    _spi->begin();
    _nbStartWrite++;
}

void TFT_ST7735::endWrite(void) {
    _nbStartWrite--;
    if (_nbStartWrite == 0) {
        _spi->end();
    }
}

/*
  SPI star end end are done externally
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

void TFT_ST7735::writePixel(uint8_t x, uint8_t y, uint16_t color) {
    setAddrWindow(x, y, 1, 1);
    // TFT display communicate in BigEndian
    _spi->send(color >> 8);
    _spi->send(color & 0xFF);
}

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

void TFT_ST7735::fillCircle(uint8_t x0, uint8_t y0, uint8_t r, uint16_t color) {
    drawVLine(x0, y0 - r, 2 * r + 1, color);
    fillCircleHelper(x0, y0, r, 3, 0, color);
}

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

void TFT_ST7735::drawTriangle(uint8_t x0, uint8_t y0, uint8_t x1, uint8_t y1,
                              uint8_t x2, uint8_t y2, uint16_t color) {
    drawLine(x0, y0, x1, y1, color);
    drawLine(x1, y1, x2, y2, color);
    drawLine(x2, y2, x0, y0, color);
}

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

void TFT_ST7735::drawChar(uint8_t x, uint8_t y, unsigned char c, uint16_t color,
                          uint16_t bg, uint8_t size) {
    drawChar(x, y, c, color, bg, size, size);
}

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

void TFT_ST7735::drawText(uint8_t x, uint8_t y, const char *txt, uint16_t color,
                          uint16_t bg, uint8_t size) {
    drawText(x, y, txt, color, bg, size, size);
}

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
