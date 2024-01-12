#ifndef _COMMON_H
#define _COMMON_H

#include <stdint.h>

class CH1115Display;

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64

#define GAME_OVER 100
#define ALIEN_WIN 101
#define ALIEN_LOST 102

#define MOVE_INIT 0
#define MOVE_LEFT 1
#define MOVE_RIGHT 2
#define MOVE_DOWN 3
#define MOVE_UP 4

#define GUNFIRE_ACTION 1

void update_spaceship(CH1115Display *display);
void move_spaceship(uint8_t direction);
void spaceship_action(uint8_t action);
void draw_shelter(CH1115Display *display);
uint8_t check_spaceship_status();


#define ALIEN_FRAME_COUNTER 3
extern uint8_t alien_frame;

void update_alien(CH1115Display *display);
void move_alien(uint8_t direction);
void kill_alien(uint8_t x, uint8_t y);
uint8_t check_alien_status();


void drawGameOver(CH1115Display *display);
void drawStart(CH1115Display *display);
void drawVictory(CH1115Display *display);
void drawScene(CH1115Display *display, bool first);

#endif
