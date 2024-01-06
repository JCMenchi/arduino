#include "CH1115Display.h"
#include "bitmap_font.h"

#include <avr/pgmspace.h>

#include <Wire.h>

// #define HAS_SERIAL

#define CH1115_Swap(a, b) \
    {                     \
        uint8_t t = a;    \
        a = b;            \
        b = t;            \
    }

CH1115Display::CH1115Display(uint8_t w, uint8_t h)
{
    _width = w;
    _height = h;
#ifdef BACK_BUFFER
    _buffer = new PageBuffer(_width);
#endif
}

CH1115Display::~CH1115Display()
{
#ifdef BACK_BUFFER
    delete _buffer;
#endif
}

#define CH1115_STATUS 0x15

// Possible Wire error code for endTransmission()
//   0 .. success
//   1 .. length to long for buffer
//   2 .. address send, NACK received
//   3 .. data send, NACK received
//   4 .. other twi error (lost bus arbitration, bus error, ..)
//   5 .. timeout
void CH1115Display::init(uint8_t contrast)
{
    // Init I2C com
    Wire.begin(CH1115_I2C_ADDRESS);
    Wire.setClock(400000L);

    // Check display type
    Wire.beginTransmission(CH1115_I2C_ADDRESS);
    uint8_t rc = Wire.endTransmission();
    if (rc != 0)
    {
#ifdef HAS_SERIAL
        Serial.print("I2C write error code: ");
        Serial.println(rc);
#endif
        _width = 0;
        _height = 0;
        return;
    }

    uint8_t nb = Wire.requestFrom(CH1115_I2C_ADDRESS, 1);
    if (nb != 1)
    {
#ifdef HAS_SERIAL
        Serial.print("I2C read error nbread: ");
        Serial.println(nb);
#endif
        _width = 0;
        _height = 0;
        return;
    }

    uint8_t u = Wire.read();
    if ((u & CH1115_DEVICE_ID) == 0x15)
    {
#ifdef HAS_SERIAL
        Serial.print("Connection to CH1115 display OK.");
#endif
    }
    else
    {
#ifdef HAS_SERIAL
        Serial.print("Screen status register: ");
        Serial.println(u, 16);
#endif
        _width = 0;
        _height = 0;
        return;
    }

    // set normal display mode
    send_command(0xA4);
    // turn on
    send_command(0xAF);

    this->contrast(contrast);
}

// 18. Display OFF/ON: (AEH - AFH)
// Alternatively turns the display on and off.
// When D = “L”, Display OFF OLED. (POR)  0xAE
// When D = “H”, Display ON OLED. 0xAF
// When the display OFF command is executed, power saver mode will be entered.
// Sleep mode:
// This mode stops every operation of the OLED display system, and can reduce current consumption nearly to a static current
// value if no access is made from the microprocessor. The internal status in the sleep mode is as follows:
// 1)Stops the oscillator circuit and DC-DC circuit.
// 2)Stops the OLED drive and outputs Hz as the segment/common driver output.
// 3)Holds the display data and operation mode provided before the start of the sleep mode.
// 4)The MPU can access to the built-in display RAM.
void CH1115Display::enable(uint8_t on)
{
    send_command(on ? 0xAF : 0xAE);
}

