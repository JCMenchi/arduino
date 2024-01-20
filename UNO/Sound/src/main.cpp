

#ifdef USE_MINICORE
#include <arduimini.h>
#include <string.h>
#else
#include <Arduino.h>
#endif

#define MUSIC_PIN 8

#include "pitches.h"
#include "tunes.h"

// notes in the melody:
int melody[] = {NOTE_C4, NOTE_G3, NOTE_G3, NOTE_A3,
                NOTE_G3, 0,       NOTE_B3, NOTE_C4};
// note durations: 4 = quarter note, 8 = eighth note, etc.:
int noteDurations[] = {4, 8, 8, 4, 4, 4, 4, 4};

// AS GS FS F
// int space_i[] = {NOTE_AS1, NOTE_GS1, NOTE_FS1, NOTE_F1, NOTE_AS2, NOTE_GS2, NOTE_FS2, NOTE_F2, 
//                  NOTE_AS3, NOTE_GS3, NOTE_FS3, NOTE_F3, NOTE_AS4, NOTE_GS4, NOTE_FS4, NOTE_F4  };
// 
int space_i[] = {NOTE_AS2, NOTE_GS2, NOTE_FS2, NOTE_F2, NOTE_AS2, NOTE_GS2, NOTE_FS2, NOTE_F2, 
                 NOTE_AS2, NOTE_GS2, NOTE_FS2, NOTE_F2, NOTE_AS2, NOTE_GS2, NOTE_FS2, NOTE_F2  };

/*
  Init and main loop
*/


uint8_t duration = 1;

void play() {
  for (int thisNote = 0; thisNote < 16; thisNote++) {
    // to calculate the note duration, take one second
    // divided by the note type.
    // e.g. quarter note = 1000 / 4, eighth note = 1000/8, etc.
    int noteDuration = 800;
    tone(MUSIC_PIN, space_i[thisNote], noteDuration);
    // to distinguish the notes, set a minimum time between them.
    // the note's duration + 30% seems to work well:
    int pauseBetweenNotes = noteDuration + 200;
    delay(pauseBetweenNotes);
    // stop the tone playing:
    noTone(MUSIC_PIN);
  }
}

void playtheme() {
  const int* t = mario_main_theme;
  int v = pgm_read_word(t);

  int note_to_play = 0;
  int current_duration = 1;

  int i = 0;
  while(v != MUSIC_END) {
    if (v > 20) {
      note_to_play = v;
    } else if (v == 1) {
      // this is a pause
      delay(2000/ current_duration);
    } else {
      // duration
      current_duration = abs(v);
      if (v < 0) {
        current_duration += current_duration/2;
      }
    }

    if (current_duration && note_to_play >= NOTE_B0) {
      // to calculate the note duration, take one second
      // divided by the note type.
      // e.g. quarter note = 1000 / 4, eighth note = 1000/8, etc.
      int noteDuration = 2000 / current_duration;
      tone(MUSIC_PIN, note_to_play, noteDuration);
      // to distinguish the notes, set a minimum time between them.
      // the note's duration + 30% seems to work well:
      int pauseBetweenNotes = noteDuration * 1.0;
      delay(pauseBetweenNotes);
      // stop the tone playing:
      noTone(MUSIC_PIN);
    }

    i++;
    v = pgm_read_word(t + i);
    Serial.println(v);
  }
}

void play_note1(char n) {

  // to calculate the note duration, take one second
  // divided by the note type.
  // e.g. quarter note = 1000 / 4, eighth note = 1000/8, etc.
  int noteDuration = 1000/duration;
  int note = NOTE_B0;

  if (n == 'A') {
    note = NOTE_A1;
  } else if (n == 'B') {
    note = NOTE_B1;
  } else if (n == 'C') {
    note = NOTE_C1;
  } else if (n == 'D') {
    note = NOTE_D1;
  } else if (n == 'E') {
    note = NOTE_E1;
  } else if (n == 'F') {
    note = NOTE_F1;
  } else if (n == 'G') {
    note = NOTE_G1;
  }

  tone(MUSIC_PIN, note, noteDuration);

  // to distinguish the notes, set a minimum time between them.
  // the note's duration + 30% seems to work well:
  int pauseBetweenNotes = noteDuration * 1.30;
  delay(pauseBetweenNotes);
  // stop the tone playing:
  noTone(MUSIC_PIN);
}

