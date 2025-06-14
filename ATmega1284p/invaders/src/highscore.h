

#ifndef _HIGHSCORE_H
#define _HIGHSCORE_H

#include <avr/eeprom.h>

#include <usart_serial.h>

const uint16_t INVADERS_TYPE=0xDECA;

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


void UserScore_initEEPROM(UserScore *highscore, size_t UserScore_size) {
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

#endif