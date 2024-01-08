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

void reset_line()
{
  memset(line_buffer, ' ', MAX_COLUMN + 1);
  line_buffer[0] = '>';
  line_buffer[MAX_COLUMN] = 0;
  textpos = 2;
}

uint8_t xcursor = 60;
uint8_t ycursor = 0;
uint8_t xprev = 1;
uint8_t yprev = 1;

#define NB_ALIENS 8
#define NB_ALIEN_ROW 4
uint8_t ALIEN_X_MAX_POS = 16;
uint8_t ALIEN_Y_MAX_POS = 16;
uint8_t alien_x_pos = 0;
uint8_t alien_y_pos = 0;
uint8_t alien_dx = 1;

uint8_t gameOver = 1;

void drawGameOver()
{
  ch1115.drawScreen(0x00, true);
  ch1115.drawString(32, 32, "GAME OVER");
  ch1115.drawString(3, 48, "spacebar to restart.");
  ch1115.scrollArea(6, 6, 2, 120, CH1115_SCROLL_RIGHT, CH1115_SCROLL_6FRAMES);
  ch1115.scroll(CH1115_SCROLL_CONTINUOUS);
  gameOver = 1;
}

void drawStart()
{
  ch1115.drawScreen(0x00, true);
  ch1115.drawString(40, 24, "INVADERS");
  ch1115.drawString(3, 48, "spacebar to start");
  ch1115.scrollArea(6, 6, 2, 120, CH1115_SCROLL_RIGHT, CH1115_SCROLL_6FRAMES);
  ch1115.scroll(CH1115_SCROLL_CONTINUOUS);
}

void drawAlien() {
  // draw aliens
  // range to clear
  for (uint8_t p = 0; p < NB_ALIEN_ROW; p++)
  {
    for (uint8_t i = 0; i < NB_ALIENS; i++)
    {
      ch1115.drawSprite(alien_x_pos + i * 15, p * 8 + alien_y_pos, SPRITE_WIDTH, SPRITE_HEIGHT, (p < 2) ? alien2 : alien, 3);
    }
  }
  
  // move alien
  if (alien_x_pos >= ALIEN_X_MAX_POS)
  {
    alien_dx = -1;
    alien_y_pos += 1;
  }
  else if (alien_x_pos == 0)
  {
    alien_dx = 1;
  }
  alien_x_pos += alien_dx;
}

#define ALIEN_FRAME_COUNTER 3
uint8_t alien_frame = 3;

void drawScene(uint8_t first)
{
  unsigned long start = millis();

  if (first)
  {
    ch1115.drawScreen(0x00);
  }

  if (first || alien_frame == 0) {
    drawAlien();
    alien_frame = ALIEN_FRAME_COUNTER;
  }
  alien_frame--;

  // check alien pos
  if (alien_y_pos >= ALIEN_Y_MAX_POS)
  {
    drawGameOver();
    return;
  }

  // draw shelter
  if (first)
  {
    ch1115.drawSprite(16, 48, SHELTER_WIDTH, SPRITE_HEIGHT, shelter, 0);
    ch1115.drawSprite(16 + 14 + 27, 48, SHELTER_WIDTH, SPRITE_HEIGHT, shelter, 0);
    ch1115.drawSprite(16 + 14 + 27 + 14 + 27, 48, SHELTER_WIDTH, SPRITE_HEIGHT, shelter, 0);
  }

  // draw spaceship
  if (first || xcursor != xprev)
  {
    ch1115.drawSprite(xcursor, 56, SPRITE_WIDTH, SPRITE_HEIGHT, spaceship, 0);
    xprev = xcursor;
  }

  unsigned long end = millis();
  if ((end - start) > 150) {
    Serial.print(F("Frame refresh in (ms): "));
    Serial.println(end - start);
  }
}

void setup()
{
  reset_line();
  Serial.begin(115200);

  // init OLED display
  ch1115.init(0x01);
  ch1115.flip(CH1115_ON);
  ch1115.drawScreen(0x00);

  drawStart();
}

void esccommand(const char *escape_buffer)
{

  if (escape_buffer[0] == '[')
  {
    // arrow
    if (escape_buffer[1] == 'C')
    {
      // right
      xcursor += 1;
    }
    else if (escape_buffer[1] == 'D')
    {
      // left
      xcursor = (xcursor ? xcursor - 1 : 0);
    }
    else if (escape_buffer[1] == 'A')
    {
      // up
      alien_y_pos = (alien_y_pos ? alien_y_pos - 1 : 0);
    }
    else if (escape_buffer[1] == 'B')
    {
      // down
      alien_y_pos += 1;
    }
    else if (escape_buffer[1] == 'H')
    {
      // beg line
      xcursor = 0;
    }
    else if (escape_buffer[1] == 'F')
    {
      // end line
      xcursor = 120;
    }
    else if (escape_buffer[1] == '5' && escape_buffer[2] == '~')
    {
      // page up
      alien_y_pos = 0;
    }
    else if (escape_buffer[1] == '6' && escape_buffer[2] == '~')
    {
      // page down
      alien_y_pos = 56;
    }
    if (xcursor > 120)
      xcursor = 120;
    if (alien_y_pos > 56)
      alien_y_pos = 56;
  }
}