void play_note2(char n) {

  // to calculate the note duration, take one second
  // divided by the note type.
  // e.g. quarter note = 1000 / 4, eighth note = 1000/8, etc.
  int noteDuration = 1000/duration;
  int note = NOTE_B0;

  if (n == 'A') {
    note = NOTE_A2;
  } else if (n == 'B') {
    note = NOTE_B2;
  } else if (n == 'C') {
    note = NOTE_C2;
  } else if (n == 'D') {
    note = NOTE_D2;
  } else if (n == 'E') {
    note = NOTE_E2;
  } else if (n == 'F') {
    note = NOTE_F2;
  } else if (n == 'G') {
    note = NOTE_G2;
  } else if (n == 'a') {
    note = NOTE_AS2;
  } else if (n == 'b') {
    note = NOTE_B2;
  } else if (n == 'c') {
    note = NOTE_CS2;
  } else if (n == 'd') {
    note = NOTE_DS2;
  } else if (n == 'e') {
    note = NOTE_E2;
  } else if (n == 'f') {
    note = NOTE_FS2;
  } else if (n == 'g') {
    note = NOTE_GS2;
  }

  tone(MUSIC_PIN, note, noteDuration);

  // to distinguish the notes, set a minimum time between them.
  // the note's duration + 30% seems to work well:
  int pauseBetweenNotes = noteDuration * 1.30;
  delay(pauseBetweenNotes);
  // stop the tone playing:
  noTone(MUSIC_PIN);
}

void play_note3(char n) {

  // to calculate the note duration, take one second
  // divided by the note type.
  // e.g. quarter note = 1000 / 4, eighth note = 1000/8, etc.
  int noteDuration = 1000/duration;
  int note = NOTE_B0;

  if (n == 'A') {
    note = NOTE_A2;
  } else if (n == 'B') {
    note = NOTE_B2;
  } else if (n == 'C') {
    note = NOTE_C2;
  } else if (n == 'D') {
    note = NOTE_D2;
  } else if (n == 'E') {
    note = NOTE_E2;
  } else if (n == 'F') {
    note = NOTE_F2;
  } else if (n == 'G') {
    note = NOTE_G2;
  } else if (n == 'a') {
    note = NOTE_A3;
  } else if (n == 'b') {
    note = NOTE_B3;
  } else if (n == 'c') {
    note = NOTE_C3;
  } else if (n == 'd') {
    note = NOTE_D3;
  } else if (n == 'e') {
    note = NOTE_E3;
  } else if (n == 'f') {
    note = NOTE_F3;
  } else if (n == 'g') {
    note = NOTE_G3;
  } 

  tone(MUSIC_PIN, note, noteDuration);

  // to distinguish the notes, set a minimum time between them.
  // the note's duration + 30% seems to work well:
  int pauseBetweenNotes = noteDuration * 1.30;
  delay(pauseBetweenNotes);
  // stop the tone playing:
  noTone(MUSIC_PIN);
}

void play_note4(char n) {

  // to calculate the note duration, take one second
  // divided by the note type.
  // e.g. quarter note = 1000 / 4, eighth note = 1000/8, etc.
  int noteDuration = 1000/duration;
  
  int note = NOTE_B0;

  if (n == 'A') {
    note = NOTE_A4;
  } else if (n == 'B') {
    note = NOTE_B4;
  } else if (n == 'C') {
    note = NOTE_C4;
  } else if (n == 'D') {
    note = NOTE_D4;
  } else if (n == 'E') {
    note = NOTE_E4;
  } else if (n == 'F') {
    note = NOTE_F4;
  } else if (n == 'G') {
    note = NOTE_G4;
  } else if (n == 'a') {
    note = NOTE_AS4;
  } else if (n == 'b') {
    note = NOTE_B4;
  } else if (n == 'c') {
    note = NOTE_CS4;
  } else if (n == 'd') {
    note = NOTE_DS4;
  } else if (n == 'e') {
    note = NOTE_E4;
  } else if (n == 'f') {
    note = NOTE_FS4;
  } else if (n == 'g') {
    note = NOTE_GS4;
  }

  tone(MUSIC_PIN, note, noteDuration);

  // to distinguish the notes, set a minimum time between them.
  // the note's duration + 30% seems to work well:
  int pauseBetweenNotes = noteDuration * 1.0;
  delay(pauseBetweenNotes);
  // stop the tone playing:
  noTone(MUSIC_PIN);
}

