

#include "common.h"
#include "sound.h"

#include <avr/eeprom.h>
#include <util/atomic.h>
#include <util/delay.h>
#include <stdlib.h>
#include <string.h>

#include <CH1115Display.h>

#include <usart_serial.h>

#include <millisec.h>

CH1115Display ch1115(SCREEN_WIDTH, SCREEN_HEIGHT);

uint8_t gameOver = 1;
unsigned long fps_start_time = 0;
uint32_t fps_nb_frame = 0;

char buffer[64];

void drawScene(CH1115Display *display, bool first) {
  unsigned long start = milliseconds();

  if (first) {
    start_sound();
    fps_start_time = 0;
    fps_nb_frame = 0;
    // reset screen effect
    display->scroll(CH1115_SCROLL_OFF);
    // init screen
    move_alien(MOVE_INIT);
    move_spaceship(MOVE_INIT);

    display->drawScreen(0x00);
    draw_shelter(&ch1115);
  }

  // update spaceship
  update_spaceship(&ch1115);
  // update alien
  update_alien(&ch1115);

  // compute FPS end, check drawing time
  unsigned long end = milliseconds();

  fps_nb_frame++;
  if (fps_start_time == 0) {
    fps_start_time = start;
  } else if ((end - fps_start_time) > 10000) {
    USART_WriteString("FPS: ");
    USART_WriteInt(((fps_nb_frame * 1000) / (end - fps_start_time)));
    USART_WriteString("\n");

    fps_start_time = 0;
    fps_nb_frame = 0;
  }

  if ((end - start) > 150) {
    USART_WriteString("Frame refresh in (ms): ");
    USART_WriteInt(end - start);
    USART_WriteString("\n");
  }
}

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

  if (c == 27) { // escape
    escape = true;
    _delay_ms(1);       // delay to ensure esc sequence is available
  } else if (c > 31 && c < 127) { // only ASCII
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

void gameloop() {
  escape = false;
  memset(escape_buffer, 0, 10);
  escpos = 0;

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

  char work_buffer[10];
  work_buffer[0] = 0;

  ATOMIC_BLOCK(ATOMIC_RESTORESTATE) {
    if (serial_buffer_pos) {
      memcpy(work_buffer, (const void *)serial_buffer, 10);
      serial_buffer_pos = 0;
    }
  }

  if (strlen(work_buffer)) {
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
  serial_buffer[serial_buffer_pos++] = d; // & 0x7F;
  serial_buffer[serial_buffer_pos] = 0;
}

#define INVADERS_TYPE 0xDECA

const uint8_t UserScore_size = 6;

struct UserScore {
  UserScore() : score(0) {
    user[0] = '_';
    user[1] = '_';
    user[2] = '_';
    user[3] = 0;
  }

  uint16_t score;
  char user[4];

  void store(uint16_t offset) {
    eeprom_update_word((uint16_t *)offset, score);
    eeprom_update_byte((uint8_t *)(offset + 2), user[0]);
    eeprom_update_byte((uint8_t *)(offset + 3), user[1]);
    eeprom_update_byte((uint8_t *)(offset + 4), user[2]);
    eeprom_update_byte((uint8_t *)(offset + 5), user[3]);
  }

  void load(uint16_t offset) {
    score = eeprom_read_word((uint16_t *)offset);
    eeprom_read_block(&user, (const void *)(offset + 2), 4);
  }

  void display() {
    USART_WriteString("User: ");
    USART_WriteString(user);
    USART_WriteString(" score: ");
    USART_WriteInt(score);
    USART_WriteString("\n");
  }
};
UserScore highscore[3];

void initEEPROM() {
  uint16_t marker = eeprom_read_word((uint16_t *)2);
  if (marker == INVADERS_TYPE) {
    USART_WriteString("EEPROM already initialized.\n");
    highscore[0].load(4);
    highscore[1].load(4 + UserScore_size);
    highscore[2].load(4 + 2 * UserScore_size);

    highscore[0].display();
    highscore[1].display();
    highscore[2].display();
  } else {
    USART_WriteString("Initialize EEPROM.\n");
    eeprom_update_word((uint16_t *)2, INVADERS_TYPE);
    highscore[0].store(4);
    highscore[1].store(4 + UserScore_size);
    highscore[2].store(4 + 2 * UserScore_size);
  }
}

/*
  Init and main loop
*/
void setup() {

  USART_Init(BAUD_RATE_115200, SERIAL_CB);

  USART_WriteString("Welcome.\n");
  serial_buffer_pos = 0;
  
  // init EEPROM
  initEEPROM();

  // init OLED display
  ch1115.init(0x01);
  ch1115.flip(CH1115_ON);
  ch1115.drawScreen(0x00);

  // draw welcome screen
  drawStart(&ch1115);
}

void loop() { gameloop(); }

#include <main.cpp.h>
