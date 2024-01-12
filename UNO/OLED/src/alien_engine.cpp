#include "common.h"

#include "sprites.h"
#include <CH1115Display.h>

#ifdef USE_MINICORE
#include <arduimini.h>
#else
#include <Arduino.h>
#endif

#define HAS_SERIAL

const uint8_t NB_ALIENS = 8;
const uint8_t NB_ALIEN_ROW = 4;

const uint8_t ALIEN_X_MAX_POS = 16;
const uint8_t ALIEN_Y_MAX_POS = 16;

const uint8_t ALIEN_X_SPACING = 15;
const uint8_t ALIEN_Y_SPACING = 8;

uint8_t aliens[NB_ALIENS * NB_ALIEN_ROW];

uint8_t alien_x_pos = 0;
uint8_t alien_y_pos = 0;
uint8_t alien_dx = 1;

uint8_t alien_frame = 0;

uint8_t dont_go_down = 5;

void update_alien(CH1115Display *display) {
  // draw aliens
  for (uint8_t p = 0; p < NB_ALIEN_ROW; p++) {
    for (uint8_t i = 0; i < NB_ALIENS; i++) {
      const uint8_t *sprite = empty;
      if (aliens[i + p * NB_ALIENS] == 1) {
        sprite = alien;
      } else if (aliens[i + p * NB_ALIENS] == 2) {
        sprite = alien2;
      } else if (aliens[i + p * NB_ALIENS] == 3) {
        sprite = explosion_frames;
      } else if (aliens[i + p * NB_ALIENS] == 4) {
        sprite = explosion_frames + SPRITE_WIDTH;
      } else if (aliens[i + p * NB_ALIENS] == 5) {
        sprite = explosion_frames + 2 * SPRITE_WIDTH;
      } else if (aliens[i + p * NB_ALIENS] == 6) {
        sprite = explosion_frames + 3 * SPRITE_WIDTH;
      }

      display->drawSprite(alien_x_pos + i * ALIEN_X_SPACING,
                          p * ALIEN_Y_SPACING + alien_y_pos, SPRITE_WIDTH,
                          SPRITE_HEIGHT, sprite, OVERWRITE_MODE);
    }
  }

  // move alien
  if (alien_x_pos >= ALIEN_X_MAX_POS) {
    alien_dx = -1;
    dont_go_down--;
    if (dont_go_down == 0) {
      dont_go_down = 5;
      alien_y_pos += 1;
    }
  } else if (alien_x_pos == 0) {
    alien_dx = 1;
  }
  alien_x_pos += alien_dx;

  // check explosion
  for (uint8_t i = 0; i < NB_ALIENS * NB_ALIEN_ROW; ++i) {
    if (aliens[i] >= 3) {
      aliens[i] += 1;
    }
    if (aliens[i] == 6) {
      aliens[i] = 0;
    }
  }
}

void move_alien(uint8_t direction) {
  if (direction == MOVE_INIT) {
    alien_x_pos = 0;
    alien_y_pos = 0;
    alien_dx = 1;

    for (uint8_t row = 0; row < NB_ALIEN_ROW; ++row) {
      for (uint8_t col = 0; col < NB_ALIENS; ++col) {
        aliens[col + row * NB_ALIENS] = (row < 2) ? 2 : 1;
      }
    }
  } else if (direction == MOVE_UP) {
    alien_y_pos = (alien_y_pos ? alien_y_pos - 1 : 0);
  } else if (direction == MOVE_DOWN) {
    alien_y_pos += 1;
  }

  if (alien_y_pos > 56) {
    alien_y_pos = 56;
  }
}

bool kill_alien(uint8_t x, uint8_t y) {
  uint8_t col = (x - alien_x_pos) / ALIEN_X_SPACING;
  uint8_t row = (y - alien_y_pos) / ALIEN_Y_SPACING;

  if (col < NB_ALIENS && row < NB_ALIEN_ROW) {
    // check if it is an explosion and skip it
    if (aliens[col + row * NB_ALIENS] >= 3) {
#ifdef HAS_SERIAL
      Serial.print("Hit alien explosion: ");
      Serial.print(col);
      Serial.print(" , ");
      Serial.println(row);
#endif
      return false;
    }
    aliens[col + row * NB_ALIENS] = 3; // start explosion frame
#ifdef HAS_SERIAL
    Serial.print("Hit alien: ");
    Serial.print(col);
    Serial.print(" , ");
    Serial.println(row);
#endif
    return true;
  }

  return false;
}

uint8_t check_alien_status() {
  // count remaining aliens
  uint8_t nb = 0;
  for (uint8_t i = 0; i < NB_ALIENS * NB_ALIEN_ROW; ++i) {
    nb += aliens[i];
  }

  if (nb == 0) {
    return ALIEN_LOST;
  }

  uint8_t nbdeadrow = 0;
  for (uint8_t r = NB_ALIEN_ROW - 1; r >= 0; --r) {
    nb = 0;
    for (uint8_t c = 0; c < NB_ALIENS; ++c) {
      nb += aliens[c + r * NB_ALIENS];
    }
    if (nb == 0) {
      nbdeadrow++;
    } else {
      break;
    }
  }

  if (alien_y_pos >= (ALIEN_Y_MAX_POS + 8 * nbdeadrow)) {
    return ALIEN_WIN;
  }

  return 0;
}