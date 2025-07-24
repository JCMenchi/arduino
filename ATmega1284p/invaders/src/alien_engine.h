#ifndef _ALIEN_ENGINE_H
#define _ALIEN_ENGINE_H

#include <stdint.h>

class CH1115Display;

void update_alien(CH1115Display *display);
void move_alien(uint8_t direction);
bool kill_alien(uint8_t x, uint8_t y);
uint8_t check_alien_status();

#endif