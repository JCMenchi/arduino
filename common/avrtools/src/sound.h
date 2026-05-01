/**
 * @file sound.h
 * @brief Musical note frequency definitions and sound generation utilities
 * 
 * Defines constants for musical note frequencies from B0 (31 Hz) to C8 (4186 Hz).
 * These frequency values are suitable for use with PWM-based sound generation or
 * timer-based tone generation.
 * 
 * Frequency values follow the equal temperament tuning scale.
 * 
 * @note Frequencies are in Hz
 * @note Use with appropriate PWM or timer-based tone generation
 */

#ifndef _SOUND_H
#define _SOUND_H

#include <stdint.h>
#include <avr/io.h>
#include <util/delay.h>

/** @brief Musical note B0 (31 Hz) */
#define NOTE_B0  31
/** @brief Musical note C1 (33 Hz) */
#define NOTE_C1  33
/** @brief Musical note C#1 (35 Hz) */
#define NOTE_CS1 35
/** @brief Musical note D1 (37 Hz) */
#define NOTE_D1  37
/** @brief Musical note D#1 (39 Hz) */
#define NOTE_DS1 39
/** @brief Musical note E1 (41 Hz) */
#define NOTE_E1  41
/** @brief Musical note F1 (44 Hz) */
#define NOTE_F1  44
/** @brief Musical note F#1 (46 Hz) */
#define NOTE_FS1 46
/** @brief Musical note G1 (49 Hz) */
#define NOTE_G1  49
/** @brief Musical note G#1 (52 Hz) */
#define NOTE_GS1 52
/** @brief Musical note A1 (55 Hz) */
#define NOTE_A1  55
/** @brief Musical note A#1 (58 Hz) */
#define NOTE_AS1 58
/** @brief Musical note B1 (62 Hz) */
#define NOTE_B1  62

/** @brief Musical note C2 (65 Hz) */
#define NOTE_C2  65
/** @brief Musical note C#2 (69 Hz) */
#define NOTE_CS2 69
/** @brief Musical note D2 (73 Hz) */
#define NOTE_D2  73
/** @brief Musical note D#2 (78 Hz) */
#define NOTE_DS2 78
/** @brief Musical note E2 (82 Hz) */
#define NOTE_E2  82
/** @brief Musical note F2 (87 Hz) */
#define NOTE_F2  87
/** @brief Musical note F#2 (93 Hz) */
#define NOTE_FS2 93
/** @brief Musical note G2 (98 Hz) */
#define NOTE_G2  98
/** @brief Musical note G#2 (104 Hz) */
#define NOTE_GS2 104
/** @brief Musical note A2 (110 Hz) */
#define NOTE_A2  110
/** @brief Musical note A#2 (117 Hz) */
#define NOTE_AS2 117
/** @brief Musical note B2 (123 Hz) */
#define NOTE_B2  123

/** @brief Musical note C3 (131 Hz) */
#define NOTE_C3  131
/** @brief Musical note C#3 (139 Hz) */
#define NOTE_CS3 139
/** @brief Musical note D3 (147 Hz) */
#define NOTE_D3  147
/** @brief Musical note D#3 (156 Hz) */
#define NOTE_DS3 156
/** @brief Musical note E3 (165 Hz) */
#define NOTE_E3  165
/** @brief Musical note F3 (175 Hz) */
#define NOTE_F3  175
/** @brief Musical note F#3 (185 Hz) */
#define NOTE_FS3 185
/** @brief Musical note G3 (196 Hz) */
#define NOTE_G3  196
/** @brief Musical note G#3 (208 Hz) */
#define NOTE_GS3 208
/** @brief Musical note A3 (220 Hz) */
#define NOTE_A3  220
/** @brief Musical note A#3 (233 Hz) */
#define NOTE_AS3 233
/** @brief Musical note B3 (247 Hz) */
#define NOTE_B3  247

/** @brief Musical note C4 - Middle C (262 Hz) */
#define NOTE_C4  262
/** @brief Musical note C#4 (277 Hz) */
#define NOTE_CS4 277
/** @brief Musical note D4 (294 Hz) */
#define NOTE_D4  294
/** @brief Musical note D#4 (311 Hz) */
#define NOTE_DS4 311
/** @brief Musical note E4 (330 Hz) */
#define NOTE_E4  330
/** @brief Musical note F4 (349 Hz) */
#define NOTE_F4  349
/** @brief Musical note F#4 (370 Hz) */
#define NOTE_FS4 370
/** @brief Musical note G4 (392 Hz) */
#define NOTE_G4  392
/** @brief Musical note G#4 (415 Hz) */
#define NOTE_GS4 415
/** @brief Musical note A4 - Concert A (440 Hz) */
#define NOTE_A4  440
/** @brief Musical note A#4 (466 Hz) */
#define NOTE_AS4 466
/** @brief Musical note B4 (494 Hz) */
#define NOTE_B4  494

