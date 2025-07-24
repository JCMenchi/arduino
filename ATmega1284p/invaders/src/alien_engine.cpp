#include "common.h"
#include "sprites.h"
#include <CH1115Display.h>
#include <stdlib.h>
#include <highscore.h>

#include <spaceship_engine.h>
#include <alien_engine.h>

#ifdef HAS_SERIAL
#include <usart_serial.h>
#endif

// --- Alien configuration constants ---
const uint8_t NB_ALIEN_COL = 8;      // Number of alien columns
const uint8_t NB_ALIEN_ROW = 4;       // Number of alien rows

const uint8_t ALIEN_X_MAX_POS = 16;   // Max X position for aliens
const uint8_t ALIEN_Y_MAX_POS = 16;   // Max Y position for aliens

const uint8_t ALIEN_X_SPACING = 15;   // Horizontal spacing between aliens
const uint8_t ALIEN_Y_SPACING = 8;    // Vertical spacing between aliens

// --- Alien state variables ---
uint8_t aliens[NB_ALIEN_COL * NB_ALIEN_ROW]; // Alien grid state
uint8_t min_col = 0, max_col = NB_ALIEN_COL; // Active alien columns
uint8_t min_row = 0, max_row = NB_ALIEN_ROW;  // Active alien rows

uint8_t alien_x_pos = 0;              // X position of alien group
uint8_t alien_y_pos = 0;              // Y position of alien group
int8_t alien_dx = 1;                 // Alien movement direction (1=right, -1=left)

const uint8_t ALIEN_FRAME_COUNTER = 3;// Animation frame counter
uint8_t alien_frame = 0;              // Current animation frame

uint8_t dont_go_down = 5;             // Delay before aliens move down

// --- Alien missile state ---
int8_t alien_missile_state = -1;      // -1: no missile, 1: just fired, 2: moving
uint8_t alien_x_missile = 0;          // Missile X position
uint8_t alien_y_missile = 0;          // Missile Y position

/**
 * @brief Update the range of active aliens (min/max rows and columns).
 *        Used to optimize drawing and movement.
 */
void update_alien_range() {
  min_col = 0;
  max_col = NB_ALIEN_COL;
  min_row = 0;
  max_row = NB_ALIEN_ROW;

  uint8_t nb = 0;

  // Find the last non-empty row (max_row)
  for (uint8_t r = NB_ALIEN_ROW - 1; r >= 0; --r) {
    nb = 0;
    for (uint8_t c = 0; c < NB_ALIEN_COL; ++c) {
      if (aliens[c + r * NB_ALIEN_COL])
        nb++;
    }
    if (nb == 0) {
      max_row--;
    } else {
      break;
    }
  }

  // Find the first non-empty row (min_row)
  for (uint8_t r = 0; r < NB_ALIEN_ROW; ++r) {
    nb = 0;
    for (uint8_t c = 0; c < NB_ALIEN_COL; ++c) {
      if (aliens[c + r * NB_ALIEN_COL])
        nb++;
    }
    if (nb == 0) {
      min_row++;
    } else {
      break;
    }
  }

  // Find the last non-empty column (max_col)
  for (uint8_t c = NB_ALIEN_COL - 1; c >= 0; --c) {
    nb = 0;
    for (uint8_t r = 0; r < NB_ALIEN_ROW; ++r) {
      if (aliens[c + r * NB_ALIEN_COL])
        nb++;
    }
    if (nb == 0) {
      max_col--;
    } else {
      break;
    }
  }

  // Find the first non-empty column (min_col)
  for (uint8_t c = 0; c < NB_ALIEN_COL; ++c) {
    nb = 0;
    for (uint8_t r = 0; r < NB_ALIEN_ROW; ++r) {
      if (aliens[c + r * NB_ALIEN_COL])
        nb++;
    }
    if (nb == 0) {
      min_col++;
    } else {
      break;
    }
  }
}

/**
 * @brief Handle what happens when an alien missile hits something (e.g., shelter).
 * @param x X position of impact
 * @param ymin Minimum Y of impact
 * @param ymax Maximum Y of impact
 * @param display Pointer to display object
 */
