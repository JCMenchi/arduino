#include "usart_serial.h"
#include <stddef.h>
#include <util/delay.h>
#include <avr/io.h>
#include <avr/wdt.h>
#include <stdlib.h>
#include <string.h>
#include <avr/pgmspace.h>

#include "millisec.h"
#include "nrf24mgr.h"
#include "SPIManager.h"
#include "gpio.h"
#include <SSD1306Display.h>
#include "BME280.h"
#include "TinyI2CMaster.h"
#include "pwm.h"

#define HAS_DISPLAY

const uint8_t ON_LED_PIN = 0;
const uint8_t RADIO_COM_LED_PIN = 1;

const uint8_t RADIO_CE_PIN = 2;

SSD1306Display display(128, 32);
SPIManager spi;
NRF24Manager radio(1);

// init MCU
void setup() {

  // setup OC1A (PD5) for PWM
  enableServoPWM(PWM_OC1A);
  setServoPWM(PWM_OC1A, 3);

  enableServoPWM(PWM_OC2A);
  setServoPWM(PWM_OC2A, 3);


  // init debug LED
  GPIO_OUTPUT(A, ON_LED_PIN);
  GPIO_OUTPUT(A, RADIO_COM_LED_PIN);
  GPIO_SET_LOW(A, RADIO_COM_LED_PIN);

  // startup blinking of ON LED
  GPIO_SET_HIGH(A, ON_LED_PIN);
  _delay_ms(200);
  GPIO_SET_LOW(A, ON_LED_PIN);
  _delay_ms(500);
  GPIO_SET_HIGH(A, ON_LED_PIN);
  _delay_ms(200);
  GPIO_SET_LOW(A, ON_LED_PIN);
  _delay_ms(500);
  GPIO_SET_HIGH(A, ON_LED_PIN);

  // init serial com
  USART_Init(BAUD_RATE_57600, SerialCommandMgr::serialInput);

  // init SPI bus to control NRF24
  spi.startMaster();
  // init I2C bus
  TinyI2C.init();

  // init NRF24
  _delay_ms(100); // give some time to NRF24 module to start
  radio.init(&spi, RADIO_CE_PIN);

#ifdef HAS_DISPLAY
  // init OLED display
  display.init(0x20);
  display.flip(SSD1306_ON);
  display.drawScreen(0x00);
  display.drawPString(0, SSD1306_LINE0, PSTR("ATmega1284P"));
#endif

  // init weather sensor
  bool ok = BME280_begin();
  if (!ok) {
    USART_WriteString("BME280 init failed.\n");
  } else {
    USART_WriteString("BME280 model: ");
    uint8_t model = BME280_chipModel();
    if (model == ChipModel_BME280) {
      USART_WriteString("BME280\n");
    } else if (model == ChipModel_BMP280) {
      USART_WriteString("BMP280\n");
    } else {
      USART_WriteString("UNKNOWN\n");
    }
  }

  // ready to enter main loop
  USART_WriteString("ATmega1284P Ready\n");

  radio.summary();
}

void sendMsg(const char* msg) {
  #ifdef HAS_DISPLAY
  display.clearPage(3);
  display.drawString(0, SSD1306_LINE3, "SND:");
  display.drawString(26, SSD1306_LINE3, msg);
  #endif

  GPIO_SET_HIGH(A, RADIO_COM_LED_PIN);
  radio.send(msg);
  radio.listen();
  GPIO_SET_LOW(A, RADIO_COM_LED_PIN);
}

void getWeather() {
  char weatherinfo[16];

  int32_t pressure = 0;
  int32_t temperature = 0;
  uint16_t humidity = 0;
  BME280_read(pressure, temperature, humidity);
  
  ltoa(temperature/100, weatherinfo, 10);
  strcat(weatherinfo, "C ");
  ltoa(pressure/100, weatherinfo+strlen(weatherinfo), 10);
  strcat(weatherinfo, " hPa ");
  ltoa(humidity/100, weatherinfo+strlen(weatherinfo), 10);
  strcat(weatherinfo, "%");

  USART_WriteString("\n");
  USART_WriteString(weatherinfo);
  USART_WriteString("\n");

  #ifdef HAS_DISPLAY
  display.clearPage(0);
  display.drawString(3, SSD1306_LINE0, weatherinfo);
  #endif

  GPIO_SET_HIGH(A, RADIO_COM_LED_PIN);
  radio.send(weatherinfo);
  radio.listen();
  GPIO_SET_LOW(A, RADIO_COM_LED_PIN);
}

uint16_t servoPos = 12;

