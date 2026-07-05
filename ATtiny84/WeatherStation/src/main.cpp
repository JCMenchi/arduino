
#include <BME280.h>
#include <SSD1306Display.h>
#include <avr/io.h>
#include <eeprom_utils.h>
#include <gpio.h>
#include <int0_serial.h>
#include <millisec.h>
#include <nrf24mgr.h>
#include <stdlib.h>
#include <string.h>
#include <tinyspi.h>
#include <util/delay.h>

// Main application for the ATtiny84 weather station.
// Reads environmental data from a BME280 sensor, updates an OLED display,
// stores min/max history in EEPROM, and broadcasts readings over nRF24L01.

const int LED_PIN = PA0;  // PA0 leg 3 on DIP8 chip, used as status indicator

// Create global instances of peripheral managers used throughout the app.
SPIManager spimgr;

const int8_t PL_SIZE = NRF24_DYNAMIC_PAYLOAD_SIZE;
NRF24Manager radio(PL_SIZE);

#define HAS_DISPLAY
#ifdef HAS_DISPLAY
SSD1306Display display(128, 32);
#endif

int16_t curTemp = 0, minTemp = 30000, maxTemp = -10000;
int8_t curHumidity = 100, minHumidity = 100, maxHumidity = 0;
int16_t curPressure = 0;

const uint8_t FONT_CHAR_WIDTH = 6;       // 5 pixels + 1 pixel space
const uint8_t CUR_MEASURE_X_POS = 13;    // X position for current measurement display
const uint8_t DISPLAY_TEXT_OFFSET = 11;  // X offset for text display

const uint8_t TEMP_Y_POS = 8;
const uint8_t PRESSURE_Y_POS = 24;
const uint8_t HUMIDITY_Y_POS = 16;

const uint16_t MENU_X_POSITION = 75;
const uint16_t MENU_Y_POSITION = 24;
const uint8_t MENU_BUFFER_LENGTH = 8;

bool menuVisible = false;
char buf[8];

// Render the current temperature and min/max temperature values on the OLED.
// The temperature is stored in tenths of a degree from the BME280 sensor.
void displayTemperature(int16_t temp) {
    curTemp = temp / 10;  // Convert to degrees Celsius with one decimal place

    if (curTemp > 800) {
        // false reading, ignore
        return;
    }

    if (temp < minTemp) {
        minTemp = temp;
        eeprom_update_word((uint16_t*)EEPROM_DATA_START + EEPROM_MIN_TEMP_OFFSET, minTemp);
    }
    if (temp > maxTemp) {
        maxTemp = temp;
        eeprom_update_word((uint16_t*)EEPROM_DATA_START + EEPROM_MAX_TEMP_OFFSET, maxTemp);
    }

    itoa(temp, buf, 10);
    uint8_t l = strlen(buf);
    // keep 1 decimal digit, add '.' and 'C' at the end
    buf[l] = 'C';
    buf[l - 1] = buf[l - 2];
    buf[l - 2] = '.';
    buf[l + 1] = '\0';
    display.drawString(CUR_MEASURE_X_POS, TEMP_Y_POS, buf);

    itoa(minTemp, buf, 10);
    l = strlen(buf);
    // keep 1 decimal digit, add '.' and 'C' at the end
    buf[l] = 'C';
    buf[l - 1] = buf[l - 2];
    buf[l - 2] = '.';
    buf[l + 1] = '\0';
    display.drawString(CUR_MEASURE_X_POS + 5 * FONT_CHAR_WIDTH + DISPLAY_TEXT_OFFSET, TEMP_Y_POS, buf);
    //
    itoa(maxTemp, buf, 10);
    l = strlen(buf);
    // keep 1 decimal digit, add '.' and 'C' at the end
    buf[l] = 'C';
    buf[l - 1] = buf[l - 2];
    buf[l - 2] = '.';
    buf[l + 1] = '\0';
    display.drawString(CUR_MEASURE_X_POS + 10 * FONT_CHAR_WIDTH + 2 * DISPLAY_TEXT_OFFSET, TEMP_Y_POS, buf);
}

