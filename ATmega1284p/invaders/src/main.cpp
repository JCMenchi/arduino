

#include <CH1115Display.h>
#include <bitmap_font.h>
#include <highscore.h>
#include <millisec.h>
#include <nunchuk.h>
#include <stdlib.h>
#include <string.h>
#include <usart_serial.h>
#include <util/atomic.h>
#include <util/delay.h>

#include "TinyI2CMaster.h"
#include "common.h"
#include "sound.h"

//#define READ_SERIAL_LINE 1

// Define joystick thresholds
#define JOYSTICK_THRESHOLD 10

Nunchuk joystick;
CH1115Display ch1115(SCREEN_WIDTH, SCREEN_HEIGHT);
UserScore highscore[3];

uint8_t gameOver = 1;

#ifdef READ_SERIAL_LINE
char buffer[64];

bool esccommand(const char *escape_buffer) {
    if (escape_buffer[0] == '[') {
        // arrow
        if (escape_buffer[1] == 'C') {
            // right
            move_spaceship(MOVE_RIGHT);
            return true;
        } else if (escape_buffer[1] == 'D') {
            // left
            move_spaceship(MOVE_LEFT);
            return true;
        } else if (escape_buffer[1] == 'A') {
            // up
            move_alien(MOVE_UP);
            return true;
        } else if (escape_buffer[1] == 'B') {
            // down
            move_alien(MOVE_DOWN);
            return true;
        } else if (escape_buffer[1] == 'H') {
            // beg line
            return true;
        } else if (escape_buffer[1] == 'F') {
            // end line
            return true;
        } else if (escape_buffer[1] == '5' && escape_buffer[2] == '~') {
            // page up
            return true;
        } else if (escape_buffer[1] == '6' && escape_buffer[2] == '~') {
            // page down
            return true;
        }
    }

    return false;
}

volatile char serial_buffer[10];
volatile uint8_t serial_buffer_pos = 0;
char work_buffer[10];

volatile void SERIAL_CB(uint8_t d, bool error) {
    if (error) {
        playNote(&MUSIC_PORT, &MUSIC_DDR, MUSIC_PIN, NOTE_G7, 35);
    }

    if (serial_buffer_pos == 9) {
        serial_buffer_pos = 0;
    }
    serial_buffer[serial_buffer_pos++] = d;  // & 0x7F;
}

bool escape = false;
char escape_buffer[10];
uint8_t escpos = 0;

bool command_interpretor(char c) {
    if (c == 27) {  // escape
        escape = true;
        _delay_ms(1);                // delay to ensure esc sequence is available
    } else if (c > 31 && c < 127) {  // only ASCII
        if (escape) {
            escape_buffer[escpos++] = c;
            if (esccommand(escape_buffer)) {
                escape = false;
                memset(escape_buffer, 0, 10);
                escpos = 0;
                return true;
            }
        } else if (c == ' ') {
            if (gameOver) {
                gameOver = 0;
                drawScene(&ch1115, true);
            } else {
                spaceship_action(GUNFIRE_ACTION);
            }
            return true;
        } else if (c == 'l') {
            move_spaceship(MOVE_RIGHT);
            return true;
        } else if (c == 'h') {
            move_spaceship(MOVE_LEFT);
            return true;
        }
    }

    return false;
}
#endif

uint8_t current_note = 0;
unsigned long start_note = 0;

const uint8_t INIT_SCREEN = 0;
const uint8_t HIGH_SCORE_SCREEN = 1;
const uint8_t GAME_SCREEN = 2;
const uint8_t VICTORY_SCREEN = 3;
const uint8_t GAME_OVER_SCREEN = 4;
const uint8_t HIGH_SCORE_UPDATE_SCREEN = 5;

uint8_t screen_mode = INIT_SCREEN;

const uint16_t SCREEN_TIME_MS = 5000;
unsigned long init_screen_start = 0;
unsigned long high_score_screen_start = 0;

void changeToInit(uint32_t now) {
    screen_mode = INIT_SCREEN;
    init_screen_start = now;
    drawStart(&ch1115);
}

