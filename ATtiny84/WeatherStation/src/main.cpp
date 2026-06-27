
#include <BME280.h>
#include <SSD1306Display.h>
#include <avr/io.h>
#include <eeprom_utils.h>
#include <gpio.h>
#include <int0_serial.h>
#include <stdlib.h>
#include <string.h>
#include <util/delay.h>

const int LED_PIN = PA0;  // PA0 leg 3 on DIP8 chip

#define HAS_DISPLAY
#ifdef HAS_DISPLAY
SSD1306Display display(128, 32);
#endif

int32_t minTemp = 100000, maxTemp = -100000;
int32_t minPressure = 1000000, maxPressure = -1000000;
uint16_t minHumidity = 64000, maxHumidity = 0;

const uint8_t FONT_CHAR_WIDTH = 6;       // 5 pixels + 1 pixel space
const uint8_t CUR_MEASURE_X_POS = 13;    // X position for current measurement display
const uint8_t DISPLAY_TEXT_OFFSET = 11;  // X offset for text display

const uint8_t TEMP_Y_POS = 8;
const uint8_t PRESSURE_Y_POS = 24;
const uint8_t HUMIDITY_Y_POS = 16;

void displayTemperature(int32_t temp) {
    char buf[12];
    ltoa(temp, buf, 10);
    uint8_t l = strlen(buf);
    // keep 1 decimal digit, add '.' and 'C' at the end
    buf[l] = 'C';
    buf[l - 1] = buf[l - 2];
    buf[l - 2] = '.';
    buf[l + 1] = '\0';
    display.drawString(CUR_MEASURE_X_POS, TEMP_Y_POS, buf);

    ltoa(minTemp, buf, 10);
    l = strlen(buf);
    // keep 1 decimal digit, add '.' and 'C' at the end
    buf[l] = 'C';
    buf[l - 1] = buf[l - 2];
    buf[l - 2] = '.';
    buf[l + 1] = '\0';
    display.drawString(CUR_MEASURE_X_POS + 5 * FONT_CHAR_WIDTH + DISPLAY_TEXT_OFFSET, TEMP_Y_POS, buf);

    ltoa(maxTemp, buf, 10);
    l = strlen(buf);
    // keep 1 decimal digit, add '.' and 'C' at the end
    buf[l] = 'C';
    buf[l - 1] = buf[l - 2];
    buf[l - 2] = '.';
    buf[l + 1] = '\0';
    display.drawString(CUR_MEASURE_X_POS + 10 * FONT_CHAR_WIDTH + 2 * DISPLAY_TEXT_OFFSET, TEMP_Y_POS, buf);
}

void displayHumidity(uint16_t hum) {
    char buf[12];

    ltoa(hum, buf, 10);
    uint8_t l = strlen(buf);
    // keep 1 decimal digit, add '.' and '%' at the end
    buf[l - 2] = '%';
    buf[l - 1] = '\0';
    display.drawString(CUR_MEASURE_X_POS, HUMIDITY_Y_POS, buf);

    ltoa(minHumidity, buf, 10);
    l = strlen(buf);
    // keep 1 decimal digit, add '.' and '%' at the end
    buf[l - 2] = '%';
    buf[l - 1] = '\0';
    display.drawString(CUR_MEASURE_X_POS + 5 * FONT_CHAR_WIDTH + DISPLAY_TEXT_OFFSET, HUMIDITY_Y_POS, buf);

    ltoa(maxHumidity, buf, 10);
    l = strlen(buf);
    // keep 1 decimal digit, add '.' and '%' at the end
    buf[l - 2] = '%';
    buf[l - 1] = '\0';
    display.drawString(CUR_MEASURE_X_POS + 10 * FONT_CHAR_WIDTH + 2 * DISPLAY_TEXT_OFFSET, HUMIDITY_Y_POS, buf);
}

void displayPressure(int32_t pres) {
    char buf[12];

    ltoa(pres, buf, 10);  // Keep one decimal place
    uint8_t l = strlen(buf);
    buf[l - 2] = 'h';
    buf[l - 1] = '\0';
    display.drawString(CUR_MEASURE_X_POS, PRESSURE_Y_POS, buf);

    ltoa(minPressure, buf, 10);
    l = strlen(buf);
    buf[l - 2] = 'h';
    buf[l - 1] = '\0';
    display.drawString(CUR_MEASURE_X_POS + 5 * FONT_CHAR_WIDTH + DISPLAY_TEXT_OFFSET, PRESSURE_Y_POS, buf);

    ltoa(maxPressure, buf, 10);
    l = strlen(buf);
    buf[l - 2] = 'h';
    buf[l - 1] = '\0';
    display.drawString(CUR_MEASURE_X_POS + 10 * FONT_CHAR_WIDTH + 2 * DISPLAY_TEXT_OFFSET, PRESSURE_Y_POS, buf);
}

