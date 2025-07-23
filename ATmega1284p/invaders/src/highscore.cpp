
#include <common.h>
#include <highscore.h>
#include <CH1115Display.h>
#include <bitmap_font.h>
#include <millisec.h>

#include <avr/eeprom.h>

#ifdef HAS_SERIAL
#include <usart_serial.h>
#endif

uint32_t UserScore::CurrentScore = 0;
int8_t UserScore::CurrentUserPos = -1;
int8_t UserScore::CurrentUserCharPos = -1;

const uint8_t UserScoreSize = 5;
const uint8_t UserScoreStoreStartOffset = 4;

void UserScore_initEEPROM(UserScore *highscore, size_t nbscore) {
    uint16_t marker = eeprom_read_word((uint16_t *)2);
    if (marker == INVADERS_TYPE) {
#ifdef HAS_SERIAL
        USART_WriteString("EEPROM already initialized.\n");
#endif
        for (uint8_t i = 0; i < nbscore; ++i) {
            highscore[i].load(UserScoreStoreStartOffset + i * UserScoreSize);
            highscore[i].display();
        }
    } else {
#ifdef HAS_SERIAL
        USART_WriteString("Initialize EEPROM.\n");
#endif
        eeprom_update_word((uint16_t *)2, INVADERS_TYPE);
        for (uint8_t i = 0; i < nbscore; ++i) {
            highscore[i].store(UserScoreStoreStartOffset + i * UserScoreSize);
        }
    }
}

void UserScore_saveEEPROM(UserScore *highscore, size_t nbscore) {
#ifdef HAS_SERIAL
    USART_WriteString("Save EEPROM.\n");
#endif
    eeprom_update_word((uint16_t *)2, INVADERS_TYPE);
    for (uint8_t i = 0; i < nbscore; ++i) {
        highscore[i].store(UserScoreStoreStartOffset + i * UserScoreSize);
    }
}

void UserScore::display() {
    #ifdef HAS_SERIAL
    USART_WriteString("User: ");
    USART_WriteString(user);
    USART_WriteString(" score: ");
    USART_WriteInt(score);
    USART_WriteString("\n");
    #endif
  }

void UserScore::store(uint16_t offset) {
    eeprom_update_word((uint16_t *)offset, score);
    eeprom_update_byte((uint8_t *)(offset + 2), user[0]);
    eeprom_update_byte((uint8_t *)(offset + 3), user[1]);
    eeprom_update_byte((uint8_t *)(offset + 4), user[2]);
}

void UserScore::load(uint16_t offset) {
    score = eeprom_read_word((uint16_t *)offset);
    user[0] = eeprom_read_byte((uint8_t *)(offset + 2));
    user[1] = eeprom_read_byte((uint8_t *)(offset + 3));
    user[2] = eeprom_read_byte((uint8_t *)(offset + 4));
    user[3] = 0;
}

//
int8_t UserScore::UpdateHighScore(UserScore *scores, uint8_t nbscore, uint32_t newScore) {
    // Find position for new score
    uint8_t pos = -1;
    for (uint8_t i = 0; i < nbscore; ++i) {
        if (newScore > scores[i].score) {
            pos = i;
            break;
        }
    }
    if (pos == -1) return pos;  // Not a high score

    // Shift lower scores down
    for (uint8_t i = nbscore - 1; i > pos; --i) {
        scores[i].copy(scores[i - 1]);
    }

    // Insert new score
    scores[pos].reset(newScore);

    return pos;
}

void UserScore::UpdateUserName(UserScore *scores, uint8_t nbscore, uint8_t direction) {
    if (direction == MOVE_RIGHT) {
        if (CurrentUserCharPos < 2) {
            CurrentUserCharPos++;
        }
    } else if (direction == MOVE_LEFT) {
        if (CurrentUserCharPos > 0) {
            CurrentUserCharPos--;
        }
    } else if (direction == MOVE_DOWN) {
        if (scores[CurrentUserPos].user[CurrentUserCharPos] < 'Z') {
            scores[CurrentUserPos].user[CurrentUserCharPos] += 1;
        } else {
            scores[CurrentUserPos].user[CurrentUserCharPos] = 'A';
        }
    } else if (direction == MOVE_UP) {
        if (scores[CurrentUserPos].user[CurrentUserCharPos] > 'A') {
            scores[CurrentUserPos].user[CurrentUserCharPos] -= 1;
        } else {
            scores[CurrentUserPos].user[CurrentUserCharPos] = 'Z';
        }
    }

#ifdef HAS_SERIAL
    USART_WriteString("UpdateUserName: ");
    USART_WriteInt(CurrentUserPos);
    USART_WriteString(" ");
    USART_WriteInt(CurrentUserCharPos);
    USART_WriteString(" ");
    USART_WriteString(scores[CurrentUserPos].user);
    USART_WriteString("\n");
#endif
}

