

#include <CH1115Display.h>
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

Nunchuk joystick;
CH1115Display ch1115(SCREEN_WIDTH, SCREEN_HEIGHT);
UserScore highscore[3];

uint8_t gameOver = 1;

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

uint8_t current_note = 0;
unsigned long start_note = 0;
char work_buffer[10];

void gameloop() {
    // music
   if (!gameOver) {
        unsigned long now = milliseconds();
        if (start_note == 0) {
            start_note = now;
            playNote(&MUSIC_PORT, &MUSIC_DDR, MUSIC_PIN, sound_loop[current_note], SOUND_LOOP_NOTE_DURATION);
        } else if (now > (start_note + SOUND_LOOP_NOTE_DURATION +
                          SOUND_LOOP_NOTE_PAUSE)) {
            stopNote();
            start_note = 0;
            current_note = (current_note + 1) % 4;
        }
    } else {
        stopNote();
    }

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
        for (size_t i = 0; i < strlen(work_buffer); ++i) {
            if (command_interpretor(work_buffer[i])) {
                break;
            }
        }
    }

    if (!gameOver) {
        drawScene(&ch1115, false);

        // check win condition
        if (check_alien_status() == ALIEN_WIN) {
            gameOver = 1;
            drawGameOver(&ch1115);
        } else if (check_alien_status() == ALIEN_LOST) {
            gameOver = 1;
            drawVictory(&ch1115);
            drawStart(&ch1115);
        }
    }
}

volatile void SERIAL_CB(uint8_t d, bool error) {
    if (error) {
        playNote(&MUSIC_PORT, &MUSIC_DDR, MUSIC_PIN, NOTE_G7, 35);
    }

    if (serial_buffer_pos == 9) {
        serial_buffer_pos = 0;
    }
    serial_buffer[serial_buffer_pos++] = d;  // & 0x7F;
}

/*
  Init and main loop
*/
void setup() {
    USART_Init(BAUD_RATE_115200, SERIAL_CB);

    USART_WriteString("Welcome.\n");
    serial_buffer_pos = 0;

    // init high score EEPROM
    UserScore_initEEPROM(highscore, 3);

    TinyI2C.init(true);

    // init OLED display
    ch1115.init(0x01);
    ch1115.flip(CH1115_ON);
    ch1115.drawScreen(0x00);

    // init nunchuk
    if (!joystick.initialize()) {
        USART_WriteString("Nunchuk not found.\n");
    } else {
        USART_WriteString("Nunchuk initialized.\n");
    }

    // draw welcome screen
    drawStart(&ch1115);
    joystick.display_calibration();
    joystick.update();
    joystick.display();
}

void loop() {
    gameloop();
    // joystick.update();
    // joystick.display();
    //_delay_ms(5000);
}

#include <main.cpp.h>
