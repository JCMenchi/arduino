#include "common.h"

#include <util/delay.h>

#include <sound.h>
#include <millisec.h>
#include <usart_serial.h>
#include <CH1115Display.h>

#include <highscore.h>
#include <bitmap_font.h>

// Draws the "Game Over" screen with scrolling text effect.
void drawGameOver(CH1115Display *display) {
  display->drawScreen(0x00, true);
  display->drawString(40, 24, "GAME OVER");
  display->drawString(3, 54, "insert coins...");
  display->scrollArea(6, 7, 2, 120, CH1115_SCROLL_RIGHT, CH1115_SCROLL_6FRAMES);
  display->scroll(CH1115_SCROLL_CONTINUOUS);
}

// Draws the start screen with the game title and scrolling "insert coins..." message.
void drawStart(CH1115Display *display) {
  display->breathingEffect(CH1115_OFF);
  display->drawScreen(0x00, true);
  display->drawString(40, 24, "INVADERS");
  display->drawString(3, 54, "insert coins...");
  display->scrollArea(6, 7, 2, 120, CH1115_SCROLL_RIGHT, CH1115_SCROLL_6FRAMES);
  display->scroll(CH1115_SCROLL_CONTINUOUS);
}

// Draws the victory screen, applies a breathing effect, waits, then returns to the start screen.
void drawVictory(CH1115Display *display) {
  display->drawScreen(0x00, true);
  display->drawString(40, 24, "YOU WIN");
  display->contrast(0xFF);
  display->breathingEffect(CH1115_ON);
  _delay_ms(5000);
  display->breathingEffect(CH1115_OFF);
  display->contrast(0x01);
  drawStart(display);
}

// Draws the "High Score" screen
void drawHighScore(CH1115Display *display, UserScore *highscore, uint8_t size) {
  const uint8_t ybase = 4;
  display->drawScreen(0x00, true);
  display->drawString((SCREEN_WIDTH - 11 * FONT_CHAR_WIDTH)/2, ybase, "High Scores");

  display->drawString(20, ybase + FONT_CHAR_HEIGHT + 4, "1st:");
  display->drawString(20 + 6 * FONT_CHAR_WIDTH, ybase + FONT_CHAR_HEIGHT + 4, highscore[0].user);
  display->drawInt(20 + 12 * FONT_CHAR_WIDTH, ybase + FONT_CHAR_HEIGHT + 4, highscore[0].score);
  
  display->drawString(20, ybase + 2 * (FONT_CHAR_HEIGHT + 4), "2nd:");
  display->drawString(20 + 6 * FONT_CHAR_WIDTH, ybase + 2 * (FONT_CHAR_HEIGHT + 4), highscore[1].user);
  display->drawInt(20 + 12 * FONT_CHAR_WIDTH, ybase + 2 * (FONT_CHAR_HEIGHT + 4), highscore[1].score);

  display->drawString(20, ybase + 3 * (FONT_CHAR_HEIGHT + 4), "3rd:");
  display->drawString(20 + 6 * FONT_CHAR_WIDTH, ybase + 3 * (FONT_CHAR_HEIGHT + 4), highscore[2].user);
  display->drawInt(20 + 12 * FONT_CHAR_WIDTH, ybase + 3 * (FONT_CHAR_HEIGHT + 4), highscore[2].score);

  display->drawString(3, 54, "insert coins...");
  display->scrollArea(6, 7, 2, 120, CH1115_SCROLL_RIGHT, CH1115_SCROLL_6FRAMES);
  display->scroll(CH1115_SCROLL_CONTINUOUS);
}

#define SHOW_PERFORMANCE 1

#ifdef SHOW_PERFORMANCE
unsigned long fps_start_time = 0;
uint32_t fps_nb_frame = 0;
#endif

// Draws the main game scene, updates game objects, and optionally prints performance info.
void drawScene(CH1115Display *display, bool first) {
  unsigned long start = milliseconds();

  if (first) {
    start_sound();
    #ifdef SHOW_PERFORMANCE
    // init FPS counters
    fps_start_time = 0;
    fps_nb_frame = 0;
    #endif
    // reset screen effect
    display->scroll(CH1115_SCROLL_OFF);
    // init screen
    move_alien(MOVE_INIT);
    move_spaceship(MOVE_INIT);

    display->drawScreen(0x00);
    draw_shelter(display);
  }

  // update spaceship
  update_spaceship(display);
  // update alien
  update_alien(display);

  #ifdef SHOW_PERFORMANCE
  // compute FPS end, check drawing time
  unsigned long end = milliseconds();

  fps_nb_frame++;
  if (fps_start_time == 0) {
    fps_start_time = start;
  } else if ((end - fps_start_time) > 10000) {
    USART_WriteString("FPS: ");
    USART_WriteInt(((fps_nb_frame * 1000) / (end - fps_start_time)));
    USART_WriteString("\n");

    fps_start_time = 0;
    fps_nb_frame = 0;
  }

  if ((end - start) > 190) {
    USART_WriteString("Frame refresh in (ms): ");
    USART_WriteInt(end - start);
    USART_WriteString("\n");
  }
  #endif
}

void drawHighScoreUpdate(CH1115Display *display, UserScore *highscore, uint8_t size, uint8_t pos) {
  const uint8_t ybase = 4;
  display->drawScreen(0x00, true);
  display->drawString((SCREEN_WIDTH - 11 * FONT_CHAR_WIDTH)/2, ybase, "High Scores");

  display->drawString(20, ybase + FONT_CHAR_HEIGHT + 4, "1st:");
  display->drawString(20 + 6 * FONT_CHAR_WIDTH, ybase + FONT_CHAR_HEIGHT + 4, highscore[0].user);
  display->drawInt(20 + 12 * FONT_CHAR_WIDTH, ybase + FONT_CHAR_HEIGHT + 4, highscore[0].score);
  
  display->drawString(20, ybase + 2 * (FONT_CHAR_HEIGHT + 4), "2nd:");
  display->drawString(20 + 6 * FONT_CHAR_WIDTH, ybase + 2 * (FONT_CHAR_HEIGHT + 4), highscore[1].user);
  display->drawInt(20 + 12 * FONT_CHAR_WIDTH, ybase + 2 * (FONT_CHAR_HEIGHT + 4), highscore[1].score);

  display->drawString(20, ybase + 3 * (FONT_CHAR_HEIGHT + 4), "3rd:");
  display->drawString(20 + 6 * FONT_CHAR_WIDTH, ybase + 3 * (FONT_CHAR_HEIGHT + 4), highscore[2].user);
  display->drawInt(20 + 12 * FONT_CHAR_WIDTH, ybase + 3 * (FONT_CHAR_HEIGHT + 4), highscore[2].score);

  display->drawString(3, 54, "Enter your name");
  
}