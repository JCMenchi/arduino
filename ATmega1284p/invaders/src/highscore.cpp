
#include <common.h>
#include <highscore.h>

uint32_t UserScore::CurrentScore = 0;
int8_t UserScore::CurrentUserPos = -1;
int8_t UserScore::CurrentUserCharPos = -1;

void UserScore_initEEPROM(UserScore *highscore, size_t UserScore_size) {
    uint16_t marker = eeprom_read_word((uint16_t *)2);
    if (marker == INVADERS_TYPE) {
        USART_WriteString("EEPROM already initialized.\n");
        highscore[0].load(4);
        highscore[1].load(4 + 5);
        highscore[2].load(4 + 2 * 5);

        highscore[0].display();
        highscore[1].display();
        highscore[2].display();
    } else {
        USART_WriteString("Initialize EEPROM.\n");
        eeprom_update_word((uint16_t *)2, INVADERS_TYPE);
        highscore[0].store(4);
        highscore[1].store(4 + 5);
        highscore[2].store(4 + 2 * 5);
    }
}

void UserScore_saveEEPROM(UserScore *highscore, size_t UserScore_size) {
    USART_WriteString("Save EEPROM.\n");
    eeprom_update_word((uint16_t *)2, INVADERS_TYPE);
    highscore[0].store(4);
    highscore[1].store(4 + 5);
    highscore[2].store(4 + 2 * 5);
}

// Add this method:
int8_t UserScore::UpdateHighScore(UserScore *scores, uint8_t count, uint32_t newScore) {
    // Find position for new score
    uint8_t pos = -1;
    for (uint8_t i = 0; i < count; ++i) {
        if (newScore > scores[i].score) {
            pos = i;
            break;
        }
    }
    if (pos == -1) return pos;  // Not a high score

    // Shift lower scores down
    for (uint8_t i = count - 1; i > pos; --i) {
        scores[i].copy(scores[i - 1]);
    }

    // Insert new score
    scores[pos].reset(newScore);

    return pos;
}

void UserScore::UpdateUserName(UserScore *scores, uint8_t count, uint8_t direction) {

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

    USART_WriteString("UpdateUserName: ");
    USART_WriteInt(CurrentUserPos);
    USART_WriteString(" ");
    USART_WriteInt(CurrentUserCharPos);
    USART_WriteString(" ");
    USART_WriteString(scores[CurrentUserPos].user);
    USART_WriteString("\n");
}