void readBME280(bool update = true) {
    int32_t temp, pres;
    uint16_t hum = 0;

    BME280_read(pres, temp, hum);

    if (hum >= 10000) {
        hum = 9999;  // cap humidity at 99.99%
    }

    if (temp < minTemp && update) {
        minTemp = temp;
        eeprom_update_dword((uint32_t*)EEPROM_DATA_START + EEPROM_MIN_TEMP_OFFSET, minTemp);
    }
    if (temp > maxTemp && update) {
        maxTemp = temp;
        eeprom_update_dword((uint32_t*)EEPROM_DATA_START + EEPROM_MAX_TEMP_OFFSET, maxTemp);
    }

    if (pres < minPressure && update) {
        minPressure = pres;
        eeprom_update_dword((uint32_t*)EEPROM_DATA_START + EEPROM_MIN_PRESSURE_OFFSET, minPressure);
    }
    if (pres > maxPressure && update) {
        maxPressure = pres;
        eeprom_update_dword((uint32_t*)EEPROM_DATA_START + EEPROM_MAX_PRESSURE_OFFSET, maxPressure);
    }

    if (hum < minHumidity && update) {
        minHumidity = hum;
        eeprom_update_word((uint16_t*)EEPROM_DATA_START + EEPROM_MIN_HUMIDITY_OFFSET, minHumidity);
    }
    if (hum > maxHumidity && update) {
        maxHumidity = hum;
        eeprom_update_word((uint16_t*)EEPROM_DATA_START + EEPROM_MAX_HUMIDITY_OFFSET, maxHumidity);
    }

    if (update) {
        displayTemperature(temp);
        displayPressure(pres);
        displayHumidity(hum);
    }
}

void setup() {
    // init LED to off
    GPIO_OUTPUT(A, 0);
    GPIO_SET_LOW(A, 0);

#ifdef HAS_INT0_SERIAL
    // Init serial interface on INT0 and PA7 pin with callback to INT0_SerialCommandMgr::serialInput
    INT0_Init(PA7, INT0_SerialCommandMgr::serialInput);
    INT0_WriteString("ATtiny84\n");
#endif

    // let other peripherals start up
    _delay_ms(1000);

    while (!eeprom_is_ready()) {
    }  // ensure EEPROM is ready before reading/writing

    bool hasChanged = InitEEPROM(EEPROM_WEATHER_TYPE, EEPROM_WEATHER_SIZE);  // Initialize EEPROM for weather station data
    if (hasChanged) {
        INT0_WriteString("EEPROM formatted for weather station.\n");
        eeprom_update_dword((uint32_t*)EEPROM_DATA_START + EEPROM_MIN_TEMP_OFFSET, minTemp);
        eeprom_update_dword((uint32_t*)EEPROM_DATA_START + EEPROM_MAX_TEMP_OFFSET, maxTemp);
        eeprom_update_dword((uint32_t*)EEPROM_DATA_START + EEPROM_MIN_PRESSURE_OFFSET, minPressure);
        eeprom_update_dword((uint32_t*)EEPROM_DATA_START + EEPROM_MAX_PRESSURE_OFFSET, maxPressure);
        eeprom_update_word((uint16_t*)EEPROM_DATA_START + EEPROM_MIN_HUMIDITY_OFFSET, minHumidity);
        eeprom_update_word((uint16_t*)EEPROM_DATA_START + EEPROM_MAX_HUMIDITY_OFFSET, maxHumidity);
    } else {
        INT0_WriteString("Using existing EEPROM data.\n");
        minTemp = eeprom_read_dword((uint32_t*)EEPROM_DATA_START + EEPROM_MIN_TEMP_OFFSET);
        maxTemp = eeprom_read_dword((uint32_t*)EEPROM_DATA_START + EEPROM_MAX_TEMP_OFFSET);
        minPressure = eeprom_read_dword((uint32_t*)EEPROM_DATA_START + EEPROM_MIN_PRESSURE_OFFSET);
        maxPressure = eeprom_read_dword((uint32_t*)EEPROM_DATA_START + EEPROM_MAX_PRESSURE_OFFSET);
        minHumidity = eeprom_read_word((uint16_t*)EEPROM_DATA_START + EEPROM_MIN_HUMIDITY_OFFSET);
        maxHumidity = eeprom_read_word((uint16_t*)EEPROM_DATA_START + EEPROM_MAX_HUMIDITY_OFFSET);
    }

    // init OLED display
    display.init(0x20);
    display.invert(SSD1306_OFF);
    display.flip(SSD1306_OFF);
    display.drawScreen(0x00, false);

    bool b = BME280_begin();
    if (!b) {
        display.drawString(0, 24, "BME280 init KO.");
    } else {
        display.drawString(0, TEMP_Y_POS, "T");
        display.drawString(0, PRESSURE_Y_POS, "P");
        display.drawString(0, HUMIDITY_Y_POS, "H");

        display.drawString(CUR_MEASURE_X_POS, 0, "Cur");
        display.drawString(CUR_MEASURE_X_POS + 5 * FONT_CHAR_WIDTH + DISPLAY_TEXT_OFFSET, 0, "Min");
        display.drawString(CUR_MEASURE_X_POS + 10 * FONT_CHAR_WIDTH + 2 * DISPLAY_TEXT_OFFSET, 0, "Max");

        display.drawLine(CUR_MEASURE_X_POS, 0, CUR_MEASURE_X_POS, 32, SSD1306_BLACK_COLOR);
        display.drawLine(CUR_MEASURE_X_POS - 1 + 5 * FONT_CHAR_WIDTH + DISPLAY_TEXT_OFFSET, 0, CUR_MEASURE_X_POS - 1 + 5 * FONT_CHAR_WIDTH + DISPLAY_TEXT_OFFSET, 32, SSD1306_BLACK_COLOR);
        display.drawLine(CUR_MEASURE_X_POS - 1 + 10 * FONT_CHAR_WIDTH + 2 * DISPLAY_TEXT_OFFSET, 0, CUR_MEASURE_X_POS - 1 + 10 * FONT_CHAR_WIDTH + 2 * DISPLAY_TEXT_OFFSET, 32, SSD1306_BLACK_COLOR);
    }

    // execute some reading to stabilize the sensor and get initial values
    for (uint8_t i = 0; i < 5; ++i) {
        readBME280(false);
        _delay_ms(100);
    }
}