/** @brief Musical note C5 (523 Hz) */
#define NOTE_C5  523
/** @brief Musical note C#5 (554 Hz) */
#define NOTE_CS5 554
/** @brief Musical note D5 (587 Hz) */
#define NOTE_D5  587
/** @brief Musical note D#5 (622 Hz) */
#define NOTE_DS5 622
/** @brief Musical note E5 (659 Hz) */
#define NOTE_E5  659
/** @brief Musical note F5 (698 Hz) */
#define NOTE_F5  698
/** @brief Musical note F#5 (740 Hz) */
#define NOTE_FS5 740
/** @brief Musical note G5 (784 Hz) */
#define NOTE_G5  784
/** @brief Musical note G#5 (831 Hz) */
#define NOTE_GS5 831
/** @brief Musical note A5 (880 Hz) */
#define NOTE_A5  880
/** @brief Musical note A#5 (932 Hz) */
#define NOTE_AS5 932
/** @brief Musical note B5 (988 Hz) */
#define NOTE_B5  988

/** @brief Musical note C6 (1047 Hz) */
#define NOTE_C6  1047
/** @brief Musical note C#6 (1109 Hz) */
#define NOTE_CS6 1109
/** @brief Musical note D6 (1175 Hz) */
#define NOTE_D6  1175
/** @brief Musical note D#6 (1245 Hz) */
#define NOTE_DS6 1245
/** @brief Musical note E6 (1319 Hz) */
#define NOTE_E6  1319
/** @brief Musical note F6 (1397 Hz) */
#define NOTE_F6  1397
/** @brief Musical note F#6 (1480 Hz) */
#define NOTE_FS6 1480
/** @brief Musical note G6 (1568 Hz) */
#define NOTE_G6  1568
/** @brief Musical note G#6 (1661 Hz) */
#define NOTE_GS6 1661
/** @brief Musical note A6 (1760 Hz) */
#define NOTE_A6  1760
/** @brief Musical note A#6 (1865 Hz) */
#define NOTE_AS6 1865
/** @brief Musical note B6 (1976 Hz) */
#define NOTE_B6  1976

/** @brief Musical note C7 (2093 Hz) */
#define NOTE_C7  2093
/** @brief Musical note C#7 (2217 Hz) */
#define NOTE_CS7 2217
/** @brief Musical note D7 (2349 Hz) */
#define NOTE_D7  2349
/** @brief Musical note D#7 (2489 Hz) */
#define NOTE_DS7 2489
/** @brief Musical note E7 (2637 Hz) */
#define NOTE_E7  2637
/** @brief Musical note F7 (2794 Hz) */
#define NOTE_F7  2794
/** @brief Musical note F#7 (2960 Hz) */
#define NOTE_FS7 2960
/** @brief Musical note G7 (3136 Hz) */
#define NOTE_G7  3136
/** @brief Musical note G#7 (3322 Hz) */
#define NOTE_GS7 3322
/** @brief Musical note A7 (3520 Hz) */
#define NOTE_A7  3520
/** @brief Musical note A#7 (3729 Hz) */
#define NOTE_AS7 3729
/** @brief Musical note B7 (3951 Hz) */
#define NOTE_B7  3951

/** @brief Musical note C8 (4186 Hz) */
#define NOTE_C8  4186
#define NOTE_CS8 4435
#define NOTE_D8  4699
#define NOTE_DS8 4978

const uint8_t SOUND_LOOP_SIZE = 4;
const uint8_t sound_loop[] = { NOTE_AS2, NOTE_GS2, NOTE_FS2, NOTE_F2 };
const uint16_t SOUND_LOOP_NOTE_DURATION = 800;
const uint16_t SOUND_LOOP_NOTE_PAUSE = 200;

#ifndef MUSIC_PORT
#define MUSIC_PORT PORTB
#endif

#ifndef MUSIC_DDR
#define MUSIC_DDR DDRB
#endif

#ifndef MUSIC_PIN
#define MUSIC_PIN PORTB0
#endif

void start_sound();

void playNote(volatile uint8_t* mcu_port, volatile uint8_t* mcu_ddr, uint8_t pin_on_port, unsigned int frequency, unsigned long duration);
void stopNote();

inline void start_sound() {
    playNote(&MUSIC_PORT, &MUSIC_DDR, MUSIC_PIN, NOTE_B5, 100);
    _delay_ms(100);
    playNote(&MUSIC_PORT, &MUSIC_DDR, MUSIC_PIN, NOTE_E6, 850);
    _delay_ms(800);
    stopNote();
}

#endif