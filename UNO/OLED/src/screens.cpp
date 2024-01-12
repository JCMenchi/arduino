#include "common.h"

#ifdef USE_MINICORE
#include <arduimini.h>
#else
#include <Arduino.h>
#endif

#include <CH1115Display.h>

void drawGameOver(CH1115Display *display) {
  display->drawScreen(0x00, true);
  display->drawString(32, 32, "GAME OVER");
  display->drawString(3, 54, "spacebar to restart.");
  display->scrollArea(6, 7, 2, 120, CH1115_SCROLL_RIGHT, CH1115_SCROLL_6FRAMES);
  display->scroll(CH1115_SCROLL_CONTINUOUS);
}

void drawStart(CH1115Display *display) {
  display->breathingEffect(CH1115_OFF);
  display->drawScreen(0x00, true);
  display->drawString(40, 24, "INVADERS");
  display->drawString(3, 54, "insert coins");
  display->scrollArea(6, 7, 2, 120, CH1115_SCROLL_RIGHT, CH1115_SCROLL_6FRAMES);
  display->scroll(CH1115_SCROLL_CONTINUOUS);
}

void drawVictory(CH1115Display *display) {
  display->drawScreen(0x00, true);
  display->drawString(40, 24, "YOU WIN");
  display->contrast(0xFF);
  display->breathingEffect(CH1115_ON);
  delay(5000);
  display->breathingEffect(CH1115_OFF);
  display->contrast(0x01);
  drawStart(display);
}