void play_note5(char n) {

  // to calculate the note duration, take one second
  // divided by the note type.
  // e.g. quarter note = 1000 / 4, eighth note = 1000/8, etc.
  int noteDuration = 1000/duration;
  
  int note = NOTE_B0;


  if (n == 'A') {
    note = NOTE_A5;
  } else if (n == 'B') {
    note = NOTE_B5;
  } else if (n == 'C') {
    note = NOTE_C5;
  } else if (n == 'D') {
    note = NOTE_D5;
  } else if (n == 'E') {
    note = NOTE_E5;
  } else if (n == 'F') {
    note = NOTE_F5;
  } else if (n == 'G') {
    note = NOTE_G5;
  } 

  tone(MUSIC_PIN, note, noteDuration);

  // to distinguish the notes, set a minimum time between them.
  // the note's duration + 30% seems to work well:
  int pauseBetweenNotes = noteDuration * 1.0;
  delay(pauseBetweenNotes);
  // stop the tone playing:
  noTone(MUSIC_PIN);
}

void play_note6(char n) {

  // to calculate the note duration, take one second
  // divided by the note type.
  // e.g. quarter note = 1000 / 4, eighth note = 1000/8, etc.
  int noteDuration = 1000/duration;
  
  int note = NOTE_B0;


  if (n == 'A') {
    note = NOTE_A6;
  } else if (n == 'B') {
    note = NOTE_B6;
  } else if (n == 'C') {
    note = NOTE_C6;
  } else if (n == 'D') {
    note = NOTE_D6;
  } else if (n == 'E') {
    note = NOTE_E6;
  } else if (n == 'F') {
    note = NOTE_F6;
  } else if (n == 'G') {
    note = NOTE_G6;
  } 

  tone(MUSIC_PIN, note, noteDuration);

  // to distinguish the notes, set a minimum time between them.
  // the note's duration + 30% seems to work well:
  int pauseBetweenNotes = noteDuration * 1.0;
  delay(pauseBetweenNotes);
  // stop the tone playing:
  noTone(MUSIC_PIN);
}

void play_note7(char n) {

  // to calculate the note duration, take one second
  // divided by the note type.
  // e.g. quarter note = 1000 / 4, eighth note = 1000/8, etc.
  int noteDuration = 1000/duration;
  
  int note = NOTE_B0;


  if (n == 'A') {
    note = NOTE_A7;
  } else if (n == 'B') {
    note = NOTE_B7;
  } else if (n == 'C') {
    note = NOTE_C7;
  } else if (n == 'D') {
    note = NOTE_D7;
  } else if (n == 'E') {
    note = NOTE_E7;
  } else if (n == 'F') {
    note = NOTE_F7;
  } else if (n == 'G') {
    note = NOTE_G7;
  } 

  tone(MUSIC_PIN, note, noteDuration);

  // to distinguish the notes, set a minimum time between them.
  // the note's duration + 30% seems to work well:
  int pauseBetweenNotes = noteDuration * 1.0;
  delay(pauseBetweenNotes);
  // stop the tone playing:
  noTone(MUSIC_PIN);
}

uint8_t level = 1;

