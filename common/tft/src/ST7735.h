#ifndef _TFT_ST7735H_
#define _TFT_ST7735H_

#include <avr/common.h>
#include <stdlib.h>

#include <gpio.h>

#define ST7735_TFTWIDTH_128 128
#define ST7735_TFTHEIGHT_160 160

#define ST_CMD_DELAY 0x80  // special signifier for command lists

#define ST7735_NOP 0x00
#define ST7735_SWRESET 0x01
#define ST7735_RDDID 0x04
#define ST7735_RDDST 0x09
#define ST7735_RDDMADCTL 0x0B
#define ST7735_RDDCOLMOD 0x0C
#define ST7735_RDDIM 0x0D

#define ST7735_SLPIN 0x10
#define ST7735_SLPOUT 0x11
#define ST7735_PTLON 0x12
#define ST7735_NORON 0x13

#define ST7735_INVOFF 0x20
#define ST7735_INVON 0x21
#define ST7735_DISPOFF 0x28
#define ST7735_DISPON 0x29
#define ST7735_CASET 0x2A
#define ST7735_RASET 0x2B
#define ST7735_RAMWR 0x2C
#define ST7735_RAMRD 0x2E

#define ST7735_PTLAR 0x30
#define ST7735_TEOFF 0x34
#define ST7735_TEON 0x35
#define ST7735_MADCTL 0x36
#define ST7735_COLMOD 0x3A

#define ST7735_MADCTL_MY 0x80
#define ST7735_MADCTL_MX 0x40
#define ST7735_MADCTL_MV 0x20
#define ST7735_MADCTL_ML 0x10
#define ST7735_MADCTL_RGB 0x00

#define ST7735_RDID1 0xDA
#define ST7735_RDID2 0xDB
#define ST7735_RDID3 0xDC
#define ST7735_RDID4 0xDD

// Some ready-made 16-bit ('565') color settings:
#define ST7735_BLACK 0x0000
#define ST7735_WHITE 0xFFFF
#define ST7735_RED 0xF800
#define ST7735_GREEN 0x07E0
#define ST7735_BLUE 0x001F
#define ST7735_CYAN 0x07FF
#define ST7735_MAGENTA 0xF81F
#define ST7735_YELLOW 0xFFE0
#define ST7735_ORANGE 0xFC00

// Some register settings
#define ST7735_MADCTL_BGR 0x08
#define ST7735_MADCTL_MH 0x04

#define ST7735_FRMCTR1 0xB1
#define ST7735_FRMCTR2 0xB2
#define ST7735_FRMCTR3 0xB3
#define ST7735_INVCTR 0xB4
#define ST7735_DISSET5 0xB6

#define ST7735_PWCTR1 0xC0
#define ST7735_PWCTR2 0xC1
#define ST7735_PWCTR3 0xC2
#define ST7735_PWCTR4 0xC3
#define ST7735_PWCTR5 0xC4
#define ST7735_VMCTR1 0xC5

#define ST7735_PWCTR6 0xFC

#define ST7735_GMCTRP1 0xE0
#define ST7735_GMCTRN1 0xE1

class SPIManager;

class TFT_ST7735 {
   public:
    TFT_ST7735(int8_t dc, int8_t rst, SPIManager* spi);

    void init();

    void setRotation(uint8_t m);

    void enableDisplay(uint8_t enable);
    void enableTearing(uint8_t enable);
    void enableSleep(uint8_t enable);
    void invertDisplay(bool i);

    void fillRect(uint8_t x, uint8_t y, uint8_t w, uint8_t h, uint16_t color);

    void drawPixel(uint8_t x, uint8_t y, uint16_t color) {
        fillRect(x, y, 1, 1, color);
    }

    uint32_t readPixel(uint8_t x, uint8_t y);

    void drawHLine(uint8_t x, uint8_t y, uint8_t w, uint16_t color) {
        fillRect(x, y, w, 1, color);
    }

    void drawVLine(uint8_t x, uint8_t y, uint8_t h, uint16_t color) {
        fillRect(x, y, 1, h, color);
    }

    void drawRGBBitmap(uint8_t x, uint8_t y, uint16_t* pcolors, uint8_t w, uint8_t h);

    uint16_t color565(uint8_t r, uint8_t g, uint8_t b);

    void drawLine(uint8_t x0, uint8_t y0, uint8_t x1, uint8_t y1, uint16_t color);
    void drawCircle(uint8_t x0, uint8_t y0, uint8_t r, uint16_t color);
    void drawCircleHelper(uint8_t x0, uint8_t y0, uint8_t r, uint8_t corners, uint16_t color);
    void fillCircle(uint8_t x0, uint8_t y0, uint8_t r, uint16_t color);
    void fillCircleHelper(uint8_t x0, uint8_t y0, uint8_t r, uint8_t corners, int16_t delta, uint16_t color);
    void drawTriangle(uint8_t x0, uint8_t y0, uint8_t x1, uint8_t y1, uint8_t x2, uint8_t y2, uint16_t color);
    void fillTriangle(uint8_t x0, uint8_t y0, uint8_t x1, uint8_t y1, uint8_t x2, uint8_t y2, uint16_t color);
    void drawRoundRect(uint8_t x0, uint8_t y0, uint8_t w, uint8_t h, uint8_t radius, uint16_t color);
    void fillRoundRect(uint8_t x0, uint8_t y0, uint8_t w, uint8_t h, uint8_t radius, uint16_t color);

    void drawChar(uint8_t x, uint8_t y, unsigned char c, uint16_t color, uint16_t bg, uint8_t size = 1);
    void drawChar(uint8_t x, uint8_t y, unsigned char c, uint16_t color, uint16_t bg, uint8_t size_x, uint8_t size_y);

    void drawText(uint8_t x, uint8_t y, const char* txt, uint16_t color, uint16_t bg, uint8_t size = 1);
    void drawText(uint8_t x, uint8_t y, const char* txt, uint16_t color, uint16_t bg, uint8_t size_x, uint8_t size_y);

    // Chip select and/or hardware SPI transaction start as needed:
    void startWrite(void);
    // Chip deselect and/or hardware SPI transaction end as needed:
    void endWrite(void);

    uint8_t readcommand8(uint8_t c);
    uint16_t readcommand16(uint8_t c);
    uint32_t readcommand32(uint8_t c);

   private:
    void writePixel(uint8_t x, uint8_t y, uint16_t color);

    void setAddrWindow(uint8_t x, uint8_t y, uint8_t w, uint8_t h);

    void SPI_DC_HIGH(void) {
        //digitalWrite(_portb_dc_pin, HIGH);
        GPIO_SET_HIGH(B, _portb_dc_pin);
    }

    void SPI_DC_LOW(void) {
        //digitalWrite(_portb_dc_pin, LOW);
        GPIO_SET_LOW(B, _portb_dc_pin);
    }

    void sendCommand(uint8_t commandByte, uint8_t* dataBytes = NULL, uint8_t numDataBytes = 0);

    void sendCommandFromPGM(uint8_t commandByte, const uint8_t* dataBytes, uint8_t numDataBytes);

    void displayInit(const uint8_t* addr);

    SPIManager* _spi;
    int8_t _portb_rst_pin;  ///< Reset pin # (or -1)
    int8_t _portb_dc_pin;   ///< Data/command pin #

    int16_t _width;
    int16_t _height;

    uint8_t _nbStartWrite;
};

#endif  // _TFT_ST7735H_