void execCommand(const char* cmd) {
  // check if command is defined
  if (cmd == NULL || strlen(cmd) ==0) return;

  #ifdef HAS_DISPLAY
  display.clearPage(1);
  display.drawString(0, SSD1306_LINE1, "EXE:");
  display.drawString(26, SSD1306_LINE1, cmd);
  #endif
  // USART_WriteString("Exec command: ");
  // USART_WriteString(cmd);
  // USART_WriteString("\n\n");
  if (strcmp(cmd, "status") == 0) {
    radio.summary();
    //chuk.initialize();
    //chuk.display();
  } else if (strcmp(cmd, "bme280") == 0) {
    getWeather();
  } else if (strcmp(cmd, "info") == 0) {
    radio.info();
  } else if (strcmp(cmd, "reset") == 0) {
    radio.reset(0);
  } else if (strcmp(cmd, "auto") == 0) {
    radio.reset(1);
  } else if (strcmp(cmd, "on") == 0) {
    radio.changeState(NRF24_POWERUP);
    radio.listen();
  } else if (strcmp(cmd, "off") == 0) {
    radio.changeState(NRF24_POWERDOWN);
  
  } else if (strcmp(cmd, "1") == 0) {
    setServoPWM(PWM_OC1A, 1);
  } else if (strcmp(cmd, "2") == 0) {
    setServoPWM(PWM_OC1A, 2);
  } else if (strcmp(cmd, "3") == 0) {
    setServoPWM(PWM_OC1A, 3);
  } else if (strcmp(cmd, "4") == 0) {
    setServoPWM(PWM_OC1A, 4);
  } else if (strcmp(cmd, "5") == 0) {
    setServoPWM(PWM_OC1A, 5);
  } else if (strcmp(cmd, "6") == 0) {
    setServoPWM(PWM_OC1A, 6);
  } else if (strcmp(cmd, "7") == 0) {
    setServoPWM(PWM_OC1A, 7);
  } else if (strcmp(cmd, "8") == 0) {
    setServoPWM(PWM_OC1A, 8);
  } else if (strcmp(cmd, "9") == 0) {
    setServoPWM(PWM_OC1A, 14);
  } else if (strcmp(cmd, "+") == 0) {
    servoPos++;
    setServoPWM(PWM_OC2A, servoPos);
    setServoPWM(PWM_OC1A, servoPos);
  } else if (strcmp(cmd, "-") == 0) {
    servoPos--;
    setServoPWM(PWM_OC2A, servoPos);
    setServoPWM(PWM_OC1A, servoPos);
  } else if (cmd[0] == 'p') {
    servoPos = atoi(cmd+1);
    //setServoPWM(PWM_OC2A, servoPos);
    setServoPWM(PWM_OC1A, servoPos);
  } else if (strlen(cmd) > 0) {
    sendMsg(cmd);
  }
}

void execRemoteCommand(const char* cmd) {
  // check if command is defined
  if (cmd == NULL || strlen(cmd) ==0) return;

  // USART_WriteString("Exec remote command: ");
  // USART_WriteString(cmd);
  // USART_WriteString("\n\n");
  if (strcmp(cmd, "bme280") == 0) {
    getWeather();
  } else if (strcmp(cmd, "info") == 0) {
    radio.info();
  } else if (strcmp(cmd, "reset") == 0) {
    radio.reset(0);
  } else if (strcmp(cmd, "auto") == 0) {
    radio.reset(1);
  } else if (strcmp(cmd, "on") == 0) {
    radio.changeState(NRF24_POWERUP);
    radio.listen();
  } else if (strcmp(cmd, "off") == 0) {
    radio.changeState(NRF24_POWERDOWN);
  }
}

uint32_t nbmsg = 0;

void loop() {
  // get current time
  uint32_t now = milliseconds();

  if (SerialCommandMgr::hasCommand()) {
    execCommand(SerialCommandMgr::command());
  }

  if (radio.dataAvailable()) {
    uint8_t msgsize = 0;
    uint8_t* msg = radio.read_binary_message(msgsize);
    USART_WriteString("now ");
    USART_WriteUInt(now/1000);
    USART_WriteString("s: ");
    USART_WriteUInt(msgsize);

    if (msgsize >= 10) {
      nbmsg++;
      uint8_t datasize = msg[0];
      USART_WriteString(" ");
      USART_WriteUInt(datasize);
      USART_WriteString(" pos: ");
      USART_WriteChar(msg[1]);
      USART_WriteString(" s: ");
      USART_WriteInt(msg[8]);
      setServoPWM(PWM_OC1A, msg[8]*10);
      USART_WriteString(" (");
      USART_WriteInt(msg[9]);
      USART_WriteString(", ");
      USART_WriteInt(msg[10]);
      USART_WriteString(")");

      int16_t x = 0;
      int16_t y = 0;
      int16_t z = 0;
      memcpy(&x, msg + 2, 2);
      memcpy(&y, msg + 4, 2);
      memcpy(&z, msg + 6, 2);

      USART_WriteString(" orientation: ");
      USART_WriteInt(x);
      USART_WriteString(", ");
      USART_WriteInt(y);
      USART_WriteString(", ");
      USART_WriteInt(z);

      #ifdef HAS_DISPLAY
      display.clearPage(2);
      display.drawString(0, SSD1306_LINE2, "REC:");
      display.drawChar(26, SSD1306_LINE2, msg[1]);
      display.drawInt(38, SSD1306_LINE2, x, 10);
      display.drawInt(68, SSD1306_LINE2, y, 10);
      display.drawInt(100, SSD1306_LINE2, z, 10);
      #endif

    } else {
      USART_WriteString((char*)msg);
    }
    USART_WriteString("\n");

  }

}

#include <main.cpp.h>