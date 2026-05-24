#ifndef _ALIEN_ENGINE_H
#define _ALIEN_ENGINE_H

#include <stdint.h>

// Forward declaration of OLED driver.
class CH1115Display;

/**
 * @brief Update aliens: handles movement, animation, and missile logic.
 * @param display Pointer to the OLED driver.
 */
void update_alien(CH1115Display *display);

/**
 * @brief Move the alien group in a given direction or reset their position.
 * @param direction MOVE_INIT, MOVE_UP, MOVE_DOWN
 */
void move_alien(uint8_t direction);

/**
 * @brief Attempt to kill an alien at the given screen position.
 * @param x X position
 * @param y Y position
 * @return true if an alien was killed, false otherwise
 */
bool kill_alien(uint8_t x, uint8_t y);

/**
 * @brief Check the current status of the aliens (win/lose/continue).
 * @return ALIEN_LOST if all aliens are dead, ALIEN_WIN if player lost, 0 otherwise
 */
uint8_t check_alien_status();

#endif