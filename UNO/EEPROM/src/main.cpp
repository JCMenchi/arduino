#include <Arduino.h>

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
    Serial.print("User: ");
    Serial.print(user);
    Serial.print(" score: ");
    Serial.println(score);
  }
};
UserScore highscore[3];

void loadEEPROM() {
  uint16_t marker = eeprom_read_word((uint16_t *)2);

  highscore[0].load(4);
  highscore[1].load(4 + UserScore_size);
  highscore[2].load(4 + 2 * UserScore_size);
}

void showEEPROM() {
  uint16_t marker = eeprom_read_word((uint16_t *)2);

  Serial.print("EEPROM header is: ");
  Serial.println(marker, 16);

  highscore[0].display();
  highscore[1].display();
  highscore[2].display();
}

const uint8_t MAX_COLUMN = 81;
uint8_t textpos = 0;
char line_buffer[MAX_COLUMN + 1];

void reset_line() {
  memset(line_buffer, 0, MAX_COLUMN + 1);
  textpos = 0;
}

void setup() {
  Serial.begin(57600);
  Serial.println("Welcome");

  reset_line();

  // init EEPROM
  loadEEPROM();
}

void command(const char *c) {

  Serial.println(c);

  if (strcmp(c, "show") == 0) {
    showEEPROM();
  } else if (strncmp(c, "setname", 7) == 0) {
    char idx = c[8];
    const char *n = c + 10;
    uint8_t i = idx - '0';

    if (i <= 2) {
      strncpy(highscore[i].user, n, 3);
    }
  } else if (strncmp(c, "setscore", 8) == 0) {
    char idx = c[9];
    const char *n = c + 11;
    uint8_t i = idx - '0';
    if (i <= 2) {
      highscore[i].score = atoi(n);
    }
  } else if (strcmp(c, "save") == 0) {
    highscore[0].store(4);
    highscore[1].store(4 + UserScore_size);
    highscore[2].store(4 + 2 * UserScore_size);
  }
}

void loop() {
  uint8_t readchar = 0;
  bool escape = false;

  while (Serial.available()) {
    readchar++;
    char c = Serial.read();
    if (c == '\n' || c == '\r') {
      command(line_buffer);
      reset_line();
    } else if (c == 27) { // escape
      escape = true;
    } else if (c > 31 && c < 127) { // only ASCII
      if (!escape) {
        line_buffer[textpos] = c;
        textpos++;
      }
    }
  }
}