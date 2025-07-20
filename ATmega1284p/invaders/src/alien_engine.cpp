#include "common.h"

#include "sprites.h"
#include <CH1115Display.h>

#include <stdlib.h>

#include <highscore.h>


const uint8_t NB_ALIENS_COL = 8;
const uint8_t NB_ALIEN_ROW = 4;

const uint8_t ALIEN_X_MAX_POS = 16;
const uint8_t ALIEN_Y_MAX_POS = 16;

const uint8_t ALIEN_X_SPACING = 15;
const uint8_t ALIEN_Y_SPACING = 8;

uint8_t nb_spaceship = MAX_LIFE;

uint8_t aliens[NB_ALIENS_COL * NB_ALIEN_ROW];
uint8_t min_col = 0;
uint8_t max_col = NB_ALIENS_COL;
uint8_t min_row = 0;
uint8_t max_row = NB_ALIEN_ROW;

uint8_t alien_x_pos = 0;
uint8_t alien_y_pos = 0;
uint8_t alien_dx = 1;

const uint8_t ALIEN_FRAME_COUNTER = 3;
uint8_t alien_frame = 0;

uint8_t dont_go_down = 5;

int8_t alien_missile_state = -1;

uint8_t alien_x_missile = 0;
uint8_t alien_y_missile = 0;

void update_alien_range() {

  if (alien_missile_state == -1) {
    alien_missile_state = rand() % NB_ALIENS_COL;
  }

  min_col = 0;
  max_col = NB_ALIENS_COL;
  min_row = 0;
  max_row = NB_ALIEN_ROW;

  uint8_t nb = 0;

  // MAX ROW
  for (uint8_t r = NB_ALIEN_ROW - 1; r >= 0; --r) {
    nb = 0;
    for (uint8_t c = 0; c < NB_ALIENS_COL; ++c) {
      if (aliens[c + r * NB_ALIENS_COL])
        nb++;
    }
    if (nb == 0) {
      max_row--;
    } else {
      break;
    }
  }

  // MIN ROW
  for (uint8_t r = 0; r < NB_ALIEN_ROW; ++r) {
    nb = 0;
    for (uint8_t c = 0; c < NB_ALIENS_COL; ++c) {
      if (aliens[c + r * NB_ALIENS_COL])
        nb++;
    }
    if (nb == 0) {
      min_row++;
    } else {
      break;
    }
  }

  for (uint8_t c = NB_ALIENS_COL - 1; c >= 0; --c) {
    nb = 0;
    for (uint8_t r = 0; r < NB_ALIEN_ROW; ++r) {
      if (aliens[c + r * NB_ALIENS_COL])
        nb++;
    }
    if (nb == 0) {
      max_col--;
    } else {
      break;
    }
  }

  for (uint8_t c = 0; c < NB_ALIENS_COL; ++c) {
    nb = 0;
    for (uint8_t r = 0; r < NB_ALIEN_ROW; ++r) {
      if (aliens[c + r * NB_ALIENS_COL])
        nb++;
    }
    if (nb == 0) {
      min_col++;
    } else {
      break;
    }
  }

}

void alien_hit_something(uint8_t x, uint8_t ymin, uint8_t ymax, CH1115Display *display) {

  if ((ymin <= 55 && ymin >= 48) || (ymax <= 55 && ymax >= 48)) {
    // Hit shelter
    alien_missile_state = -1;
    display->startPageDrawing(x - 1, 48);
    display->updatePageColumn(0x00, OVERWRITE_MODE);
    display->updatePageColumn(0x00, OVERWRITE_MODE);
    display->updatePageColumn(0x00, OVERWRITE_MODE);
    display->endPageDrawing();
  }

}