void command(char c)
{
  if (c == 'F')
  {
    ch1115.breathingEffect(CH1115_ON);
  }
  else if (c == 'f')
  {
    ch1115.breathingEffect(CH1115_OFF);
  }
  else if (c == 'p')
  {
    ch1115.drawPixel(0, 0, CH1115_WHITE_COLOR);
  }
  else if (c == 'O')
  {
    ch1115.enable(CH1115_ON);
  }
  else if (c == 'o')
  {
    ch1115.enable(CH1115_OFF);
  }
  else if (c == 'I')
  {
    ch1115.invert(CH1115_ON);
  }
  else if (c == 'i')
  {
    ch1115.invert(CH1115_OFF);
  }
  else if (c == 'L')
  {
    ch1115.flip(CH1115_ON);
  }
  else if (c == 'l')
  {
    ch1115.flip(CH1115_OFF);
  }
  else if (c == 'S')
  {
    ch1115.scrollArea(0, 7, 0, 127, CH1115_SCROLL_LEFT, CH1115_SCROLL_2FRAMES);
    ch1115.scroll(CH1115_SCROLL_ONE_COLUMN);
  }
  else if (c == 's')
  {
    ch1115.scroll(CH1115_SCROLL_OFF);
  }
  else if (c == 'H')
  {
    ch1115.scrollArea(2, 5, 32, 96, CH1115_SCROLL_RIGHT, CH1115_SCROLL_2FRAMES);
    ch1115.scroll(CH1115_SCROLL_CONTINUOUS);
  }
  else if (c == 'h')
  {
    ch1115.scroll(CH1115_SCROLL_OFF);
  }
}

void commandloop()
{
  bool escape = false;
  memset(escape_buffer, 0, 10);
  escpos = 0;
  uint8_t readchar = 0;

  while (Serial.available())
  {
    readchar++;
    char c = Serial.read();
    if (c == '\n' || c == '\r')
    {
      ch1115.drawString(0, curline * 8, line_buffer);
      curline++;
      if (curline >= MAX_LINE)
      {
        curline = 0;
      }
      reset_line();
      ch1115.drawString(0, curline * 8, line_buffer);
    }
    else if (c == 27)
    { // escape
      escape = true;
      delay(1); // delay to ensure esc sequence is available
    }
    else if (c == '\b' || c == 127)
    { // backspace or delete
      textpos--;
      if (textpos < 2)
      {
        textpos = 2;
      }
      line_buffer[textpos] = ' ';
      ch1115.drawString(0, curline * 8, line_buffer);
    }
    else if (c > 31 && c < 127)
    { // only ASCII
      if (escape)
      {
        escape_buffer[escpos++] = c;
      }
      else
      {
        line_buffer[textpos] = c;
        textpos++;
        if (textpos == MAX_COLUMN)
        {
          ch1115.drawString(0, curline * 8, line_buffer);
          curline++;
          if (curline >= MAX_LINE)
          {
            curline = 0;
          }
          memset(line_buffer, ' ', MAX_COLUMN + 1);
          line_buffer[MAX_COLUMN] = 0;
          textpos = 0;
        }
        ch1115.drawString(0, curline * 8, line_buffer);
        command(c);
      }
    }
  }

  if (escape)
  {
    // Serial.print("Current ESC buffer: ");
    // Serial.println(escape_buffer);
    esccommand(escape_buffer);
  }
}

void gameloop()
{
  bool escape = false;
  memset(escape_buffer, 0, 10);
  escpos = 0;
  uint8_t readchar = 0;

  while (Serial.available())
  {
    readchar++;
    char c = Serial.read();
    if (c == 27)
    { // escape
      escape = true;
      yield();
      //delayMicroseconds(700); // delay to ensure esc sequence is available
    }
    else if (c > 31 && c < 127)
    { // only ASCII
      if (escape)
      {
        escape_buffer[escpos++] = c;
      }
      else if (c == ' ')
      {
        if (gameOver)
        {
          Serial.println("Restart");
          gameOver = 0;
          alien_x_pos = 0;
          alien_y_pos = 0;
          alien_dx = 1;
          ch1115.scroll(CH1115_SCROLL_OFF);
          drawScene(1);
        }
        else
        {
          Serial.println("Shoot");
        }
      }
    }
  }

  if (escape)
  {
    // Serial.print("Current ESC buffer: ");
    // Serial.println(escape_buffer);
    esccommand(escape_buffer);
  }

  if (!gameOver)
  {
    drawScene(0);
  }
}

void loop()
{
  gameloop();
}