void alien_hit_something(uint8_t x, uint8_t ymin, uint8_t ymax, CH1115Display *display) {
  if ((ymin <= 55 && ymin >= 48) || (ymax <= 55 && ymax >= 48)) {
    #ifdef HAS_SERIAL
    USART_WriteString("Alien missile hit shelter\n");
    #endif
    // Hit shelter
    alien_missile_state = -1;
    display->startPageDrawing(x - 1, 48);
    display->updatePageColumn(0x00, OVERWRITE_MODE);
    display->updatePageColumn(0x00, OVERWRITE_MODE);
    display->updatePageColumn(0x00, OVERWRITE_MODE);
    display->endPageDrawing();
  }
}

/**
 * @brief Erase the previous alien missile from the display.
 * @param x X position
 * @param y Y position
 * @param display Pointer to display object
 */
void clear_alien_missile(uint8_t x, uint8_t y, CH1115Display *display) {
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

/**
 * @brief Draw the alien missile at its current position.
 * @param x X position
 * @param y Y position
 * @param display Pointer to display object
 * @return Previous pixel data (for collision)
 */
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

/**
 * @brief Main update function for aliens: draws, moves, animates, and handles missiles.
 * @param display Pointer to display object
 */
void do_update_alien(CH1115Display *display) {
  // Draw all aliens in their current positions
  for (int8_t r = min_row; r < max_row; r++) {
    for (int8_t i = min_col; i < max_col; i++) {
      const uint8_t *sprite = empty;
      if (aliens[i + r * NB_ALIEN_COL] == 1) {
        sprite = alien;
      } else if (aliens[i + r * NB_ALIEN_COL] == 2) {
        sprite = alien2;
      } else if (aliens[i + r * NB_ALIEN_COL] == 3) {
        sprite = explosion_frames;
      } else if (aliens[i + r * NB_ALIEN_COL] == 4) {
        sprite = explosion_frames + SPRITE_WIDTH;
      } else if (aliens[i + r * NB_ALIEN_COL] == 5) {
        sprite = explosion_frames + 2 * SPRITE_WIDTH;
      } else if (aliens[i + r * NB_ALIEN_COL] == 6) {
        sprite = explosion_frames + 3 * SPRITE_WIDTH;
      } else if (aliens[i + r * NB_ALIEN_COL] == 7) {
        sprite = empty;
      }

      if (sprite) {
        display->drawSprite(alien_x_pos + (i - min_col) * ALIEN_X_SPACING,
                            alien_y_pos + r * ALIEN_Y_SPACING, SPRITE_WIDTH,
                            SPRITE_HEIGHT, sprite, OVERWRITE_MODE);
      }
    }
  }

  // Move aliens horizontally and vertically as needed
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

  // Handle alien explosions and update range if needed
  uint8_t prev_min_col = min_col;
  for (uint8_t i = 0; i < NB_ALIEN_COL * NB_ALIEN_ROW; ++i) {
    if (aliens[i] >= 3) {
      aliens[i] += 1;
    }
    if (aliens[i] == 8) {
      aliens[i] = 0;
      update_alien_range();
    }
  }
  if (prev_min_col != min_col) {
    alien_x_pos += (min_col - prev_min_col) * ALIEN_X_SPACING;
  }

  // --- Alien missile logic ---
  // If no missile, randomly fire one from a living alien in the bottom row
  if (alien_missile_state == -1 && (rand() % 3 == 0)) { // 1/3 chance per frame
    uint8_t candidates_col[NB_ALIEN_COL];
    uint8_t candidates_row[NB_ALIEN_COL];
    uint8_t count = 0;
    for (int8_t col = min_col; col < max_col; col++) {
      for (int8_t r = max_row - 1; r >= 0; r--) {
        if (aliens[col + r * NB_ALIEN_COL] == 1 || aliens[col + r * NB_ALIEN_COL] == 2) {
          candidates_col[count] = col;
          candidates_row[count] = r;
          count++;
          break; // Only need one from this column
        }
      }
    }
    if (count > 0) {
      uint8_t chosen = (count > 1)?(rand() % count):0;
      alien_missile_state = 1;
      alien_x_missile = alien_x_pos + (candidates_col[chosen] - min_col) * ALIEN_X_SPACING + SPRITE_WIDTH / 2;
      alien_y_missile = ((alien_y_pos + candidates_row[chosen] * ALIEN_Y_SPACING + SPRITE_HEIGHT) / 4) * 4; // Align to 4px grid
      #ifdef HAS_SERIAL
      USART_WriteString("Alien create missile at: ");
      USART_WriteInt(alien_x_missile);
      USART_WriteString(", ");
      USART_WriteInt(alien_y_missile);
      USART_WriteString("\n");
      #endif
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
      #ifdef HAS_SERIAL
      USART_WriteString("Alien missile hit at: ");
      USART_WriteInt(alien_x_missile);
      USART_WriteString(", ");
      USART_WriteInt(alien_y_missile);
      USART_WriteString("\n");
      #endif
      alien_hit_something(alien_x_missile, alien_y_missile-3, alien_y_missile, display);
    }
    // Check if missile is off screen
    if (alien_y_missile >= 60) { // Assuming 64px screen height
      alien_y_missile = 60; // Cap it to 60, to be safe
      clear_alien_missile(alien_x_missile, alien_y_missile, display);
      alien_missile_state = -1;
    }
    // Check collision with spaceship
    if (alien_y_missile >= 56 && alien_x_missile < x_spaceship_position + SPRITE_WIDTH && alien_x_missile > x_spaceship_position) {
      #ifdef HAS_SERIAL
      USART_WritePString(PSTR("Alien missile hit spaceship at: "));
      USART_WriteInt(alien_x_missile);
      USART_WriteString(", ");
      USART_WriteInt(alien_y_missile);
      USART_WriteString(" ship pos: ");
      USART_WriteInt(x_spaceship_position);
      USART_WriteString(" prev: ");
      USART_WriteInt(prev, 16);
      
      USART_WriteString("\n");
      #endif
      // Hit spaceship
      alien_missile_state = -1;
      if (nb_spaceship > 0) {
        nb_spaceship -= 1;
        if (UserScore::CurrentScore > 10) {
          UserScore::CurrentScore -= 10; // Penalty for being hit
        } else {
          UserScore::CurrentScore = 0; // Don't go negative
        }
      }
    }
    
  }
}

/**
 * @brief Call this function every frame to update aliens (with animation timing).
 * @param display Pointer to display object
 */
void update_alien(CH1115Display *display) {
  if (alien_frame == 0) {
    do_update_alien(display);
    alien_frame = ALIEN_FRAME_COUNTER;
  }
  alien_frame--;
}

/**
 * @brief Move the alien group in a given direction or reset their position.
 * @param direction MOVE_INIT, MOVE_UP, MOVE_DOWN
 */
void move_alien(uint8_t direction) {
  if (direction == MOVE_INIT) {
    min_col = 0;
    max_col = NB_ALIEN_COL;
    min_row = 0;
    max_row = NB_ALIEN_ROW;
    alien_x_pos = 0;
    alien_y_pos = 0;
    alien_dx = 1;
    alien_frame = 0;

    // Initialize aliens: top 2 rows type 2, bottom 2 rows type 1
    for (uint8_t row = 0; row < NB_ALIEN_ROW; ++row) {
      for (uint8_t col = 0; col < NB_ALIEN_COL; ++col) {
        aliens[col + row * NB_ALIEN_COL] = (row < 2) ? 2 : 1;
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

/**
 * @brief Attempt to kill an alien at the given screen position.
 * @param x X position
 * @param y Y position
 * @return true if an alien was killed, false otherwise
 */
bool kill_alien(uint8_t x, uint8_t y) {
  uint8_t col = (x - alien_x_pos) / ALIEN_X_SPACING + min_col;
  uint8_t row = (y - alien_y_pos) / ALIEN_Y_SPACING;

  if (col < NB_ALIEN_COL && row < NB_ALIEN_ROW) {
    // check if it is an explosion and skip it
    if (aliens[col + row * NB_ALIEN_COL] >= 3 ||
        aliens[col + row * NB_ALIEN_COL] == 0) {

      return false;
    }
    aliens[col + row * NB_ALIEN_COL] = 3; // start explosion frame
    // update score
    UserScore::CurrentScore += 10;
    
    return true;
  }

  return false;
}

/**
 * @brief Check the current status of the aliens (win/lose/continue).
 * @return ALIEN_LOST if all aliens are dead, ALIEN_WIN if player lost, 0 otherwise
 */
uint8_t check_alien_status() {
  // count remaining aliens
  uint8_t nb = 0;
  for (uint8_t i = 0; i < NB_ALIEN_COL * NB_ALIEN_ROW; ++i) {
    nb += aliens[i];
  }

  if (nb == 0) {
    return ALIEN_LOST;
  }

  uint8_t nbdeadrow = 0;
  for (uint8_t r = NB_ALIEN_ROW - 1; r >= 0; --r) {
    nb = 0;
    for (uint8_t c = 0; c < NB_ALIEN_COL; ++c) {
      nb += aliens[c + r * NB_ALIEN_COL];
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