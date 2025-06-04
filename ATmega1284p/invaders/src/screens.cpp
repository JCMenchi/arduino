#include "common.h"

#include <usart_serial.h>
#include <util/delay.h>
#include <millisec.h>
#include <sound.h>

#include <CH1115Display.h>

unsigned long fps_start_time = 0;
uint32_t fps_nb_frame = 0;

void drawGameOver(CH1115Display *display) {
  display->drawScreen(0x00, true);
  display->drawString(40, 24, "GAME OVER");
  display->drawString(3, 54, "insert coins...");
  display->scrollArea(6, 7, 2, 120, CH1115_SCROLL_RIGHT, CH1115_SCROLL_6FRAMES);
  display->scroll(CH1115_SCROLL_CONTINUOUS);
}

void drawStart(CH1115Display *display) {
  display->breathingEffect(CH1115_OFF);
  display->drawScreen(0x00, true);
  display->drawString(40, 24, "INVADERS");
  display->drawString(3, 54, "insert coins...");
  display->scrollArea(6, 7, 2, 120, CH1115_SCROLL_RIGHT, CH1115_SCROLL_6FRAMES);
  display->scroll(CH1115_SCROLL_CONTINUOUS);
}

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

void drawScene(CH1115Display *display, bool first) {
  unsigned long start = milliseconds();

  if (first) {
    start_sound();
    fps_start_time = 0;
    fps_nb_frame = 0;
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

  if ((end - start) > 150) {
    USART_WriteString("Frame refresh in (ms): ");
    USART_WriteInt(end - start);
    USART_WriteString("\n");
  }
}