// Render the current humidity and min/max humidity values on the OLED.
// Humidity is converted from the BME280 raw value into a percent integer.
void displayHumidity(uint16_t hum) {
    curHumidity = hum / 100;  // Convert to percentage without decimal place

    if (curHumidity > 90.0 || curHumidity < 1.0) {   
        // false reading, ignore
        return;
    }

    if (curHumidity < minHumidity) {
        minHumidity = curHumidity;
        eeprom_update_byte((uint8_t*)EEPROM_DATA_START + EEPROM_MIN_HUMIDITY_OFFSET, minHumidity);
    }
    if (curHumidity > maxHumidity) {
        maxHumidity = curHumidity;
        eeprom_update_byte((uint8_t*)EEPROM_DATA_START + EEPROM_MAX_HUMIDITY_OFFSET, maxHumidity);
    }

    itoa(curHumidity, buf, 10);
    uint8_t l = strlen(buf);
    buf[l] = '%';
    buf[l + 1] = '\0';
    display.drawString(CUR_MEASURE_X_POS, HUMIDITY_Y_POS, buf);

    itoa(minHumidity, buf, 10);
    l = strlen(buf);
    // keep 1 decimal digit, add '.' and '%' at the end
    buf[l] = '%';
    buf[l + 1] = '\0';
    display.drawString(CUR_MEASURE_X_POS + 5 * FONT_CHAR_WIDTH + DISPLAY_TEXT_OFFSET, HUMIDITY_Y_POS, buf);

    itoa(maxHumidity, buf, 10);
    l = strlen(buf);
    // keep 1 decimal digit, add '.' and '%' at the end
    buf[l] = '%';
    buf[l + 1] = '\0';
    display.drawString(CUR_MEASURE_X_POS + 10 * FONT_CHAR_WIDTH + 2 * DISPLAY_TEXT_OFFSET, HUMIDITY_Y_POS, buf);
}

// Render the current pressure reading on the OLED.
// Pressure from the sensor is provided in Pa and converted to hPa.
void displayPressure(int32_t pres) {
    curPressure = pres / 100.0f;
    itoa(curPressure, buf, 10);
    uint8_t l = strlen(buf);
    buf[l] = 'h';
    buf[l + 1] = '\0';
    display.drawString(CUR_MEASURE_X_POS, PRESSURE_Y_POS, buf);
}

// Read the BME280 sensor and optionally update the OLED display.
// When update is false, the sensor is read but display output is suppressed.
void readBME280(bool update = true) {
    int32_t temp, pres;
    uint16_t hum = 0;

    BME280_read(pres, temp, hum);

    if (update) {
        displayTemperature(temp);
        displayPressure(pres);
        displayHumidity(hum);
    }
}

const uint8_t JOYSTICK_VRY = 7;
const uint8_t JOYSTICK_SW = 2;


// Initialize all hardware and restore state from EEPROM.
// This runs once when the microcontroller starts.
void setup() {
    // init LED to off
    GPIO_OUTPUT(A, 0);
    GPIO_SET_LOW(A, 0);

    // configure joystick pins as input
    GPIO_INPUT(A, JOYSTICK_VRY);  // VRY
    GPIO_INPUT_PULLUP(B, JOYSTICK_SW);  // SW

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
#ifdef HAS_INT0_SERIAL
        INT0_WriteString("EEPROM formatted for weather station.\n");
#endif
        eeprom_update_word((uint16_t*)EEPROM_DATA_START + EEPROM_MIN_TEMP_OFFSET, minTemp);
        eeprom_update_word((uint16_t*)EEPROM_DATA_START + EEPROM_MAX_TEMP_OFFSET, maxTemp);
        eeprom_update_byte((uint8_t*)EEPROM_DATA_START + EEPROM_MIN_HUMIDITY_OFFSET, minHumidity);
        eeprom_update_byte((uint8_t*)EEPROM_DATA_START + EEPROM_MAX_HUMIDITY_OFFSET, maxHumidity);
    } else {
#ifdef HAS_INT0_SERIAL
        INT0_WriteString("Using existing EEPROM data.\n");
#endif
        minTemp = eeprom_read_word((uint16_t*)EEPROM_DATA_START + EEPROM_MIN_TEMP_OFFSET);
        maxTemp = eeprom_read_word((uint16_t*)EEPROM_DATA_START + EEPROM_MAX_TEMP_OFFSET);
        minHumidity = eeprom_read_byte((uint8_t*)EEPROM_DATA_START + EEPROM_MIN_HUMIDITY_OFFSET);
        maxHumidity = eeprom_read_byte((uint8_t*)EEPROM_DATA_START + EEPROM_MAX_HUMIDITY_OFFSET);
    }

    // init NRF24
    _delay_ms(1500);  // give some time to NRF24 module to start
    // init SPI bus to control NRF24
    spimgr.startMaster();

    GPIO_OUTPUT(A, 2);  // PA2 is CSN for radio, set to output
    radio.setDestinationAddress("amg32");
    radio.setMyAddress("atn84");
    radio.init(&spimgr, PA1, PA2);  // Initialize NRF24 radio with SPI manager and CE pin

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
        //
        // display.drawLine(CUR_MEASURE_X_POS, 0, CUR_MEASURE_X_POS, 32, SSD1306_BLACK_COLOR);
        // display.drawLine(CUR_MEASURE_X_POS - 1 + 5 * FONT_CHAR_WIDTH + DISPLAY_TEXT_OFFSET, 0, CUR_MEASURE_X_POS - 1 + 5 * FONT_CHAR_WIDTH + DISPLAY_TEXT_OFFSET, 32, SSD1306_BLACK_COLOR);
        // display.drawLine(CUR_MEASURE_X_POS - 1 + 10 * FONT_CHAR_WIDTH + 2 * DISPLAY_TEXT_OFFSET, 0, CUR_MEASURE_X_POS - 1 + 10 * FONT_CHAR_WIDTH + 2 * DISPLAY_TEXT_OFFSET, 32, SSD1306_BLACK_COLOR);
    }

    // execute some reading to stabilize the sensor and get initial values
    for (uint8_t i = 0; i < 5; ++i) {
        readBME280(false);
        _delay_ms(100);
    }
}

