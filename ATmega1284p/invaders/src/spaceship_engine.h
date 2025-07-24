#ifndef _SPACESHIP_ENGINE_H
#define _SPACESHIP_ENGINE_H

#include <stdint.h>

class CH1115Display;

extern uint8_t x_spaceship_position;

const uint8_t MAX_LIFE = 1;
extern uint8_t nb_spaceship;

void update_spaceship(CH1115Display *display);
void move_spaceship(uint8_t direction);
void spaceship_action(uint8_t action);
void draw_shelter(CH1115Display *display);
uint8_t check_spaceship_status();
void clear_missile(uint8_t x, uint8_t y, CH1115Display *display);

#endif