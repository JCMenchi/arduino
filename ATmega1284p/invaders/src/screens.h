#ifndef _SCREENS_H
#define _SCREENS_H

// Forward declaration of the CH1115Display class used for drawing on the display
class CH1115Display;

// Draws the "Game Over" screen on the display
void drawGameOver(CH1115Display *display);

// Draws the start screen on the display
void drawStart(CH1115Display *display);

// Draws the victory screen on the display
void drawVictory(CH1115Display *display);

// Draws the main game scene; 'first' indicates if this is the initial draw
void drawScene(CH1115Display *display, bool first);

#endif