// Message format used to send sensor data over the nRF24L01 radio.
// The payload is kept compact to fit dynamic payload sizes.
struct RadioMessage {
    char protocol;     // Protocol identifier, e.g., 'W' for weather station
    char version;      // Protocol version, e.g., '1'
    char type;         // Message type, e.g., 'T' for temperature
    uint32_t timecode; // time
    int16_t temp;      // Temperature value in tenths of degrees Celsius
    int8_t humidity;   // Humidity value in percentage
    int16_t pressure;  // Pressure value in hPa (hectopascals)
};

// Send the latest temperature and humidity values over the nRF24L01 link.
void sendRadioMessage() {
    GPIO_SET_HIGH(A, 0);
    RadioMessage msg;
    msg.protocol = 'W';
    msg.version = '1';
    msg.type = 'T';
    msg.temp = curTemp;
    msg.humidity = curHumidity;
    msg.pressure = curPressure;
    msg.timecode = milliseconds();
    uint8_t length = sizeof(RadioMessage);
    radio.send_binary((uint8_t*)(&msg), length);
    _delay_ms(100);
    GPIO_SET_LOW(A, 0);
}


enum JoystickDirection {
    JOY_MIDDLE = 0,
    JOY_UP = 1,
    JOY_DOWN = 2,
    
};

enum JoystickButtonState {
    JOY_BUTTON_RELEASED = 1, // due to pullup, released state is HIGH (1)
    JOY_BUTTON_PRESSED = 0
};

JoystickButtonState prevButtonState = JOY_BUTTON_RELEASED;
JoystickDirection prevJoystickY = JOY_MIDDLE;

uint8_t menuPosition = 0;  // Current position in the menu
const uint8_t MENU_ITEM_COUNT = 4;  // Number of items in the menu
const char* menuItems[MENU_ITEM_COUNT] = {
    "Menu    ",
    "Reset   ",
    "Send    ",
    "Exit    "
};

void menuCallback() {
    // Handle menu actions based on the current menu position
    switch (menuPosition) {
        case 0:  // "Menu" selected
            // No action needed, just display the menu
            if (!menuVisible) {
                menuVisible = true;
                display.drawString(MENU_X_POSITION, MENU_Y_POSITION, " Menu   ", true);
            }
            break;
        case 1:  // "Reset" selected
            minTemp = 30000;
            maxTemp = -10000;
            minHumidity = 100;
            maxHumidity = 0;
            eeprom_update_word((uint16_t*)EEPROM_DATA_START + EEPROM_MIN_TEMP_OFFSET, minTemp);
            eeprom_update_word((uint16_t*)EEPROM_DATA_START + EEPROM_MAX_TEMP_OFFSET, maxTemp);
            eeprom_update_byte((uint8_t*)EEPROM_DATA_START + EEPROM_MIN_HUMIDITY_OFFSET, minHumidity);
            eeprom_update_byte((uint8_t*)EEPROM_DATA_START + EEPROM_MAX_HUMIDITY_OFFSET, maxHumidity);
            readBME280();
            break;
        case 2:  // "Send" selected
            sendRadioMessage();
            break;
        case 3:  // "Exit" selected
            menuVisible = false;
            menuPosition = 0;
            display.drawString(MENU_X_POSITION, MENU_Y_POSITION, "         ");  // Clear menu area on display
            break;
        default:
            break;
    }
}

