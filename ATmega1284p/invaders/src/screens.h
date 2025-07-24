#ifndef _SCREENS_H
#define _SCREENS_H

class CH1115Display;

void drawGameOver(CH1115Display *display);
void drawStart(CH1115Display *display);
void drawVictory(CH1115Display *display);

void drawScene(CH1115Display *display, bool first);


#endif