void changeToHighScore(uint32_t now) {
    screen_mode = HIGH_SCORE_SCREEN;
    high_score_screen_start = now;
    drawHighScore(&ch1115, highscore, 3);
}

void changeToHighScoreUpdate(uint32_t now, int8_t pos) {
    screen_mode = HIGH_SCORE_UPDATE_SCREEN;
    high_score_screen_start = now;

    UserScore::CurrentUserPos = pos;
    UserScore::CurrentUserCharPos = 0;
    drawHighScoreUpdate(&ch1115, highscore, 3, pos);
}

void changeToGameOver(uint32_t now) {
    screen_mode = GAME_OVER_SCREEN;
    drawGameOver(&ch1115);
}

void changeToVictory(uint32_t now) {
    screen_mode = VICTORY_SCREEN;
    drawVictory(&ch1115);
}

bool joystick_interpretor(Nunchuk *joystick, bool changed) {
    if (changed && (screen_mode == INIT_SCREEN || screen_mode == HIGH_SCORE_SCREEN) && joystick->c_button()) {
        screen_mode = GAME_SCREEN;
        UserScore::CurrentScore = 0;
        drawScene(&ch1115, true);
        joystick->display();
        return true;
    } else if (screen_mode == GAME_SCREEN && joystick->z_button()) {
        spaceship_action(GUNFIRE_ACTION);
        return true;
    } else if (screen_mode == GAME_SCREEN && joystick->joystick_x() > joystick->joystick_x_center() + JOYSTICK_THRESHOLD) {
        move_spaceship(MOVE_RIGHT);
        return true;
    } else if (screen_mode == GAME_SCREEN && joystick->joystick_x() < joystick->joystick_x_center() - JOYSTICK_THRESHOLD) {
        move_spaceship(MOVE_LEFT);
        return true;
    } else if (changed && screen_mode == HIGH_SCORE_UPDATE_SCREEN && joystick->joystick_y() > joystick->joystick_y_center() + 4 * JOYSTICK_THRESHOLD) {
        UserScore::UpdateUserName(highscore, 3, MOVE_UP);
        return true;
    } else if (changed && screen_mode == HIGH_SCORE_UPDATE_SCREEN && joystick->joystick_y() < joystick->joystick_y_center() - 4 * JOYSTICK_THRESHOLD) {
        UserScore::UpdateUserName(highscore, 3, MOVE_DOWN);
        return true;
    } else if (changed && screen_mode == HIGH_SCORE_UPDATE_SCREEN && joystick->joystick_x() > joystick->joystick_x_center() + 6 * JOYSTICK_THRESHOLD) {
        UserScore::UpdateUserName(highscore, 3, MOVE_RIGHT);
        return true;
    } else if (changed && screen_mode == HIGH_SCORE_UPDATE_SCREEN && joystick->joystick_x() < joystick->joystick_x_center() - 6 * JOYSTICK_THRESHOLD) {
        UserScore::UpdateUserName(highscore, 3, MOVE_LEFT);
        return true;
    } else if (changed && screen_mode == HIGH_SCORE_UPDATE_SCREEN && joystick->z_button()) {
        USART_WriteString("Validate High Score username\n");
        UserScore_saveEEPROM(highscore, 3);
        UserScore::CurrentScore = 0;
        UserScore::CurrentUserPos = -1;
        UserScore::CurrentUserCharPos = -1;
        _delay_ms(50);
        changeToInit(milliseconds());
        _delay_ms(50);
        joystick->update();
        return true;
    }

    return false;
}

