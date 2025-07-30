#include <CH1115Display.h>
#include <millisec.h>

#include <common.h>
#include <sprites.h>
#include <spaceship_engine.h>
#include <alien_engine.h>
#include <highscore.h>
#include <soundmanager.h>

#ifdef HAS_SERIAL
#include <usart_serial.h>
#endif


// Number of spaceship lives
uint8_t nb_spaceship = MAX_LIFE;

// State of the missile: 0 = not fired, 1 = in flight
uint8_t missile_state = 0;

// Missile position
uint8_t x_missile = 0;
uint8_t y_missile = 0;

// Spaceship position (horizontal)
uint8_t x_spaceship_position = 64;
uint8_t x_prev_position = 64;

// manage explosion state
int8_t explosion_state = -1;

// Move the spaceship left, right, or initialize its position
void move_spaceship(uint8_t direction) {
    if (explosion_state != -1) {
        return;
    }

    x_prev_position = x_spaceship_position;

    if (direction == MOVE_INIT) {
        x_spaceship_position = 64;
        x_prev_position = 64;
        if (x_spaceship_position % 4) {
          #ifdef HAS_SERIAL
          USART_WriteString("Init spaceship at: ");
          USART_WriteInt(x_spaceship_position);
          USART_WriteString("\n");
          #endif
        }
    } else if (direction == MOVE_LEFT && x_spaceship_position > 0) {
        x_spaceship_position -= 1;
#ifdef HAS_SERIAL
        if (x_spaceship_position % 4 == 0) {
          USART_WriteString("Move spaceship at: ");
          USART_WriteInt(x_spaceship_position);
          USART_WriteString("\n");
        }
#endif
    } else if (direction == MOVE_RIGHT && x_spaceship_position < (SCREEN_WIDTH - 9)) {
        x_spaceship_position += 1;
#ifdef HAS_SERIAL
        if (x_spaceship_position % 4 == 0) {
          USART_WriteString("Move spaceship at: ");
          USART_WriteInt(x_spaceship_position);
          USART_WriteString("\n");
        }
#endif
    }
}

// Handle spaceship actions (e.g., firing missile)
void spaceship_action(uint8_t action) {
    if (action == GUNFIRE_ACTION && missile_state == 0 && explosion_state == -1) {
        SoundManager::instance()->sound_effect(SoundManager::FIRE_EFFECT);
        //playNote(&MUSIC_PORT, &MUSIC_DDR, MUSIC_PIN, NOTE_G4, 35);
        x_missile = x_spaceship_position + SPRITE_WIDTH / 2;
        y_missile = 0;
        missile_state = 1;
    }
}

// Handle missile collision with shelter or alien
void hit_something(uint8_t x, uint8_t ymin, uint8_t ymax, CH1115Display *display) {
    if ((ymin <= 55 && ymin >= 48) || (ymax <= 55 && ymax >= 48)) {
        // Hit shelter
        SoundManager::instance()->sound_effect(SoundManager::HIT_SHELTER_EFFECT);
        //stopNote();
        //playNote(&MUSIC_PORT, &MUSIC_DDR, MUSIC_PIN, NOTE_D1, 100);
        missile_state = 0;
        y_missile = 0;
        display->startPageDrawing(x_missile - 1, 48);
        display->updatePageColumn(0x00, OVERWRITE_MODE);
        display->updatePageColumn(0x00, OVERWRITE_MODE);
        display->updatePageColumn(0x00, OVERWRITE_MODE);
        display->endPageDrawing();
        if (UserScore::CurrentScore > 1) {
            UserScore::CurrentScore -= 1;  // Penalty for hitting shelter
        } else {
            UserScore::CurrentScore = 0;  // Don't go negative
        }
    } else {
        // Check if hit alien
        bool k = kill_alien(x, ymin) || kill_alien(x, ymax);
        if (k) {
            SoundManager::instance()->sound_effect(SoundManager::HIT_ALIEN_EFFECT);
            //stopNote();
            //playNote(&MUSIC_PORT, &MUSIC_DDR, MUSIC_PIN, NOTE_C2, 100);
            clear_missile(x_missile, y_missile, display);
            missile_state = 0;
            y_missile = 0;
        }
    }
}

// Draw the missile at given position
uint8_t draw_missile(uint8_t x, uint8_t y, CH1115Display *display) {
    uint8_t missile_pattern = 0xF0;
    uint8_t shift = y_missile % 8;
    if (shift == 7) {
        missile_pattern = 0xF0;
    } else if (shift == 3) {
        missile_pattern = 0x0F;
    }

    display->startPageDrawing(x_missile, y_missile);
    uint8_t prev =
        display->updatePageColumn(missile_pattern, OVERWRITE_MODE, missile_pattern);
    display->endPageDrawing();

    return prev;
}

