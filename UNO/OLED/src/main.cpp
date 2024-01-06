#include <Arduino.h>

#include <CH1115Display.h>

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

uint8_t xcursor = 40;
uint8_t ycursor = 28;

void drawScene() {
  ch1115.drawScreen(0x00);
  ch1115.drawString(xcursor, ycursor, "Welcome");
  return;
  ch1115.drawLine(0,0,127,0, CH1115_WHITE_COLOR);
  ch1115.drawLine(127,0,127,63, CH1115_WHITE_COLOR);
  ch1115.drawLine(127,63,0,63, CH1115_WHITE_COLOR);
  ch1115.drawLine(0,63,0,0,CH1115_WHITE_COLOR);
}

void setup()
{
  reset_line();
  Serial.begin(115200);

  // init OLED display
  ch1115.init(0x01);
  ch1115.flip(CH1115_ON);

  drawScene();
}

void esccommand(const char* escape_buffer) {

    if (escape_buffer[0] == '[') {
      // arrow
      if (escape_buffer[1] == 'C') {
        // right
        xcursor += 8;
      } else if (escape_buffer[1] == 'D') {
        // left
        xcursor = (xcursor?xcursor-8:0);
      } else if (escape_buffer[1] == 'A') {
        // up
        ycursor = (ycursor?ycursor-8:0);
      } else if (escape_buffer[1] == 'B') {
        // down
        ycursor += 8;
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
