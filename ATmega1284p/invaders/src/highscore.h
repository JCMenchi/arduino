#ifndef _HIGHSCORE_H
#define _HIGHSCORE_H

#include <stddef.h>
#include <stdint.h>

class CH1115Display;

// Magic number to identify invaders high score data in EEPROM
const uint16_t INVADERS_TYPE=0xDECA;

/**
 * @brief Structure to store and manage a user's high score entry.
 *
 * Contains the score, user initials, and static helpers for managing
 * the high score table and user name editing.
 */
struct UserScore {
  // Current score in play
  static uint32_t CurrentScore;

  // Position of the user being edited in the high score table
  static int8_t CurrentUserPos;
  // Position of the character being edited in the user's name
  static int8_t CurrentUserCharPos;

  /**
   * @brief Default constructor. Initializes score to 0 and user name to "___".
   */
  UserScore() : score(0) {
    user[0] = '_';
    user[1] = '_';
    user[2] = '_';
    user[3] = 0;
  }

  /**
   * @brief Checks if newScore qualifies as a high score and updates the table if so.
   * @param highscores Array of UserScore entries.
   * @param nbscore Number of entries in the high score table.
   * @param newScore The new score to check.
   * @return Index where the score was inserted, or -1 if not a high score.
   */
  static int8_t UpdateHighScore(UserScore* highscores, uint8_t nbscore, uint32_t newScore);

  /**
   * @brief Updates the user name for a high score entry, based on input direction.
   * @param highscores Array of UserScore entries.
   * @param nbscore Number of entries in the high score table.
   * @param direction Direction of update (e.g., up/down/left/right).
   */
  static void UpdateUserName(UserScore *highscores, uint8_t nbscore, uint8_t direction);

  uint16_t score;   ///< The user's score.
  char user[4];     ///< The user's initials (3 chars + null terminator).

  /**
   * @brief Store this score entry to EEPROM at the given offset.
   * @param offset EEPROM offset.
   */
  void store(uint16_t offset);

  /**
   * @brief Load this score entry from EEPROM at the given offset.
   * @param offset EEPROM offset.
   */
  void load(uint16_t offset);

  /**
   * @brief Display this score entry if serial line is available
   */
  void display();

  /**
   * @brief Copy another UserScore into this one.
   * @param other The UserScore to copy from.
   */
  void copy(const UserScore &other) {
    score = other.score;
    user[0] = other.user[0];
    user[1] = other.user[1];
    user[2] = other.user[2];
    user[3] = 0;
  }

  /**
   * @brief Reset this score entry to a default or given score and name "AAA".
   * @param newScore The score to set (default 0).
   */
  void reset(uint32_t newScore = 0) {
    score = newScore;
    user[0] = 'A';
    user[1] = 'A';
    user[2] = 'A';
    user[3] = 0;
  }

};

/**
 * @brief Draws the high score table on the display.
 */
void drawHighScore(CH1115Display *display, UserScore *highscore, uint8_t nbscore);

/**
 * @brief Draws the high score table with the new score to update.
 */
void drawHighScoreUpdate(CH1115Display *display, UserScore *highscore, uint8_t nbscore, uint8_t pos);

/**
 * @brief Updates the high score display during user name edition.
 */
void updateHighScore(CH1115Display *display, UserScore *highscore, uint8_t nbscore);

/**
 * @brief Initializes the high score table from EEPROM.
 */
void UserScore_initEEPROM(UserScore *highscore, size_t nbscore);

/**
 * @brief Saves the high score table to EEPROM.
 */
void UserScore_saveEEPROM(UserScore *highscore, size_t nbscore);

#endif