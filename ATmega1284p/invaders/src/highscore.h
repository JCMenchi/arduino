

#ifndef _HIGHSCORE_H
#define _HIGHSCORE_H

#include <avr/eeprom.h>

#include <usart_serial.h>

const uint16_t INVADERS_TYPE=0xDECA;

const uint8_t UserScore_size = 6;

struct UserScore {
  static uint32_t CurrentScore;

  static int8_t CurrentUserPos;
  static int8_t CurrentUserCharPos;

  UserScore() : score(0) {
    user[0] = '_';
    user[1] = '_';
    user[2] = '_';
    user[3] = 0;
  }

  static int8_t UpdateHighScore(UserScore* scores, uint8_t count, uint32_t newScore);
  static void UpdateUserName(UserScore *scores, uint8_t count, uint8_t direction);

  uint16_t score;
  char user[4];

  void store(uint16_t offset) {
    eeprom_update_word((uint16_t *)offset, score);
    eeprom_update_byte((uint8_t *)(offset + 2), user[0]);
    eeprom_update_byte((uint8_t *)(offset + 3), user[1]);
    eeprom_update_byte((uint8_t *)(offset + 4), user[2]);
  }

  void load(uint16_t offset) {
    score = eeprom_read_word((uint16_t *)offset);
    user[0] = eeprom_read_byte((uint8_t *)(offset + 2));
    user[1] = eeprom_read_byte((uint8_t *)(offset + 3));
    user[2] = eeprom_read_byte((uint8_t *)(offset + 4));
    user[3] = 0;
  }

  void display() {
    USART_WriteString("User: ");
    USART_WriteString(user);
    USART_WriteString(" score: ");
    USART_WriteInt(score);
    USART_WriteString("\n");
  }

  void copy(const UserScore &other) {
    score = other.score;
    user[0] = other.user[0];
    user[1] = other.user[1];
    user[2] = other.user[2];
    user[3] = 0;
  }

  void reset(uint32_t newScore = 0) {
    score = newScore;
    user[0] = 'A';
    user[1] = 'A';
    user[2] = 'A';
    user[3] = 0;
  }

};


void UserScore_initEEPROM(UserScore *highscore, size_t UserScore_size);
void UserScore_saveEEPROM(UserScore *highscore, size_t UserScore_size);

#endif