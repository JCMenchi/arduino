#ifndef _CH1115_DISPLAY_H
#define _CH1115_DISPLAY_H

#include "Arduino.h"

// Default I2C address is 0x3C or 0x3D if SA0 input is set high
#define CH1115_I2C_ADDRESS 0x3C
#define CH1115_DEVICE_ID 0x15

// Display Pixel colours  definition
#define CH1115_WHITE_COLOR 0
#define CH1115_BLACK_COLOR 1
#define CH1115_INVERSE_COLOR 2

#define CH1115_OFF 0
#define CH1115_ON 1

// Scrolling definition
#define CH1115_SCROLL_RIGHT 0
#define CH1115_SCROLL_LEFT 1
//  00 -> Continuous horizontal/vertical scroll(default)
//      01 -> Single Screen scroll
//      1X -> 1 Column scroll mode
#define CH1115_SCROLL_OFF 0xFF
#define CH1115_SCROLL_CONTINUOUS 0x00
#define CH1115_SCROLL_ONCE 0x01
#define CH1115_SCROLL_ONE_COLUMN 0x02
// define values for frame number as defined in datasheet
#define CH1115_SCROLL_2FRAMES 0x07
#define CH1115_SCROLL_3FRAMES 0x04
#define CH1115_SCROLL_4FRAMES 0x05
#define CH1115_SCROLL_5FRAMES 0x06
#define CH1115_SCROLL_6FRAMES 0x00
#define CH1115_SCROLL_32FRAMES 0x01
#define CH1115_SCROLL_64FRAMES 0x02
#define CH1115_SCROLL_128FRAMES 0x03

// uncomment to active double buffer
#define BACK_BUFFER

class CH1115Display
{
public:
    CH1115Display(uint8_t w, uint8_t h);
    ~CH1115Display();

    void init(uint8_t contrast = 0x80);

    // Basic screen setup
    void enable(uint8_t on);
    void invert(uint8_t on);
    void flip(uint8_t on);
    void contrast(uint8_t contrast);

    // Special Effects
    void breathingEffect(uint8_t on);

    void scrollArea(uint8_t startPage, uint8_t endPage, uint8_t startCol, uint8_t endCol, uint8_t dir, uint8_t nbFrame);
    void scroll(uint8_t mode);
    
    // Drawing
    void drawScreen(uint8_t pattern);

    void drawString(uint8_t x, uint8_t y, const char *pText);
    void drawPixel(uint8_t x, uint8_t y, uint8_t color);
    void drawLine(uint8_t x0, uint8_t y0, uint8_t x1, uint8_t y1, uint8_t color);
    void drawSprite(uint8_t x, uint8_t y, uint8_t w, uint8_t h, const uint8_t *data);
    
#ifdef BACK_BUFFER
    void drawPixelBuf(uint8_t x, uint8_t y, uint8_t color);
    void drawStringBuf(uint8_t x, uint8_t y, const char *pText);
#endif

private:

#ifdef BACK_BUFFER
    class PageBuffer
    {
    public:
        PageBuffer(uint8_t w)
        {
            width = w;
            screenBuffer = new uint8_t[width];
            height = 8;
            xoffset = 0;
            yoffset = 0;
        }
        ~PageBuffer()
        {
            delete[] screenBuffer;
        }

        void clearBuffer()
        {
            memset(this->screenBuffer, 0x00, this->width);
        }

        uint8_t *screenBuffer;
        uint8_t width;
        uint8_t height;
        uint8_t xoffset;
        uint8_t yoffset;
    };
    void update();
    PageBuffer *_buffer;

    void drawChar(uint8_t x, uint8_t y, unsigned char c, uint8_t color, uint8_t bg);

#endif

    void setAddress(uint8_t x, uint8_t y);
    
    void send_data(uint8_t data);
    void start_data(uint8_t byte);
    void add_data(uint8_t byte);
    void stop_data(uint8_t byte);
    void send_command(uint8_t command);

    uint8_t _height;
    uint8_t _width;
};

#endif