void loop(void) {
    // blink LED while reading BME280
    GPIO_SET_HIGH(A, 0);
    readBME280();
    _delay_ms(100);

    GPIO_SET_LOW(A, 0);
    _delay_ms(20000);

#ifdef HAS_INT0_SERIAL
    // check if we have received a command over serial (INT0) and print it if so
    if (INT0_SerialCommandMgr::hasCommand()) {
        const char* command = INT0_SerialCommandMgr::command();
        if (command) {
            INT0_WriteString("Cmd: ");
            INT0_WriteString(command);
            INT0_WriteString("\n");

            if (strcmp(command, "read") == 0) {
                // read EEPROM and print values
                uint16_t start = eeprom_read_word((uint16_t*)0);
                uint16_t marker = eeprom_read_word((uint16_t*)2);
                int32_t minTemp = eeprom_read_dword((uint32_t*)EEPROM_DATA_START + EEPROM_MIN_TEMP_OFFSET);
                int32_t maxTemp = eeprom_read_dword((uint32_t*)EEPROM_DATA_START + EEPROM_MAX_TEMP_OFFSET);
                int32_t minPressure = eeprom_read_dword((uint32_t*)EEPROM_DATA_START + EEPROM_MIN_PRESSURE_OFFSET);
                int32_t maxPressure = eeprom_read_dword((uint32_t*)EEPROM_DATA_START + EEPROM_MAX_PRESSURE_OFFSET);
                uint16_t minHumidity = eeprom_read_word((uint16_t*)EEPROM_DATA_START + EEPROM_MIN_HUMIDITY_OFFSET);
                uint16_t maxHumidity = eeprom_read_word((uint16_t*)EEPROM_DATA_START + EEPROM_MAX_HUMIDITY_OFFSET);

                INT0_WriteString("EEPROM marker: ");
                INT0_WriteUInt(start, 16);
                INT0_WriteString(" : ");
                INT0_WriteUInt(marker, 16);
                INT0_WriteString("\n");
                INT0_WriteString("Temp: ");
                INT0_WriteInt(minTemp, 10);
                INT0_WriteString(" / ");
                INT0_WriteInt(maxTemp, 10);
                INT0_WriteString("\nPressure: ");
                INT0_WriteInt(minPressure, 10);
                INT0_WriteString(" / ");
                INT0_WriteInt(maxPressure, 10);
                INT0_WriteString("\nHumidity: ");
                INT0_WriteInt(minHumidity, 10);
                INT0_WriteString(" / ");
                INT0_WriteInt(maxHumidity, 10);
                INT0_WriteString("\n");
            } else if (strcmp(command, "reset") == 0) {
                // reset EEPROM data
                eeprom_update_word((uint16_t*)2, 0);
                INT0_WriteString("EEPROM reset.\n");
                minTemp = 100000;
                maxTemp = -100000;
                minPressure = 1000000;
                maxPressure = -1000000;
                minHumidity = 64000;
                maxHumidity = 0;
            } else {
                INT0_WriteString("Unknown command.\n");
            }
        }
    }
#endif
}

#ifndef ARDUINO
#include <main.cpp.h>
#endif