void gameloop() {
    uint32_t now = milliseconds();
    bool changed = joystick.update();
    joystick_interpretor(&joystick, changed);

    if (screen_mode == INIT_SCREEN) {
        if (now > (init_screen_start + SCREEN_TIME_MS)) {
            changeToHighScore(now);
            _delay_ms(50);
        }
    } else if (screen_mode == HIGH_SCORE_SCREEN) {
        if (now > (high_score_screen_start + 2 * SCREEN_TIME_MS)) {
            changeToInit(now);
            _delay_ms(50);
        }
    } else if (screen_mode == HIGH_SCORE_UPDATE_SCREEN) {
        updateHighScore(&ch1115, highscore, 3);
        _delay_ms(50);
    } else if (screen_mode == GAME_SCREEN) {
        // music loop
        if (start_note == 0) {
            start_note = now;
            playNote(&MUSIC_PORT, &MUSIC_DDR, MUSIC_PIN, sound_loop[current_note], SOUND_LOOP_NOTE_DURATION);
        } else if (now > (start_note + SOUND_LOOP_NOTE_DURATION + SOUND_LOOP_NOTE_PAUSE)) {
            stopNote();
            start_note = 0;
            current_note = (current_note + 1) % 4;
        }

        drawScene(&ch1115, false);

        // check win condition
        uint8_t alien_status = check_alien_status();
        if (alien_status != 0) {
            // draw result
            if (alien_status == ALIEN_WIN) {
                changeToGameOver(now);
            } else if (alien_status == ALIEN_LOST) {
                changeToVictory(now);
            }
            // reset number of life
            nb_spaceship = MAX_LIFE;

            // check high score
            int8_t changed = UserScore::UpdateHighScore(highscore, 3, UserScore::CurrentScore);
            now = milliseconds();
            if (changed != -1) {
                USART_WriteString("New score detected: ");
                USART_WriteInt(changed);
                USART_WriteString(" ");
                USART_WriteInt(UserScore::CurrentScore);
                USART_WriteString("\n");
                changeToHighScoreUpdate(now, changed);
                UserScore_saveEEPROM(highscore, 3);
            } else {
                changeToInit(now);
            }
        }
    }

#ifdef READ_SERIAL_LINE
    ATOMIC_BLOCK(ATOMIC_RESTORESTATE) {
        if (serial_buffer_pos) {
            memcpy(work_buffer, (const void *)serial_buffer, 10);
            work_buffer[serial_buffer_pos] = 0;
            serial_buffer_pos = 0;
        } else {
            work_buffer[0] = 0;
        }
    }

    if (work_buffer[0] != 0) {
        escape = false;
        memset(escape_buffer, 0, 10);
        escpos = 0;
        USART_WriteString(work_buffer);
        if (strcmp(work_buffer, "score") == 0) {
            USART_WriteString("set score\n");
            highscore[1].reset(42);
            changeToHighScoreUpdate(milliseconds(), 1);
            return;
        } else if (strcmp(work_buffer, "reset") == 0) {
            USART_WriteString("reset score\n");
            highscore[0].reset();
            highscore[1].reset();
            highscore[2].reset();
            UserScore_saveEEPROM(highscore, 3);
            return;
        } else if (strcmp(work_buffer, "gameover") == 0) {
            // game over command
            changeToGameOver(now);
            return;
        } else if (strcmp(work_buffer, "victory") == 0) {
            // victory command
            changeToVictory(now);
            return;
        }
        for (size_t i = 0; i < strlen(work_buffer); ++i) {
            if (command_interpretor(work_buffer[i])) {
                break;
            }
        }
    }
#endif
}

/*
  Init and main loop
*/
void setup() {
#ifdef READ_SERIAL_LINE
    USART_Init(BAUD_RATE_115200, SERIAL_CB);
    serial_buffer_pos = 0;
#else
    USART_Init(BAUD_RATE_115200, NULL);
#endif

    USART_WriteString("Welcome.\n");

    // init high score EEPROM
    UserScore_initEEPROM(highscore, 3);

    TinyI2C.init(true);
    _delay_ms(1000);

    // init OLED display
    ch1115.init(0x01);
    ch1115.flip(CH1115_ON);
    ch1115.drawScreen(0x00);
    _delay_ms(1000);

    // init nunchuk
    if (!joystick.initialize()) {
        USART_WriteString("Nunchuk not found.\n");
    } else {
        USART_WriteString("Nunchuk initialized.\n");
    }
    _delay_ms(1000);

    // draw welcome screen
    drawStart(&ch1115);
    joystick.display_calibration();
    joystick.update();
    joystick.display();

    // init random generator
    srand(milliseconds());
}

void loop() {
    gameloop();
}

#include <main.cpp.h>