// 4. Additional Horizontal Scroll Setup: (Three Bytes Command)
// This command consists of 3 consecutive bytes to set up the horizontal scroll parameters. It determined the scrolling start
// column position and end column position. The end column position must be larger than start column position.
//
// - Additional Horizontal Scroll Setup Mode Set: (24H)
//       | D7 | D6 | D5 | D4 | D3 | D2 | D1 | D0 |
//       |  0 |  0 |  1 |  0 |  0 |  1 |  0 |  0 | -> 0x24
//       | A7 | A6 | A5 | A4 | A3 | A2 | A1 | A0 | -> start colum 0 to 127
//       | B7 | B6 | B5 | B4 | B3 | B2 | B1 | B0 | -> end column  0 to 127
//
// 5. Horizontal Scroll Setup: (Four Bytes Command)
// This command consists of 4 consecutive bytes to set up the horizontal scroll parameters. It determined the number of
// horizontal scroll per step , scrolling start page, time interval and end page.
// Before issuing this command, the horizontal scroll must be deactivated (2EH). Otherwise, ram content may be corrupted.
//
// - Horizontal Scroll Setup Mode Set: (26H - 27H)
//     | D7 | D6 | D5 | D4 | D3 | D2 | D1 | D0 |
//     |  0 |  0 |  1 |  0 |  0 | 1  |  1 |  D | D is direction 0 -> right, 1 -> left
//     |  * |  * |  * |  * |  * | A2 | A1 | A0 | start page address 0 to 7
//     |  * |  * |  * |  * |  * | B2 | B1 | B0 | time interval number of frame
//     |  * |  * |  * |  * |  * | C2 | C1 | C0 | end page address   0 to 7
//
//           000   6 frames(POR)
//           001  32 frames
//           010  64 frames
//           011 128 frames
//           100   3 frames
//           101   4 frames
//           110   5 frames
//           111   2 frames
void CH1115Display::scrollArea(uint8_t startPage, uint8_t endPage, uint8_t startCol, uint8_t endCol, uint8_t dir, uint8_t nbFrame)
{
    // Additional Horizontal Scroll Setup
    send_command(0x24);
    send_command(startCol ? startCol : 0);
    send_command(endCol ? endCol : 127);

    // Horizontal Scroll Setup
    if (dir == CH1115_SCROLL_LEFT)
    {
        send_command(0x27);
    }
    else
    {
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
// The scroll is end after the 2EH command is written. 0x2B2F2E scroll one column
//
// 7. Set Deactivate /Activate Horizontal Scroll: (2EH - 2FH)
// Stop or start motion of horizontal scrolling. This command should only be issued after horizontal scroll setup parameters
// (24H/26H/27H/28H/29H/2CH/2DH) are defined.
//
// When D(bit0)=”L”, Stop motion of horizontal scroll. (POR) 0x2E
// When D(bit0)=”H”, Start motion of horizontal scroll. 0x2F
// Note: The following actions are prohibited after the horizontal scroll is activated
//  Changing additional horizontal scroll setup parameters.
//  Changing horizontal scroll setup parameters.
//  Changing scroll mode setup parameters.
// After the deactivate horizontal scroll issued, the display of screen is reset to original status.
void CH1115Display::scroll(uint8_t mode)
{
    if (mode == CH1115_SCROLL_OFF)
    {
        send_command(0x2E);
    }
    else if (mode == CH1115_SCROLL_CONTINUOUS)
    {
        send_command(0x28);
        send_command(0x2F);
    }
    else if (mode == CH1115_SCROLL_ONCE)
    {
        send_command(0x29);
        send_command(0x2F);
    }
    else if (mode == CH1115_SCROLL_ONE_COLUMN)
    {
        send_command(0x2A);
        send_command(0x2F);
    }
}

// 10. Set Contrast Control Register: (Double Bytes Command)
// This command is to set contrast setting of the display. The chip has 256 contrast steps from 00 to FF. The segment output
// current increases as the contrast step value increases.
// Segment output current setting: ISEG = (α+1)/256 X IREF X scale factor
// Where: α is contrast step; Scale factor = 16.
//
// - The Contrast Control Mode Set: (81H)
// When this command is input, the contrast data register set command becomes enabled. Once the contrast control mode
// has been set, no other command except for the contrast data register command can be used. Once the contrast data set
// command has been used to set data into the register, then the contrast control mode is released.
//
// - Contrast Data Register Set: (00H – FFH)
// By using this command to set eight bits of data to the contrast data register; the OLED segment output assumes one of the
// 256 current levels.
// When this command is input, the contrast control mode is released after the contrast data register has been set.//
// When the contrast control function is not used, set the D7 - D0 to 10000000.
void CH1115Display::contrast(uint8_t contrast)
{
    send_command(0x81);     // Contrast Control Mode Set
    send_command(contrast); // Contrast Data Register Set:
}

// 20. Set Common Output Scan Direction: (C0H - C8H)
// This command sets the scan direction of the common output allowing layout flexibility in OLED module design. In addition,
// the display will have immediate effect once this command is issued. That is, if this command is sent during normal display,
// the graphic display will be vertically flipped.
// When D (bit3) = “L”, Scan from COM0 to COM [N -1]. (POR)
// When D (bit3) = “H”, Scan from COM [N -1] to COM0.
//
// 12. Set Segment Re-map: (A0H - A1H)
// Change the relationship between RAM column address and segment driver. The order of segment driver output pads can be
// reversed by software. This allows flexible IC layout during OLED module assembly. For details, refer to the column address
// section of Figure. 18. When display data is written or read, the column address is incremented by 1 as shown in Figure.18.
//
// When ADC (bit0) = “L”, the right rotates (normal direction). (POR)
// When ADC (bit0) = “H”, the left rotates (reverse direction).
void CH1115Display::flip(uint8_t on)
{
    if (on)
    {
        send_command(0xC8); // Common Output Scan Direction
        send_command(0xA1); // SEG REMAP
    }
    else
    {
        send_command(0xC0); // Common Output Scan Direction
        send_command(0xA0); // SEG REMAP
    }
}

// 3. Set Breathing Display Effect: (Double Bytes Command)
// This command set Breathing Display Effect ON/OFF and Time Interval.
// -BYTE1: Breathing Light Set: (23H)
//
// - BYTE2
//       D7   | D6 | D5 | D4 | D3 | D2 | D1 | D0
//     ON/OFF |    |    | A4 | A3 | A2 | A1 | A0
//
//    When D7 = ”L”, Breathing Light OFF. (POR)
//    When D7 = ”H”, Breathing Light ON.
//
//  Breathing Display Effect Maximum Brightness Adjust Set: (A4 – A3)
//    00 -> 256, 01 -> 128, 10 -> 64, 11 -> 32
//
//  Breathing Display Effect Time Interval Set: (A2 – A0)
//    A2-A0 define number of frame 000 -> 1 frame, 111 -> 8 frames
void CH1115Display::breathingEffect(uint8_t on)
{
    send_command(0x23);
    // when ON, maxBrightness=256, 3 frames
    send_command(on ? 0x82 : 0x00);
}

// 15. Set Normal/Reverse Display: (A6H -A7H)
// Reverses the display ON/OFF status without rewriting the contents of the display data RAM.
// When D = “L”, the RAM data is high, being OLED ON potential (normal display). (POR) 0xA6
// When D = “H”, the RAM data is low, being OLED ON potential (reverse display)  0xA7
void CH1115Display::invert(uint8_t on)
{
    send_command(on ? 0xA7 : 0xA6);
}

// CH1115 display addressing
// Display is divided in pages each page has a height of 8 pixels

// Specifies column address of display RAM. Divide the column address into 4 higher bits and 4 lower bits. Set each of them into
// successions. When the microprocessor repeats to access to the display RAM, the column address counter is incremented
// during each access until address 127 is accessed. The page address is not changed during this time.
#define CH1115_SET_COLADD_LSB 0x00 // 1. Set Lower Column Address: (00H - 0FH)
#define CH1115_SET_COLADD_MSB 0x10 // 2. Set Higher Column Address: (10H – 1FH)
// 19. Set Page Address: (B0H - B7H)
// Specifies page address to load display RAM data to page address register. Any RAM data bit can be accessed when its
// page address and column address are specified. The display remains unchanged even when the page address is changed.
// 4 lower bits are used to select the page from 0 to 7
#define CH1115_SET_PAGEADD 0xB0
void CH1115Display::setAddress(uint8_t x, uint8_t y)
{
    Wire.beginTransmission(CH1115_I2C_ADDRESS);
    // start read modify write
    Wire.write(0x80); // command start D/C bit is 0, continuous (1000 0000)
    Wire.write(CH1115_SET_PAGEADD | (y / 8));
    Wire.write(0x80); // command start D/C bit is 0, continuous (1000 0000)
    Wire.write(CH1115_SET_COLADD_LSB | (x & 0x0F));
    Wire.write(0x00); // command start D/C bit is 0, no continuous (0000 0000)
    Wire.write(CH1115_SET_COLADD_MSB | ((x & 0xF0) >> 4));
    Wire.endTransmission(); // switch to reception mode
}

void CH1115Display::drawScreen(uint8_t pattern)
{
    for (uint8_t page = 0; page < (_height / 8); page++)
    {
        setAddress(0, page * 8);

        for (uint8_t i = 0; i < _width; i++)
        {
            send_data(pattern);
        }
    }
}

void CH1115Display::drawPixel(uint8_t x, uint8_t y, uint8_t colour)
{
    if ((x >= this->_width) || (y >= this->_height))
    {
        return;
    }

    setAddress(x, y);

    Wire.beginTransmission(CH1115_I2C_ADDRESS);
    // start read modify write
    Wire.write(0x80); // command D/C bit is 0, with continuation (1000 0000)
    Wire.write(0xE0);
    // request data
    Wire.write(0x40);
    Wire.endTransmission(); // switch to reception mode

    // wait for answer
    Wire.requestFrom(CH1115_I2C_ADDRESS, 2);
    int r = Wire.read(); // dummy bit start read response
    r = Wire.read();

    // update bit
    Wire.beginTransmission(CH1115_I2C_ADDRESS);
    Wire.write(0xC0); // data followed by command D/C bit is 0, continuation is 1 (1100 0000)

    switch (colour)
    {
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
    Wire.write(r);

    // stop read modify write
    Wire.write(0x00); // command D/C bit is 0 (0000 0000)
    Wire.write(0xEE);
    Wire.endTransmission();
}

void CH1115Display::drawLine(uint8_t x0, uint8_t y0, uint8_t x1, uint8_t y1, uint8_t color)
{
    uint8_t steep = abs(y1 - y0) > abs(x1 - x0);
    if (steep)
    {
        CH1115_Swap(x0, y0);
        CH1115_Swap(x1, y1);
    }

    if (x0 > x1)
    {
        CH1115_Swap(x0, x1);
        CH1115_Swap(y0, y1);
    }

    uint8_t dx, dy;
    dx = x1 - x0;
    dy = abs(y1 - y0);

    int8_t err = dx / 2;
    int8_t ystep = (y0 < y1) ? 1 : -1;

    for (; x0 <= x1; x0++)
    {
        if (steep)
        {
            drawPixel(y0, x0, color);
        }
        else
        {
            drawPixel(x0, y0, color);
        }
        err -= dy;
        if (err < 0)
        {
            y0 += ystep;
            err += dx;
        }
    }
}

void CH1115Display::drawString(uint8_t x, uint8_t y, const char *pText)
{
    setAddress(x, y);

    while (*pText != '\0')
    {
        // draw
        char c = pText[0];
        uint8_t line = pgm_read_byte(small_font + ((c - 32) * FONT_CHAR_WIDTH));
        start_data(line);
        for (int8_t i = 1; i < FONT_CHAR_WIDTH; i++)
        {
            line = pgm_read_byte(small_font + (c * FONT_CHAR_WIDTH) + i);
            add_data(line);
        }

        // draw empty vert line to separate char
        stop_data(0x00);

        pText++;
    }
}

void CH1115Display::drawSprite(uint8_t x, uint8_t y, uint8_t sw, uint8_t sh, const uint8_t *data)
{
    for (uint8_t ty = 0; ty < sh; ty = ty + 8)
    {
        if (y + ty >= _height)
        {
            continue;
        }
        setAddress(x, ty);

        for (uint8_t tx = 0; tx < sw; tx++)
        {
            if (x + tx >= _width)
            {
                continue;
            }
            uint16_t offset = (sw * (ty / 8)) + tx;
            send_data(pgm_read_byte(&data[offset]));
        }
    }
}

void CH1115Display::send_command(uint8_t command)
{
    Wire.beginTransmission(CH1115_I2C_ADDRESS);
    Wire.write(0x00); // command start D/C bit is 0 and no continuation (0000 0000)
    uint8_t rc = Wire.write(command);
    rc = Wire.endTransmission(true);

#ifdef HAS_SERIAL
    if (rc != 0)
    {
        Serial.print("I2C end: ");
        Serial.println(rc);
    }
#endif
}

void CH1115Display::send_data(uint8_t byte)
{
    Wire.beginTransmission(CH1115_I2C_ADDRESS);
    Wire.write(0x40); // data prefix byte D/C bit is 1 (0100 0000)
    uint8_t rc = Wire.write(byte);
    if (rc != 1)
    {
#ifdef HAS_SERIAL
        Serial.println("I2C write nb data");
#endif
        return;
    }
    rc = Wire.endTransmission(true);

#ifdef HAS_SERIAL
    if (rc != 0)
    {
        Serial.print("I2C end: ");
        Serial.println(rc);
    }
#endif
}

void CH1115Display::start_data(uint8_t byte)
{
    Wire.beginTransmission(CH1115_I2C_ADDRESS);
    Wire.write(0xC0); // data prefix byte D/C bit is 1 (1100 0000)
    Wire.write(byte);
}

void CH1115Display::add_data(uint8_t byte)
{
    Wire.write(0xC0); // data prefix byte D/C bit is 1 (1100 0000)
    Wire.write(byte);
}

void CH1115Display::stop_data(uint8_t byte)
{
    Wire.write(0x40); // data prefix byte D/C bit is 1 (0100 0000)
    Wire.write(byte);
    uint8_t rc = Wire.endTransmission(true);

#ifdef HAS_SERIAL
    if (rc != 0)
    {
        Serial.print("I2C end: ");
        Serial.println(rc);
    }
#endif
}

#ifdef BACK_BUFFER
void CH1115Display::drawPixelBuf(uint8_t x, uint8_t y, uint8_t colour)
{
    if ((x >= this->_buffer->width) || (y >= this->_buffer->height))
    {
        return;
    }
    uint16_t offset = (this->_buffer->width * (y / 8)) + x;
    switch (colour)
    {
    case CH1115_WHITE_COLOR:
        this->_buffer->screenBuffer[offset] |= (1 << (y & 7));
        break;
    case CH1115_BLACK_COLOR:
        this->_buffer->screenBuffer[offset] &= ~(1 << (y & 7));
        break;
    case CH1115_INVERSE_COLOR:
        this->_buffer->screenBuffer[offset] ^= (1 << (y & 7));
        break;
    }
}

void CH1115Display::update()
{
    uint8_t tx, ty;
    uint16_t offset = 0;
    uint8_t page = this->_buffer->yoffset / 8;

    for (ty = 0; ty < this->_buffer->height; ty = ty + 8)
    {
        if (this->_buffer->yoffset + ty < 0 || this->_buffer->yoffset + ty >= _height)
        {
            continue;
        }

        setAddress(this->_buffer->xoffset, page * 8);
        page++;

        for (tx = 0; tx < this->_buffer->width; tx++)
        {
            if (this->_buffer->xoffset + tx < 0 || this->_buffer->xoffset + tx >= _width)
            {
                continue;
            }
            offset = (this->_buffer->width * (ty / 8)) + tx;
            send_data(this->_buffer->screenBuffer[offset++]);
        }
    }
}

#endif