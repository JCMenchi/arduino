#ifndef _SOUNDMANAGER_H
#define _SOUNDMANAGER_H

#include <stddef.h>
#include <stdint.h>

/**
 * @brief Manages sound effects and background music for the game
 * 
 * This class handles both background music loops and sound effects,
 * using a single pin for audio output through PWM
 */
struct SoundManager {
    /** 
     * @brief Initialize the sound manager
     * @param mcu_port Pointer to the MCU port register
     * @param mcu_ddr Pointer to the data direction register
     * @param pin_on_port Pin number on the selected port
     */
    static void init(volatile uint8_t* mcu_port, volatile uint8_t* mcu_ddr, uint8_t pin_on_port);

    /**
     * @brief Get the singleton instance of SoundManager
     * @return Pointer to the SoundManager instance
     */
    static SoundManager* instance() {
        return _instance;
    }

    /**
     * @brief Available sound effects in the game
     */
    enum Effect {
        NO_EFFECT = 0,         ///< No sound effect playing
        FIRE_EFFECT = 1,       ///< Player shooting sound
        HIT_SHELTER_EFFECT = 2, ///< Projectile hitting shelter
        HIT_ALIEN_EFFECT = 3,   ///< Projectile hitting alien
        EXPLOSION_EFFECT = 4    ///< Player explosion sound
    };

    /**
     * @brief Configure the background music parameters
     * @param sound_loop Array of frequency values for the music
     * @param size Number of notes in the sound loop
     * @param note_duration Duration of each note in milliseconds
     * @param note_pause Pause between notes in milliseconds
     */
    void setMusic(const uint16_t* sound_loop, size_t size, uint16_t note_duration, uint16_t note_pause) {
        // Set the music to play
        _music_notes = sound_loop;
        _music_size = size;
        _note_duration = note_duration;
        _note_pause = note_pause;
    }

    void start_sound();    ///< Start sound output
    void start_music();    ///< Start playing background music
    void stop_music();     ///< Stop playing background music
    bool is_music_playing() const { return _music_playing; } ///< Check if music is currently playing
    bool is_effect_enabled() const { return _effect_enabled; } ///< Check if sound effects
    
    /**
     * @brief Play a sound effect
     * @param effect_id The effect to play from the Effect enum
     */
    void sound_effect(SoundManager::Effect effect_id);

    /**
     * @brief Update sound state - should be called in the main loop
     * @param now Current time in milliseconds
     */
    void update(uint32_t now);

    void enable_effect() { _effect_enabled = true; }   ///< Enable sound output
    void disable_effect() { _effect_enabled = false; } ///< Disable sound output

   private:
    /** 
     * @brief Private constructor for singleton pattern
     */
    SoundManager() : _current_music_note(-1), _music_size(0), _music_notes(nullptr), 
                    _note_duration(0), _note_pause(0), _last_note_time(0), 
                    _current_effect_id(NO_EFFECT), _current_effect_note(-1), 
                    _effect_size(0), _effect_notes(NULL), _effect_enabled(false) {}

    int8_t _current_music_note;      ///< Current note index in the music loop
    int8_t _saved_music_note;        ///< Saved music note position when effect interrupts
    uint8_t _music_size;             ///< Total number of notes in music loop
    const uint16_t* _music_notes;     ///< Array of frequency values for music

    uint16_t _note_duration;         ///< Duration of each note
    uint16_t _note_pause;            ///< Pause duration between notes
    uint32_t _last_note_time;        ///< Timestamp of last note played

    SoundManager::Effect _current_effect_id;  ///< Currently playing effect
    int8_t _current_effect_note;              ///< Current note index in effect
    uint8_t _effect_size;                     ///< Total notes in current effect
    const uint16_t* _effect_notes;            ///< Array of effect frequencies
    const uint16_t* _effect_note_durations;   ///< Array of effect note durations

    volatile uint8_t* _mcu_port;     ///< MCU port register for audio output
    volatile uint8_t* _mcu_ddr;      ///< Data direction register for audio pin
    uint8_t _pin_on_port;            ///< Pin number used for audio output

    bool _effect_enabled;                   ///< Flag to enable or disable sound output
    bool _music_playing;                    ///< Flag to indicate if music is currently playing
    static SoundManager* _instance;   ///< Singleton instance pointer
};

#endif