int select_note(char n) {

  int offset = (level - 1) * 24;

  if (n == 'a') {
    return pgm_read_word(notes + offset);
  } else if (n == 'z') {
    return pgm_read_word(notes + offset + 1);
  } else if (n == 'e') {
    return pgm_read_word(notes + offset + 2);
  } else if (n == 'r') {
    return pgm_read_word(notes + offset + 3);
  } else if (n == 't') {
    return pgm_read_word(notes + offset + 4);
  } else if (n == 'y') {
    return pgm_read_word(notes + offset + 5);
  } else if (n == 'u') {
    return pgm_read_word(notes + offset + 6);
  } else if (n == 'i') {
    return pgm_read_word(notes + offset + 7);
  } else if (n == 'o') {
    return pgm_read_word(notes + offset + 8);
  } else if (n == 'p') {
    return pgm_read_word(notes + offset + 9);
  } else if (n == 'q') {
    return pgm_read_word(notes + offset + 10);
  } else if (n == 's') {
    return pgm_read_word(notes + offset + 11);
  } else if (n == 'd') {
    return pgm_read_word(notes + offset + 12);
  } else if (n == 'f') {
    return pgm_read_word(notes + offset + 13);
  } else if (n == 'g') {
    return pgm_read_word(notes + offset + 14);
  } else if (n == 'h') {
    return pgm_read_word(notes + offset + 15);
  } else if (n == 'j') {
    return pgm_read_word(notes + offset + 16);
  } else if (n == 'k') {
    return pgm_read_word(notes + offset + 17);
  } else if (n == 'l') {
    return pgm_read_word(notes + offset + 18);
  } else if (n == 'm') {
    return pgm_read_word(notes + offset + 19);
  } else if (n == 'w') {
    return pgm_read_word(notes + offset + 20);
  } else if (n == 'x') {
    return pgm_read_word(notes + offset + 21);
  } else if (n == 'c') {
    return pgm_read_word(notes + offset + 22);
  } else if (n == 'v') {
    return pgm_read_word(notes + offset + 23);
  }

  return 1234;
}

void play_note(char n) {

  // to calculate the note duration, take one second
  // divided by the note type.
  // e.g. quarter note = 1000 / 4, eighth note = 1000/8, etc.
  int noteDuration = 1000/duration;
  
  int note = select_note(n);
  Serial.print(" n");
  Serial.print(note);
  tone(MUSIC_PIN, note, noteDuration);

  // to distinguish the notes, set a minimum time between them.
  // the note's duration + 30% seems to work well:
  int pauseBetweenNotes = noteDuration * 1.0;
  delay(pauseBetweenNotes);
  // stop the tone playing:
  noTone(MUSIC_PIN);
}

void setup() {
  Serial.begin(115200);

  Serial.println("Setup env");
  // playtheme();
  //A fast falling tone makes a better laser sound (like the laser 'bullet' is getting further away) than a single tone does. 
    for(int i=800;i>500;i--){ //Starting at 800 and decreasing to 500
        tone(MUSIC_PIN,i); //play the tone with pitch equal to variable 'i'
        delay(1); //delay 1 millisecond before changing the pitch and playing again
    }
    noTone(MUSIC_PIN);

}

char escape_buffer[10];

void loop() {
  bool escape = false;
  memset(escape_buffer, 0, 10);
  uint8_t escpos = 0;

  while (Serial.available()) {
    char c = Serial.read();
    if (c == 27) { // escape
      escape = true;
      delayMicroseconds(700);       // delay to ensure esc sequence is available
    } else if (c > 31 && c < 127) { // only ASCII
      if (escape) {
        escape_buffer[escpos++] = c;
      //} else if (c == 'p') {
      //  play();
      } else if (c == 'P') {
        playtheme();
      } else if (c == 'L') {
        duration = 1;
      //} else if (c == 'l') {
      //  duration = 2;
      //} else if (c == 's') {
      //  duration = 4;
      } else if (c == 'S') {
        duration = 8;
      } else if ((c >= 'A' && c <= 'G')) {
        if (level == 1) {
          play_note1(c);
        } else if (level == 2) {
          play_note2(c);
        } else if (level == 3) {
          play_note3(c);
        } else if (level == 4) {
          play_note4(c);
        } else if (level == 5) {
          play_note5(c);
        } else if (level == 6) {
          play_note6(c);
        } /*else if (level == 7) {
          play_note7(c);
        }*/
        Serial.print(c);
      } else if (c >= 'a' && c <= 'z') {
        Serial.print(c);
        play_note(c);
      } else if (c >= '1' && c <= '7') {
        level = c - '0';
        Serial.print("Level = ");
        Serial.println(level);
      }
    }
  }
}

#ifdef USE_MINICORE
#include <main.cpp.h>
#endif
