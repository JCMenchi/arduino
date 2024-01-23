
#include "BME280.h"
#include <CH1115Display.h>
#include <util/delay.h>

#include <avr/io.h>
#include <stdlib.h>
#include <string.h>

const int LED_PIN = PB4; // PB4 leg 3 on DIP8 chip

CH1115Display ch1115(128, 64);

void setup() {
  // init LED to off
  DDRB |= (1 << LED_PIN);
  PORTB &= ~(1 << LED_PIN);
  
  // init OLED display
  ch1115.init(0x20);
  ch1115.drawScreen(0x00);

  bool b = BME280_begin();
  if (b) {
    ch1115.drawString(0, 0, "BME280");
  } else {
    ch1115.drawString(0, 0, "BME280 init KO.");
  }
}

void readBME280() {
  char buf[12];
  int32_t temp, pres;
  uint16_t hum = 0;

  //BME280_read(pres, temp, hum);

  //hum = BME280_hum(temp);
  pres = BME280_pres(temp);

  ltoa(temp, buf, 10);
  uint8_t l = strlen(buf);
  buf[l] = 'C';
  buf[l - 1] = buf[l - 2];
  buf[l - 2] = '.';
  buf[l + 1] = '\0';

  ch1115.drawString(8, 24, "T");
  ch1115.drawString(20, 24, buf);

  ltoa(hum, buf, 10);
  l = strlen(buf);
  buf[l] = '%';
  buf[l - 1] = buf[l - 2];
  buf[l - 2] = '.';
  buf[l + 1] = '\0';
  ch1115.drawString(8, 32, "H");
  ch1115.drawString(20, 32, buf);

  ltoa(pres, buf, 10);
  l = strlen(buf);
  buf[l] = 'h';
  buf[l - 1] = buf[l - 2];
  buf[l - 2] = '.';
  buf[l + 1] = '\0';
  ch1115.drawString(8, 40, "P");
  ch1115.drawString(20, 40, buf);
}

int main(void) {

  setup();
  _delay_ms(1000);
  for (;;) {
    // blink LED while reading BME280
    PORTB |= (1 << LED_PIN);
    _delay_ms(100);
    readBME280();
    PORTB &= ~(1 << LED_PIN);
     _delay_ms(5000);
  }

  return 0;
}