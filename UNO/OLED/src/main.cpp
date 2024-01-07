#include <Arduino.h>

#include <CH1115Display.h>

#include "sprites.h"

CH1115Display ch1115(128, 64);

const uint8_t MAX_COLUMN = 21;
const uint8_t MAX_LINE = 8;
uint8_t textpos = 0;
uint8_t curline = 0;
uint8_t escpos = 0;
char line_buffer[MAX_COLUMN + 1];
char escape_buffer[10];

void reset_line() {
  memset(line_buffer, ' ', MAX_COLUMN + 1);
  line_buffer[0] = '>';
  line_buffer[MAX_COLUMN] = 0;
  textpos = 2;
}

uint8_t xcursor = 60;
uint8_t ycursor = 0;
uint8_t xprev = 1;
uint8_t yprev = 1;

void drawScene() {
  unsigned long start = millis();

  //ch1115.drawScreen(0x00, false);
  // ch1115.drawString(xcursor, ycursor, "Welcome");
  
  //ch1115.drawLine(32,0,32,63, CH1115_WHITE_COLOR);
  //ch1115.drawLine(64,0,64,63, CH1115_WHITE_COLOR);
  //ch1115.drawLine(0,32,127,32, CH1115_WHITE_COLOR);

  //ch1115.drawLine(64,0,127,63, CH1115_WHITE_COLOR);
  //ch1115.drawLine(64,63,127,0, CH1115_WHITE_COLOR);

  // ch1115.drawLine(0,0,16,32, CH1115_WHITE_COLOR);
  // ch1115.drawLine(16,32,64,0, CH1115_WHITE_COLOR);
  // ch1115.drawLine(0,63,16,32, CH1115_WHITE_COLOR);

  if (ycursor != yprev) {
    // range to clear
    uint8_t p1 = yprev/8;
    uint8_t ymax = yprev + 4*8;
    uint8_t p2 = ymax/8;
    for(uint8_t p = p1; p <= p2; p++) {
      ch1115.drawPage(p, 0x00);
    }
    for(uint8_t p = 0; p < 4; p++) {
      for(uint8_t i = 0; i < 9; i++) {
        ch1115.drawSprite(12+i*12, p*8 + ycursor, SPRITE_WIDTH, SPRITE_HEIGHT, (p<2)?alien2:alien);
      }
    }
    yprev = ycursor;
  }
  
  ch1115.drawSprite(16, 48, SHELTER_WIDTH, SPRITE_HEIGHT, shelter);
  ch1115.drawSprite(16+14+27, 48, SHELTER_WIDTH, SPRITE_HEIGHT, shelter);
  ch1115.drawSprite(16+14+27+14+27, 48, SHELTER_WIDTH, SPRITE_HEIGHT, shelter);

  if (xcursor != xprev) {
    ch1115.drawPage(7, 0x00);
    ch1115.drawSprite(xcursor, 56, SPRITE_WIDTH, SPRITE_HEIGHT, spaceship);
    xprev = xcursor;
  }

  unsigned long end = millis();
  Serial.print(F("Frame refresh in (ms): "));
  Serial.println(end-start);
}

void setup()
{
  reset_line();
  Serial.begin(115200);

  // init OLED display
  ch1115.init(0x01);
  ch1115.flip(CH1115_ON);
  ch1115.drawScreen(0x00);

  drawScene();
}

void esccommand(const char* escape_buffer) {

    if (escape_buffer[0] == '[') {
      // arrow
      if (escape_buffer[1] == 'C') {
        // right
        xcursor += 1;
      } else if (escape_buffer[1] == 'D') {
        // left
        xcursor = (xcursor?xcursor-1:0);
      } else if (escape_buffer[1] == 'A') {
        // up
        ycursor = (ycursor?ycursor-1:0);
      } else if (escape_buffer[1] == 'B') {
        // down
        ycursor += 1;
      } else if (escape_buffer[1] == 'H') {
        // beg line
        xcursor = 0;
      } else if (escape_buffer[1] == 'F') {
        // end line
        xcursor = 120;
      } else if (escape_buffer[1] == '5' && escape_buffer[2] == '~') {
        // page up
        ycursor = 0;
      } else if (escape_buffer[1] == '6' && escape_buffer[2] == '~') {
        // page down
        ycursor = 56;
      }
      if (xcursor > 120) xcursor = 120;
      if (ycursor > 56) ycursor = 56;

      drawScene();
    }
}

void command(char c) {
  if (c == 'F') {
    ch1115.breathingEffect(CH1115_ON);
  } else if (c == 'f') {
    ch1115.breathingEffect(CH1115_OFF);
  } else if (c == 'p') {
    ch1115.drawPixel(0,0,CH1115_WHITE_COLOR);
  } else if (c == 'O') {
    ch1115.enable(CH1115_ON);
  } else if (c == 'o') {
    ch1115.enable(CH1115_OFF);
  } else if (c == 'I') {
    ch1115.invert(CH1115_ON);
  } else if (c == 'i') {
    ch1115.invert(CH1115_OFF);
  } else if (c == 'L') {
    ch1115.flip(CH1115_ON);
  } else if (c == 'l') {
    ch1115.flip(CH1115_OFF);
  } else if (c == 'S') {
    ch1115.scrollArea(0, 7, 0, 127, CH1115_SCROLL_LEFT, CH1115_SCROLL_2FRAMES);
    ch1115.scroll(CH1115_SCROLL_ONE_COLUMN);
  } else if (c == 's') {
    ch1115.scroll(CH1115_SCROLL_OFF);
  } else if (c == 'H') {
    ch1115.scrollArea(2, 5, 32, 96, CH1115_SCROLL_RIGHT, CH1115_SCROLL_2FRAMES);
    ch1115.scroll(CH1115_SCROLL_CONTINUOUS);
  } else if (c == 'h') {
    ch1115.scroll(CH1115_SCROLL_OFF);
  }
}

void loop()
{
  bool escape = false;
  memset(escape_buffer, 0, 10);
  escpos = 0;
  uint8_t readchar = 0;

  while (Serial.available()) {
    readchar++;
    char c = Serial.read();
    if (c == '\n' || c == '\r') {
      ch1115.drawString(0, curline*8, line_buffer);
      curline++;
      if (curline >= MAX_LINE) {
        curline = 0;
      }
      reset_line();
      ch1115.drawString(0, curline*8, line_buffer);
    } else if (c == 27) { // escape
      escape = true;
      delay(1); // delay to ensure esc sequence is available
    } else if (c == '\b' || c == 127) { // backspace or delete
      textpos--;
      if (textpos < 2) {
        textpos = 2;
      }
      line_buffer[textpos] = ' ';
      ch1115.drawString(0, curline*8, line_buffer);
    } else if (c > 31 && c < 127) { // only ASCII
      if (escape) {
        escape_buffer[escpos++] = c;
      } else {
        line_buffer[textpos] = c;
        textpos++;
        if (textpos == MAX_COLUMN) {
          ch1115.drawString(0, curline*8, line_buffer);
          curline++;
          if (curline >= MAX_LINE) {
            curline = 0;
          }
          memset(line_buffer, ' ', MAX_COLUMN + 1);
          line_buffer[MAX_COLUMN] = 0;
          textpos = 0;
        }
        ch1115.drawString(0, curline*8, line_buffer);
        command(c);
      }
    }
  }

  if (escape) {
    Serial.print("Current ESC buffer: ");
    Serial.println(escape_buffer);
    esccommand(escape_buffer);
  }
}