void readJoystick() {
    

    // check if menu needs to be displayed - button state shall be different from previous state to avoid multiple toggles
    JoystickButtonState sw = (JoystickButtonState) GPIO_READ(B, JOYSTICK_SW);
    if (sw != prevButtonState) {
        _delay_ms(100); // debounce delay
        sw = (JoystickButtonState) GPIO_READ(B, JOYSTICK_SW);
        if (sw == JOY_BUTTON_PRESSED) {
            menuCallback();
        }
    }

    // read joystick analog values
    uint16_t yvalue = readADC(JOYSTICK_VRY);
    JoystickDirection yDirection = JOY_MIDDLE;
    if (yvalue < 200) {
        yDirection = JOY_UP;
    } else if (yvalue > 800) {
        yDirection = JOY_DOWN;
    }

    if (yDirection != prevJoystickY) {
        prevJoystickY = yDirection;
        _delay_ms(50); // debounce delay
        if (menuVisible) {
            // Update menu based on joystick direction
            if (yDirection == JOY_UP) {
                // Scroll up in menu
                menuPosition = (menuPosition - 1 + MENU_ITEM_COUNT) % MENU_ITEM_COUNT;
            } else if (yDirection == JOY_DOWN) {
                // Scroll down in menu
                menuPosition = (menuPosition + 1) % MENU_ITEM_COUNT;
            }
            display.drawString(MENU_X_POSITION, MENU_Y_POSITION, menuItems[menuPosition], true);
        }
    }
}


const uint32_t MEASURE_INTERVAL_MS = 10000;  // Measure every 5 seconds
uint32_t lastMeasureTime = 0;  // Initialize to 0 to trigger immediate measurement on startup

// Main application loop. Periodically polls the BME280 sensor,
// updates the display, blinks the status LED, and sends a radio packet.
void loop(void) {
    uint32_t currentMillis = milliseconds();

    if (currentMillis - lastMeasureTime >= MEASURE_INTERVAL_MS || lastMeasureTime == 0) {
        lastMeasureTime = currentMillis;
        readBME280();
        sendRadioMessage();
    }

    readJoystick();
   

#ifdef HAS_INT0_SERIAL
    // check if we have received a command over serial (INT0) and print it if so
    if (INT0_SerialCommandMgr::hasCommand()) {
        const char* command = INT0_SerialCommandMgr::command();
        if (command) {
            if (strcmp(command, "read") == 0) {
                // read EEPROM and print values
                // uint16_t start = eeprom_read_word((uint16_t*)0);
                //uint16_t marker = eeprom_read_word((uint16_t*)2);
                //int16_t minTemp = eeprom_read_word((uint16_t*)EEPROM_DATA_START + EEPROM_MIN_TEMP_OFFSET);
                //int16_t maxTemp = eeprom_read_word((uint16_t*)EEPROM_DATA_START + EEPROM_MAX_TEMP_OFFSET);
                // uint8_t minHumidity = eeprom_read_byte((uint8_t*)EEPROM_DATA_START + EEPROM_MIN_HUMIDITY_OFFSET);
                // uint8_t maxHumidity = eeprom_read_byte((uint8_t*)EEPROM_DATA_START + EEPROM_MAX_HUMIDITY_OFFSET);                 INT0_WriteString("EEPROM marker: ");
                // INT0_WriteUInt(start, 16);
                // INT0_WriteString(" : ");
                //INT0_WriteUInt(marker, 16);
                //INT0_WriteString("\n");
                //INT0_WriteString("Temp: ");
                //INT0_WriteInt(minTemp, 10);
                //INT0_WriteString(" / ");
                //INT0_WriteInt(maxTemp, 10);
                // INT0_WriteString("\nPressure: ");
                // INT0_WriteInt(minHumidity, 10);
                // INT0_WriteString(" / ");
                // INT0_WriteInt(maxHumidity, 10);
                // INT0_WriteString("\n");
            } else if (strcmp(command, "reset") == 0) {
                // reset EEPROM data
                eeprom_update_word((uint16_t*)2, 0);
                INT0_WriteString("EEPROM reset.\n");
                minTemp = 10000;
                maxTemp = -10000;
                minHumidity = 100;
                maxHumidity = 0;
            } else {
                //INT0_WriteString("Unknown command: ");
                //INT0_WriteString(command);
                //INT0_WriteString("\n");
            }
        }
    }
#endif
}

#ifndef ARDUINO
#include <main.cpp.h>
#endif