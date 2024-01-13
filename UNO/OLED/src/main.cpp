

#include "common.h"

#ifdef USE_MINICORE
#include <arduimini.h>
#include <string.h>
#else
#include <Arduino.h>
#endif
#include <CH1115Display.h>

CH1115Display ch1115(SCREEN_WIDTH, SCREEN_HEIGHT);

uint8_t gameOver = 1;

void drawScene(CH1115Display *display, bool first) {
  unsigned long start = millis();

  if (first) {
    // reset screen effect
    display->scroll(CH1115_SCROLL_OFF);
    // init screen
    move_alien(MOVE_INIT);
    move_spaceship(MOVE_INIT);
    display->drawScreen(0x00);
    draw_shelter(&ch1115);
  }

  if (first || alien_frame == 0) {
    update_alien(&ch1115);
    alien_frame = ALIEN_FRAME_COUNTER;
  }
  alien_frame--;

  // draw spaceship
  update_spaceship(&ch1115);

  unsigned long end = millis();
  if ((end - start) > 160) {
    Serial.print("Frame refresh in (ms): ");
    Serial.println(end - start);
  }
}

bool esccommand(const char *escape_buffer) {

  if (escape_buffer[0] == '[') {
    // arrow
    if (escape_buffer[1] == 'C') {
      // right
      move_spaceship(MOVE_RIGHT); 
      return true;
    } else if (escape_buffer[1] == 'D') {
      // left
      move_spaceship(MOVE_LEFT);
      return true;
    } else if (escape_buffer[1] == 'A') {
      // up
      move_alien(MOVE_UP);
      return true;
    } else if (escape_buffer[1] == 'B') {
      // down
      move_alien(MOVE_DOWN);
      return true;
    } else if (escape_buffer[1] == 'H') {
      // beg line
      return true;
    } else if (escape_buffer[1] == 'F') {
      // end line
      return true;
    } else if (escape_buffer[1] == '5' && escape_buffer[2] == '~') {
      // page up
      return true;
    } else if (escape_buffer[1] == '6' && escape_buffer[2] == '~') {
      // page down
      return true;
    }
  }

  return false;
}

char escape_buffer[10];

void gameloop() {
  bool escape = false;
  memset(escape_buffer, 0, 10);
  uint8_t escpos = 0;

  delay(1);
  while (Serial.available()) {
    char c = Serial.read();
    if (c == 27) { // escape
      escape = true;
      delayMicroseconds(700);       // delay to ensure esc sequence is available
    } else if (c > 31 && c < 127) { // only ASCII
      if (escape) {
        escape_buffer[escpos++] = c;
        if (esccommand(escape_buffer)) {
          escape = false;
          memset(escape_buffer, 0, 10);
          escpos = 0;
          break;
        }
      } else if (c == ' ') {
        if (gameOver) {
          gameOver = 0;
          drawScene(&ch1115, true);
        } else {
          spaceship_action(GUNFIRE_ACTION);
        }
        break;
      } else if (c == 'l') {
        move_spaceship(MOVE_RIGHT);
        break;
      } else if (c == 'h') {
        move_spaceship(MOVE_LEFT);
        break;
      }

    }
  }

  if (!gameOver) {
    drawScene(&ch1115, false);

    // check win condition
    if (check_alien_status() == ALIEN_WIN) {
      gameOver = 1;
      drawGameOver(&ch1115);
    } else if (check_alien_status() == ALIEN_LOST) {
      gameOver = 1;
      drawVictory(&ch1115);
      drawStart(&ch1115);
    }
  }
}

/*
  Init and main loop
*/
void setup() {
  Serial.begin(115200);

  // init OLED display
  ch1115.init(0x01);
  ch1115.flip(CH1115_ON);
  ch1115.drawScreen(0x00);

  // draw welcome screen
  drawStart(&ch1115);
}

void loop() { gameloop(); }

#ifdef USE_MINICORE
#include <main.cpp.h>
#endif
