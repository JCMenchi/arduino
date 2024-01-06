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

void setup()
{
  reset_line();
  Serial.begin(115200);
  Serial.println(F("screen init"));

  // init OLED display
  ch1115.init(0x01);
  ch1115.enable(CH1115_ON);
  ch1115.flip(CH1115_ON);
  ch1115.drawScreen(0x00);

  // draw some text
  ch1115.drawString(0, 0, (char*)"the quick brown fox, ");
  ch1115.drawString(0, 8, (char*)"jumps over the lazy dog");
  ch1115.drawString(0, 16, (char*)"jumps over the lazy dog");
  ch1115.drawString(0, 24, (char*)"jumps over the lazy dog");
  ch1115.drawString(0, 32, (char*)"A B C D E F G H I J K L ");
  ch1115.drawString(0, 40, (char*)"M N O P Q R S T U V W X ");
  ch1115.drawString(0, 48, (char*)"Y Z 0 1 2 3 4 5 6 7 8 9 ");
  ch1115.drawString(0, 56, (char*)"Y Z 0 1 2 3 4 5 6 7 8 9 ");

  
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
    Serial.print("Get char: ");
    Serial.println(c, 10);
    if (c == '\n' || c == '\r') {
      ch1115.drawString(0, curline*8, line_buffer);
      curline++;
      if (curline >= MAX_LINE) {
        curline = 0;
      }
      reset_line();
      Serial.print("Curline: ");
      Serial.println(curline);
    } else if (c == 27) { // escape
      escape = true;
      Serial.println("Start escape");
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
      }
      command(c);
    }
    
  }

  if (escape) {
    Serial.print("Current ESC buffer: ");
    Serial.println(escape_buffer);
  }
  if (readchar) {
    Serial.print(readchar);
    Serial.print(" Current line buffer: ");
    Serial.println(line_buffer);
  }
  ch1115.drawString(0, curline*8, line_buffer);
}
