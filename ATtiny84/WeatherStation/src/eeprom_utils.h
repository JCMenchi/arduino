#include <avr/eeprom.h>

// define EEPROM layout
const uint16_t EEPROM_INVADERS_TYPE=0xDECA;
const uint16_t EEPROM_WEATHER_TYPE=0xCAD0;

const uint16_t EEPROM_WEATHER_SIZE=2*2+2;  // 2 int16_t + 2 int8_t

const uint16_t EEPROM_DATA_START = 4;  // start of EEPROM data, 2 bytes after the type marker

const uint16_t EEPROM_MIN_TEMP_OFFSET = 0;
const uint16_t EEPROM_MAX_TEMP_OFFSET = 2;
const uint16_t EEPROM_MIN_HUMIDITY_OFFSET = 5;
const uint16_t EEPROM_MAX_HUMIDITY_OFFSET = 6;

bool InitEEPROM(uint16_t eptype, size_t epsize) {
    uint16_t marker = eeprom_read_word((uint16_t *)2);
    if (marker != eptype) {
        // format EEPROM with the given type and size
        eeprom_write_word((uint16_t *)2, eptype);
        for (uint8_t i = 0; i < epsize; ++i) {
            eeprom_update_byte((uint8_t *)(2 + i), 0);
        }
    }

    eeprom_update_word((uint16_t *)2, eptype);
    return marker != eptype;
}