void clear_alien_missile(uint8_t x, uint8_t y, CH1115Display *display) {
  // erase previous missile
  uint8_t missile_pattern = 0xF0;
  uint8_t shift = alien_y_missile % 8;
  if (shift == 7) {
    missile_pattern = 0xF0;
  } else if (shift == 3) {
    missile_pattern = 0x0F;
  }

  display->startPageDrawing(x, y);
  display->updatePageColumn(0x00, OVERWRITE_MODE, missile_pattern);
  display->endPageDrawing();
}

uint8_t draw_alien_missile(uint8_t x, uint8_t y, CH1115Display *display) {
    uint8_t missile_pattern = 0xF0;
    uint8_t shift = alien_y_missile % 8;
    if (shift == 7) {
      missile_pattern = 0xF0;
    } else if (shift == 3) {
      missile_pattern = 0x0F;
    }

    display->startPageDrawing(alien_x_missile, alien_y_missile);
    uint8_t prev =
        display->updatePageColumn(missile_pattern, OVERWRITE_MODE, missile_pattern);
    display->endPageDrawing();

    return prev;
}

void do_update_alien(CH1115Display *display) {
  // draw aliens
  for (uint8_t r = 0; r < max_row; r++) {
    for (uint8_t i = min_col; i < max_col; i++) {
      const uint8_t *sprite = empty;
      if (aliens[i + r * NB_ALIENS_COL] == 1) {
        sprite = alien;
      } else if (aliens[i + r * NB_ALIENS_COL] == 2) {
        sprite = alien2;
      } else if (aliens[i + r * NB_ALIENS_COL] == 3) {
        sprite = explosion_frames;
      } else if (aliens[i + r * NB_ALIENS_COL] == 4) {
        sprite = explosion_frames + SPRITE_WIDTH;
      } else if (aliens[i + r * NB_ALIENS_COL] == 5) {
        sprite = explosion_frames + 2 * SPRITE_WIDTH;
      } else if (aliens[i + r * NB_ALIENS_COL] == 6) {
        sprite = explosion_frames + 3 * SPRITE_WIDTH;
      } else if (aliens[i + r * NB_ALIENS_COL] == 7) {
        sprite = empty;
      }

      if (sprite) {
        display->drawSprite(alien_x_pos + (i - min_col) * ALIEN_X_SPACING,
                            alien_y_pos + r * ALIEN_Y_SPACING, SPRITE_WIDTH,
                            SPRITE_HEIGHT, sprite, OVERWRITE_MODE);
      }
    }
  }

  // move alien
  if (alien_x_pos >= (128 - ((max_col - min_col) * (ALIEN_X_SPACING)))) {
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
  uint8_t prev_min_col = min_col;
  for (uint8_t i = 0; i < NB_ALIENS_COL * NB_ALIEN_ROW; ++i) {
    if (aliens[i] >= 3) {
      aliens[i] += 1;
    }
    if (aliens[i] == 8) {
      aliens[i] = 0;
      // end explosion update range
      update_alien_range();
    }
  }
  if (prev_min_col != min_col) {
    alien_x_pos += (min_col - prev_min_col) * ALIEN_X_SPACING;
  }

  // --- Alien missile logic ---
  // If no missile, randomly fire one
  if (alien_missile_state == -1 && (rand() % 2 == 0)) { // 1/5 chance per frame
    // Find a random alien in bottom row that is alive
    uint8_t candidates_col[NB_ALIENS_COL];
    uint8_t candidates_row[NB_ALIENS_COL];
    uint8_t count = 0;
    for (uint8_t col = min_col; col < max_col; col++) {
      for (uint8_t r = max_row - 1; r >= 0; r--) {
        if (aliens[col + r * NB_ALIENS_COL] == 1 || aliens[col + r * NB_ALIENS_COL] == 2) {
          candidates_col[count++] = col;
          candidates_row[count++] = r;
          break; // Only need one from this column
        }
      }
    }
    if (count > 0) {
      uint8_t chosen = rand() % count;
      alien_missile_state = 1;
      alien_x_missile = alien_x_pos + (candidates_col[chosen] - min_col) * ALIEN_X_SPACING + SPRITE_WIDTH / 2;
      alien_y_missile = ((alien_y_pos + candidates_row[chosen] * ALIEN_Y_SPACING + SPRITE_HEIGHT) / 8) * 8; // Align to 8px grid
    }
  }

  // Move and draw missile if active
  if (alien_missile_state != -1) {
    if (alien_missile_state == 1) {
      // First frame, just draw missile
      alien_missile_state = 2;
    } else if (alien_missile_state == 2) {
      // Continue drawing missile
      clear_alien_missile(alien_x_missile, alien_y_missile, display);
      // Move missile down
      alien_y_missile += 4;
    }
    // Draw missile (simple vertical line)
    uint8_t prev = draw_alien_missile(alien_x_missile, alien_y_missile, display);
    
    if (prev != 0) {
      alien_hit_something(alien_x_missile, alien_y_missile-3, alien_y_missile, display);
    }

    if (alien_y_missile >= 56 && alien_x_missile < x_spaceship_position + SPRITE_WIDTH && alien_x_missile > x_spaceship_position) {
      // Hit spaceship
      alien_missile_state = -1;
      if (nb_spaceship > 0) {
        nb_spaceship -= 1;
        UserScore::CurrentScore -= 10; // Penalty for being hit
      }
    }
    // Check if missile is off screen
    if (alien_y_missile > 63) { // Assuming 64px screen height
      clear_alien_missile(alien_x_missile, alien_y_missile, display);
      alien_missile_state = -1;
    }
  }

}



void update_alien(CH1115Display *display) {
  if (alien_frame == 0) {
    do_update_alien(display);
    alien_frame = ALIEN_FRAME_COUNTER;
  }
  alien_frame--;
}

void move_alien(uint8_t direction) {
  if (direction == MOVE_INIT) {
    min_col = 0;
    max_col = NB_ALIENS_COL;
    min_row = 0;
    max_row = NB_ALIEN_ROW;
    alien_x_pos = 0;
    alien_y_pos = 0;
    alien_dx = 1;
    alien_frame = 0;

    for (uint8_t row = 0; row < NB_ALIEN_ROW; ++row) {
      for (uint8_t col = 0; col < NB_ALIENS_COL; ++col) {
        aliens[col + row * NB_ALIENS_COL] = (row < 2) ? 2 : 1;
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
  uint8_t col = (x - alien_x_pos) / ALIEN_X_SPACING + min_col;
  uint8_t row = (y - alien_y_pos) / ALIEN_Y_SPACING;

  if (col < NB_ALIENS_COL && row < NB_ALIEN_ROW) {
    // check if it is an explosion and skip it
    if (aliens[col + row * NB_ALIENS_COL] >= 3 ||
        aliens[col + row * NB_ALIENS_COL] == 0) {

      return false;
    }
    aliens[col + row * NB_ALIENS_COL] = 3; // start explosion frame
    // update score
    UserScore::CurrentScore += 10;
    
    return true;
  }

  return false;
}

uint8_t check_alien_status() {
  // count remaining aliens
  uint8_t nb = 0;
  for (uint8_t i = 0; i < NB_ALIENS_COL * NB_ALIEN_ROW; ++i) {
    nb += aliens[i];
  }

  if (nb == 0) {
    return ALIEN_LOST;
  }

  uint8_t nbdeadrow = 0;
  for (uint8_t r = NB_ALIEN_ROW - 1; r >= 0; --r) {
    nb = 0;
    for (uint8_t c = 0; c < NB_ALIENS_COL; ++c) {
      nb += aliens[c + r * NB_ALIENS_COL];
    }
    if (nb == 0) {
      nbdeadrow++;
    } else {
      break;
    }
  }

  if (nb_spaceship == 0 || alien_y_pos >= (ALIEN_Y_MAX_POS + 8 * nbdeadrow)) {
    return ALIEN_WIN;
  }

  return 0;
}