// Erase the missile from previous position
void clear_missile(uint8_t x, uint8_t y, CH1115Display *display) {
    // erase previous missile
    uint8_t missile_pattern = 0xF0;
    uint8_t shift = y_missile % 8;
    if (shift == 7) {
        missile_pattern = 0xF0;
    } else if (shift == 3) {
        missile_pattern = 0x0F;
    }

    display->startPageDrawing(x, y);
    display->updatePageColumn(0x00, OVERWRITE_MODE, missile_pattern);
    display->endPageDrawing();
}

// Update missile position and handle collisions
void update_missile(CH1115Display *display) {
    if (y_missile != 0) {
        // erase previous missile
        clear_missile(x_missile, y_missile, display);

        // move missile
        y_missile -= 4;
    } else {
        y_missile = 55;
    }

    // draw new missile
    if (y_missile >= 7) {
        uint8_t prev = draw_missile(x_missile, y_missile, display);

        if (prev != 0) {
            hit_something(x_missile, y_missile - 3, y_missile, display);
        }

    } else {
        missile_state = 0;
        y_missile = 0;
    }
}

uint32_t prev_update_explosion_time = 0;

// Draw spaceship and update missile if needed
void update_spaceship(CH1115Display *display) {
    
    // draw life indicators
    for (int8_t i = 0; i < nb_spaceship -1; i++) {
        display->drawSprite( i * SMALL_SPRITE_WIDTH, 56, SMALL_SPRITE_WIDTH, SPRITE_HEIGHT,small_spaceship, OVERWRITE_MODE);
    }
    // clear draw life indicators
    for (int8_t i = nb_spaceship-1; i < MAX_LIFE-1; i++) {
        display->drawSprite( i * SMALL_SPRITE_WIDTH, 56, SMALL_SPRITE_WIDTH, SPRITE_HEIGHT,empty, OVERWRITE_MODE);
    }

    // If spaceship is destroyed, show explosion animation
    if (explosion_state != -1) {
        // Handle explosion animation
        display->drawSprite(x_spaceship_position, 56, SPRITE_WIDTH, SPRITE_HEIGHT,
                            explosion_frames + explosion_state * SPRITE_WIDTH,
                            OVERWRITE_MODE);
        uint32_t now = milliseconds();
        if (prev_update_explosion_time == 0) {
            //playNote(&MUSIC_PORT, &MUSIC_DDR, MUSIC_PIN, NOTE_C2, 200);
            prev_update_explosion_time = now;
        } else if (now - prev_update_explosion_time > 400) {
            //playNote(&MUSIC_PORT, &MUSIC_DDR, MUSIC_PIN, NOTE_C2, 200);
            explosion_state++;
            prev_update_explosion_time = now;
        }   

        if (explosion_state >= 5) {
            explosion_state = -1;  // Reset after last frame
            prev_update_explosion_time = 0;
        }
        
    } else {
        // Draw the spaceship normally
        display->drawSprite(x_spaceship_position, 56, SPRITE_WIDTH, SPRITE_HEIGHT, spaceship,
                            OVERWRITE_MODE);
        if (missile_state) {
            update_missile(display);
        }
    }

}

// Draw all shelters on the screen
void draw_shelter(CH1115Display *display) {
    display->drawSprite(16, 48, SHELTER_WIDTH, SPRITE_HEIGHT, shelter,
                        OVERWRITE_MODE);
    display->drawSprite(16 + 14 + 27, 48, SHELTER_WIDTH, SPRITE_HEIGHT, shelter,
                        OVERWRITE_MODE);
    display->drawSprite(16 + 14 + 27 + 14 + 27, 48, SHELTER_WIDTH, SPRITE_HEIGHT,
                        shelter, OVERWRITE_MODE);
}

// Check spaceship status (stub, always returns 0)
uint8_t check_spaceship_status() { 
    if (nb_spaceship == 0) {
        return ALIEN_WIN;  // Spaceship is destroyed
    }    
    return 0; 
}

// Handle spaceship destruction by alien
// Decreases the number of spaceships and returns true if this was the last one
// If the score is greater than 10, it deducts 10 points, otherwise sets it to 0
bool kill_spaceship() {
    if (nb_spaceship > 0) {
        nb_spaceship -= 1;
        explosion_state = 0;
        if (UserScore::CurrentScore > 10) {
            UserScore::CurrentScore -= 10;  // Penalty for being hit
        } else {
            UserScore::CurrentScore = 0;  // Don't go negative
        }
        // start explosion sound
        SoundManager::instance()->sound_effect(SoundManager::EXPLOSION_EFFECT);
        // playNote(&MUSIC_PORT, &MUSIC_DDR, MUSIC_PIN, NOTE_C2, 100);
        #ifdef HAS_SERIAL
        USART_WriteString("Kill spaceship, remaining: ");
        USART_WriteInt(nb_spaceship);
        USART_WriteString("\n");
        #endif
    }

    return (nb_spaceship == 0);  // Return true if this was the last spaceship
}