// Draws the "High Score" screen
void drawHighScore(CH1115Display *display, UserScore *highscore, uint8_t nbscore) {
  const uint8_t ybase = 4;
  display->drawScreen(0x00, true);
  display->drawString((SCREEN_WIDTH - 11 * FONT_CHAR_WIDTH)/2, ybase, "High Scores");

  display->drawString(20, ybase + FONT_CHAR_HEIGHT + 4, "1st:");
  display->drawString(20 + 6 * FONT_CHAR_WIDTH, ybase + FONT_CHAR_HEIGHT + 4, highscore[0].user);
  display->drawInt(20 + 12 * FONT_CHAR_WIDTH, ybase + FONT_CHAR_HEIGHT + 4, highscore[0].score);
  
  display->drawString(20, ybase + 2 * (FONT_CHAR_HEIGHT + 4), "2nd:");
  display->drawString(20 + 6 * FONT_CHAR_WIDTH, ybase + 2 * (FONT_CHAR_HEIGHT + 4), highscore[1].user);
  display->drawInt(20 + 12 * FONT_CHAR_WIDTH, ybase + 2 * (FONT_CHAR_HEIGHT + 4), highscore[1].score);

  display->drawString(20, ybase + 3 * (FONT_CHAR_HEIGHT + 4), "3rd:");
  display->drawString(20 + 6 * FONT_CHAR_WIDTH, ybase + 3 * (FONT_CHAR_HEIGHT + 4), highscore[2].user);
  display->drawInt(20 + 12 * FONT_CHAR_WIDTH, ybase + 3 * (FONT_CHAR_HEIGHT + 4), highscore[2].score);

  display->drawString(3, 54, "insert coins...");
  display->scrollArea(6, 7, 2, 120, CH1115_SCROLL_RIGHT, CH1115_SCROLL_6FRAMES);
  display->scroll(CH1115_SCROLL_CONTINUOUS);
}

uint32_t prevHighScoreUpdate = 0;
bool drawName = true;

void updateHighScore(CH1115Display *display, UserScore *highscore, uint8_t nbscore) {
  const uint8_t ybase = 4;
  if (UserScore::CurrentUserPos < 0 || UserScore::CurrentUserPos >= nbscore) {
    // Invalid position, do nothing
    return;
  }
  uint32_t now = milliseconds();

  if (now - prevHighScoreUpdate > 500) {
    prevHighScoreUpdate = now;
    drawName = !drawName; // Toggle name drawing every 500 ms
  }

  if (drawName) {
    display->drawString(20 + 6 * FONT_CHAR_WIDTH, ybase + (UserScore::CurrentUserPos + 1) * (FONT_CHAR_HEIGHT + 4), highscore[UserScore::CurrentUserPos].user);
  } else {
    display->drawString(20 + (6 + UserScore::CurrentUserCharPos)* FONT_CHAR_WIDTH, ybase + (UserScore::CurrentUserPos + 1) * (FONT_CHAR_HEIGHT + 4), " ");
  }
}

void drawHighScoreUpdate(CH1115Display *display, UserScore *highscore, uint8_t size, uint8_t pos) {
  const uint8_t ybase = 4;
  display->drawScreen(0x00, true);
  display->drawString((SCREEN_WIDTH - 11 * FONT_CHAR_WIDTH)/2, ybase, "High Scores");

  display->drawString(20, ybase + FONT_CHAR_HEIGHT + 4, "1st:");
  display->drawString(20 + 6 * FONT_CHAR_WIDTH, ybase + FONT_CHAR_HEIGHT + 4, highscore[0].user);
  display->drawInt(20 + 12 * FONT_CHAR_WIDTH, ybase + FONT_CHAR_HEIGHT + 4, highscore[0].score);
  
  display->drawString(20, ybase + 2 * (FONT_CHAR_HEIGHT + 4), "2nd:");
  display->drawString(20 + 6 * FONT_CHAR_WIDTH, ybase + 2 * (FONT_CHAR_HEIGHT + 4), highscore[1].user);
  display->drawInt(20 + 12 * FONT_CHAR_WIDTH, ybase + 2 * (FONT_CHAR_HEIGHT + 4), highscore[1].score);

  display->drawString(20, ybase + 3 * (FONT_CHAR_HEIGHT + 4), "3rd:");
  display->drawString(20 + 6 * FONT_CHAR_WIDTH, ybase + 3 * (FONT_CHAR_HEIGHT + 4), highscore[2].user);
  display->drawInt(20 + 12 * FONT_CHAR_WIDTH, ybase + 3 * (FONT_CHAR_HEIGHT + 4), highscore[2].score);

  display->drawString(3, 54, "Enter your name");
  
}