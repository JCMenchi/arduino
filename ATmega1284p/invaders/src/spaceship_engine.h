#ifndef _SPACESHIP_ENGINE_H
#define _SPACESHIP_ENGINE_H

#include <stdint.h>

// Forward declaration of display class
class CH1115Display;

// Global variable: current X position of the spaceship
extern uint8_t x_spaceship_position;

// Maximum number of lives for the spaceship
const uint8_t MAX_LIFE = 3;

// Global variable: number of spaceships (lives) remaining
extern uint8_t nb_spaceship;

/**
 * @brief Updates the spaceship state and draws it on the display.
 * @param display Pointer to the display object.
 */
void update_spaceship(CH1115Display *display);

/**
 * @brief Moves the spaceship in the specified direction.
 * @param direction 0 = left, 1 = right.
 */
void move_spaceship(uint8_t direction);

/**
 * @brief Performs an action with the spaceship (e.g., fire missile).
 * @param action Action code.
 */
void spaceship_action(uint8_t action);

/**
 * @brief Draws the shelter(s) on the display.
 * @param display Pointer to the display object.
 */
void draw_shelter(CH1115Display *display);

/**
 * @brief Checks the status of the spaceship (e.g., alive or destroyed).
 * @return Status code (implementation-defined).
 */
uint8_t check_spaceship_status();

/**
 * @brief Clears a missile from the display at the given coordinates.
 * @param x X coordinate.
 * @param y Y coordinate.
 * @param display Pointer to the display object.
 */
void clear_missile(uint8_t x, uint8_t y, CH1115Display *display);

/**
 * @brief spaceship is destroyed by alien. Decreases the number of spaceships.
 * @return true if this is the last spaceship, false otherwise
 */
bool kill_spaceship();

#endif