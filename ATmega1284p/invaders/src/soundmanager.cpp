#include <millisec.h>
#include <sound.h>
#include <soundmanager.h>
#include <stdlib.h>

#ifdef HAS_SERIAL
#include <usart_serial.h>
#endif

// Singleton instance pointer initialization
SoundManager* SoundManager::_instance = NULL;

/**
 * @brief Initialize the sound manager singleton instance
 * 
 * Creates the singleton instance if it doesn't exist and configures
 * the hardware pins for sound output
 */
void SoundManager::init(volatile uint8_t* mcu_port, volatile uint8_t* mcu_ddr, uint8_t pin_on_port) {
    if (SoundManager::_instance == NULL) {
        SoundManager::_instance = (SoundManager*)malloc(sizeof(SoundManager));

        // Initialize the sound manager with MCU port, DDR, and pin
        SoundManager::_instance->_mcu_port = mcu_port;
        SoundManager::_instance->_mcu_ddr = mcu_ddr;
        SoundManager::_instance->_pin_on_port = pin_on_port;
        SoundManager::_instance->_effect_enabled = false; // Initialize to disabled
        SoundManager::_instance->_music_playing = false; // Initialize music to not playing
    }
}

void SoundManager::start_music() {
    _music_playing = true;
    _last_note_time = 0;
    _current_music_note = 0; // Reset music note index
}

void SoundManager::stop_music() {
    _music_playing = false;
    stopNote();
    _last_note_time = 0;
    _current_music_note = 0; // Reset music note index
}

/**
 * @brief Update sound system state
 * 
 * Handles both sound effects and background music playback:
 * 1. If a sound effect is playing, handles its note transitions
 * 2. Otherwise plays background music if active
 * Each note has a duration and pause between notes
 */
void SoundManager::update(uint32_t now) {

    // Handle sound effect playback
    if (_current_effect_id > 0 && _effect_notes && _effect_size > 0) {
        if (now - _last_note_time >= _effect_note_durations[_current_effect_note] + 1) {
            stopNote();
            _last_note_time = now;
            if (_current_effect_note >= _effect_size - 1) {
                // Effect finished - restore background music state
                _current_music_note = _saved_music_note;
                _current_effect_id = NO_EFFECT;
                _current_effect_note = -1;
                _effect_notes = NULL;
                _effect_note_durations = NULL;
            } else {
                // Play next note in effect sequence
                _current_effect_note = (_current_effect_note + 1);
                playNote(_mcu_port, _mcu_ddr, _pin_on_port, 
                        _effect_notes[_current_effect_note], 
                        _effect_note_durations[_current_effect_note]);
            }
        }
    } 
    // Handle background music playback when no effect is playing
    else if (_music_playing && _current_music_note >= 0 && _music_notes && _music_size > 0) {
        if (now - _last_note_time >= _note_duration + _note_pause) {
            stopNote();
            _last_note_time = now;
            // Loop through music notes
            _current_music_note = (_current_music_note + 1) % _music_size;
            playNote(_mcu_port, _mcu_ddr, _pin_on_port, 
                    _music_notes[_current_music_note], _note_duration);
        }
    }
}

/**
 * @brief Play startup sound sequence
 * 
 * Plays a simple two-note melody (B5 followed by E6)
 * Used when the game starts
 */
void SoundManager::start_sound() {
    stopNote();
    playNote(_mcu_port, _mcu_ddr, _pin_on_port, NOTE_B5, 100);
    _delay_ms(100);
    playNote(_mcu_port, _mcu_ddr, _pin_on_port, NOTE_E6, 850);
    _delay_ms(800);
    stopNote();
}

// Define sound effect sequences
// Each effect has an array of frequencies and corresponding durations

// Firing sound effect - short ascending sequence
const uint16_t fire_sound[] = {NOTE_C3, NOTE_GS1, NOTE_GS2};
const uint16_t fire_sound_duration[] = {200, 100, 200};
uint8_t fire_sound_size = 1;

// G3:0.05 F#3:0.05 F3:0.05 E3:0.05 D#3:0.05 D3:0.05 C#3:0.05 

// Shelter hit sound - single low note
const uint16_t hit_shelter_sound[] = {NOTE_D1};
const uint16_t hit_shelter_sound_duration[] = {200};
uint8_t hit_shelter_sound_size = 1;

// Explosion sound - descending sequence
const uint16_t explosion_sound[] = {NOTE_D1, NOTE_C2, NOTE_F2};
const uint16_t explosion_sound_duration[] = {100, 100, 100};
uint8_t explosion_sound_size = 3;

// C5:0.01 C4:0.01 C3:0.01 C2:0.01 G2:0.02 F2:0.02 E2:0.02 D2:0.02 C2:0.1 C1:0.2 
// or
// C2:0.01 G4:0.01 C3:0.01 E5:0.01 D2:0.01 A4:0.01 G3:0.05 C2:0.05 F4:0.05 B2:0.05 C2:0.1

// or
// C2:0.01 G4:0.01 C3:0.01 E5:0.01 D2:0.01 A4:0.01 G3:0.05 C2:0.05 F4:0.05 B2:0.05 C2:0.1 G1:0.15 F1:0.2 C1:0.3

/**
 * @brief Play a sound effect
 * 
 * Interrupts current background music (if playing) to play the requested effect.
 * Only plays the new effect if it has higher priority than current effect.
 * Restores background music after effect completes.
 */
void SoundManager::sound_effect(SoundManager::Effect effect_id) {
    if (_effect_enabled == false) {
        return; // Sound is disabled, do nothing
    }

    // Only play new effect if it has higher priority
    if (effect_id > _current_effect_id) {
        _current_effect_id = effect_id;
        
        // Select appropriate sound effect sequence
        switch (_current_effect_id) {
            case FIRE_EFFECT:
                _effect_notes = fire_sound;
                _effect_note_durations = fire_sound_duration;
                _effect_size = fire_sound_size;
                break;
            case HIT_SHELTER_EFFECT:
                _effect_notes = hit_shelter_sound;
                _effect_note_durations = hit_shelter_sound_duration;
                _effect_size = hit_shelter_sound_size;
                break;
            case HIT_ALIEN_EFFECT:
            case EXPLOSION_EFFECT:
                _effect_notes = explosion_sound;
                _effect_note_durations = explosion_sound_duration;
                _effect_size = explosion_sound_size;
                break;
            default:
                // Unknown effect ID, do nothing
                _current_effect_id = NO_EFFECT;  // Reset current effect ID
                _current_effect_note = -1; // Reset current effect note
                _effect_size = 0;
                _effect_notes = NULL;
                _effect_note_durations = NULL;
                return;
        }

        // Save music state and start effect
        stopNote();
        _last_note_time = milliseconds();
        _current_effect_note = 0;
        if (_current_music_note >= 0) {
            _saved_music_note = _current_music_note;  // Save current music note to play effect
            _current_music_note = -1;
        }
        playNote(_mcu_port, _mcu_ddr, _pin_on_port, 
                _effect_notes[_current_effect_note], 
                _effect_note_durations[_current_effect_note]);
    }
}