#include "common.h"

#include <CH1115Display.h>

#include "sprites.h"

#ifdef USE_MINICORE
#include <arduimini.h>
#else
#include <Arduino.h>
#endif

// #define HAS_SERIAL

uint8_t missile_state = 0;

uint8_t x_missile = 0;
uint8_t y_missile = 0;

uint8_t x_position = 64;
uint8_t x_prev_position = 64;

void move_spaceship(uint8_t direction) {
  x_prev_position = x_position;

  if (direction == MOVE_INIT) {
    x_position = 64;
    x_prev_position = 64;
  } else if (direction == MOVE_LEFT && x_position > 0) {
    x_position -= 1;

  } else if (direction == MOVE_RIGHT && x_position < (SCREEN_WIDTH - 9)) {
    x_position += 1;
  }
}

void spaceship_action(uint8_t action) {
  if (action == GUNFIRE_ACTION && missile_state == 0) {
    x_missile = x_position + SPRITE_WIDTH / 2;
    y_missile = 0;
    missile_state = 1;
#ifdef HAS_SERIAL
    Serial.println("Shoot");
#endif
  }
}

void hit_something(uint8_t x, uint8_t ymin, uint8_t ymax, CH1115Display *display) {
#ifdef HAS_SERIAL
  Serial.print("Hit something missile: ");
  Serial.print(x);
  Serial.print(" , ");
  Serial.println(y);
#endif

  if ((ymin <= 55 && ymin >= 48) || (ymax <= 55 && ymax >= 48)) {
#ifdef HAS_SERIAL
    Serial.println("Hit shelter");
#endif
    missile_state = 0;
    y_missile = 0;
    display->startPageDrawing(x_missile - 1, 48);
    display->updatePageColumn(0x00, OVERWRITE_MODE);
    display->updatePageColumn(0x00, OVERWRITE_MODE);
    display->updatePageColumn(0x00, OVERWRITE_MODE);
    display->endPageDrawing();
  } else {

#ifdef HAS_SERIAL
    Serial.println("Hit alien");
#endif
    bool k = kill_alien(x, ymin) || kill_alien(x, ymax);
    if (k) {
      clear_missile(x_missile, y_missile, display);
      missile_state = 0;
      y_missile = 0;
    }
  }
}

uint8_t draw_missile(uint8_t x, uint8_t y, CH1115Display *display) {
    uint8_t missile_pattern = 0xF0;
    uint8_t shift = y_missile % 8;
    if (shift == 7) {
      missile_pattern = 0xF0;
    } else if (shift == 3) {
      missile_pattern = 0x0F;
    }

#ifdef HAS_SERIAL
    Serial.print("Draw missile: ");
    Serial.print(x_missile);
    Serial.print(" , ");
    Serial.print(y_missile);
    Serial.print(" , ");
    Serial.println(missile_pattern, 16);
#endif
    display->startPageDrawing(x_missile, y_missile);
    uint8_t prev =
        display->updatePageColumn(missile_pattern, XOR_MODE, missile_pattern);
    display->endPageDrawing();

    return prev;
}

void clear_missile(uint8_t x, uint8_t y, CH1115Display *display) {
  // erase previous missile
  uint8_t missile_pattern = 0xF0;
  uint8_t shift = y_missile % 8;
  if (shift == 7) {
    missile_pattern = 0xF0;
  } else if (shift == 3) {
    missile_pattern = 0x0F;
  }

  display->startPageDrawing(x, y);
  uint8_t prev = display->updatePageColumn(missile_pattern, XOR_MODE, missile_pattern);
  display->endPageDrawing();
}

void update_missile(CH1115Display *display) {
  if (y_missile != 0) {
    // erase previous missile
    clear_missile(x_missile, y_missile, display);

    // move missile
    y_missile -= 4;
  } else {
    y_missile = 55;
  }

  // draw new missile
  if (y_missile >= 7) {
    uint8_t prev = draw_missile(x_missile, y_missile, display);

    if (prev != 0) {
      hit_something(x_missile, y_missile-3, y_missile, display);
    }
    
  } else {
    missile_state = 0;
    y_missile = 0;
#ifdef HAS_SERIAL
    Serial.println("Missile left without hitting anything.");
#endif
  }
}

void update_spaceship(CH1115Display *display) {
  display->drawSprite(x_position, 56, SPRITE_WIDTH, SPRITE_HEIGHT, spaceship,
                      OVERWRITE_MODE);
  if (missile_state) {
    update_missile(display);
  }
}

void draw_shelter(CH1115Display *display) {
  display->drawSprite(16, 48, SHELTER_WIDTH, SPRITE_HEIGHT, shelter,
                      OVERWRITE_MODE);
  display->drawSprite(16 + 14 + 27, 48, SHELTER_WIDTH, SPRITE_HEIGHT, shelter,
                      OVERWRITE_MODE);
  display->drawSprite(16 + 14 + 27 + 14 + 27, 48, SHELTER_WIDTH, SPRITE_HEIGHT,
                      shelter, OVERWRITE_MODE);
}

uint8_t check_spaceship_status() { return 0; }