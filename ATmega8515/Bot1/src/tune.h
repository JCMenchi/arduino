#ifndef TUNE_H
#define TUNE_H

#include <Arduino.h>
#include <notes.h>
/*
const uint8_t COUNT_NOTES = 39;

// notes
uint16_t frequences[COUNT_NOTES] = {
    NOTE_G4, NOTE_G4, NOTE_G4, NOTE_DS4, NOTE_AS4, NOTE_G4, NOTE_DS4, NOTE_AS4, NOTE_G4,
    NOTE_D5, NOTE_D5, NOTE_D5, NOTE_DS5, NOTE_AS4, NOTE_FS4, NOTE_DS4, NOTE_AS4, NOTE_G4,
    NOTE_G5, NOTE_G4, NOTE_G4, NOTE_G5, NOTE_FS5, NOTE_F5, NOTE_E5, NOTE_DS5, NOTE_E5,
    NOTE_GS4, NOTE_CS5, NOTE_C5, NOTE_B4, NOTE_AS4, NOTE_A4, NOTE_AS4,
    NOTE_DS4, NOTE_FS4, NOTE_DS4, NOTE_AS4, NOTE_G4};

// note duration in ms
uint16_t durations[COUNT_NOTES] = {
    350, 350, 350, 250, 100, 350, 250, 100, 700,
    350, 350, 350, 250, 100, 350, 250, 100, 700,
    350, 250, 100, 350, 250, 100, 100, 100, 450,
    150, 350, 250, 100, 100, 100, 450,
    150, 350, 250, 100, 750};

void play(uint8_t pin)
{
    for (int i = 0; i < COUNT_NOTES; i++)
    {
        tone(pin, frequences[i], durations[i] * 2);
        delay(durations[i] * 2);
        noTone(11);
    }
}
*/
#endif
