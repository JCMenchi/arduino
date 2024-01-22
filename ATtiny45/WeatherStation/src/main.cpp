
#include "BME280.h"
#include <CH1115Display.h>
#include <util/delay.h>

#include <avr/io.h>
#include <stdlib.h>
#include <string.h>

const int LED_PIN = PB4; // PB4 leg 3 on DIP8 chip

CH1115Display ch1115(128, 64);
BME280 bme280;

void setup() {
  // init LED to off
  DDRB |= (1 << LED_PIN);
  PORTB &= ~(1 << LED_PIN);
  
  // init OLED display
  ch1115.init(0x20);
  ch1115.drawScreen(0x00);

  bool b = bme280.begin();
  if (b) {
    ch1115.drawString(0, 0, "BME280");
  } else {
    ch1115.drawString(0, 0, "BME280 init KO.");
  }
}

void readBME280() {
  char buf[12];
  /*int32_t temp(0), hum(0), pres(0);

  bme280.read(pres, temp, hum);

  ch1115.drawString(8, 24, "T");
  ltoa(temp, buf, 10);
  ch1115.drawString(20, 24, buf);

  ch1115.drawString(8, 32, "H");
  ltoa(hum, buf, 10);
  ch1115.drawString(20, 32, buf);

  ch1115.drawString(8, 40, "P");
  ltoa(pres, buf, 10);
  ch1115.drawString(20, 40, buf);
*/

  int32_t temp;
  uint32_t hum = bme280.hum(temp);
  int32_t pres = 0; //bme280.pres(temp);

  ltoa(temp, buf, 10);
  uint8_t l = strlen(buf);
  buf[l] = 'C';
  buf[l - 1] = buf[l - 2];
  buf[l - 2] = '.';
  buf[l + 1] = '\0';

  ch1115.drawString(8, 24, "T");
  ch1115.drawString(20, 24, buf);

  
  ltoa(hum, buf, 10);
  buf[l] = '%';
  buf[l - 1] = buf[l - 2];
  buf[l - 2] = '.';
  buf[l + 1] = '\0';
  ch1115.drawString(8, 32, "H");
  ch1115.drawString(20, 32, buf);


  ltoa(pres, buf, 10);
  ch1115.drawString(8, 40, "P");
  ch1115.drawString(20, 40, buf);
}

int main(void) {

  setup();
  for (;;) {
    _delay_ms(5000);
    // blink LED while reading BME280
    PORTB |= (1 << LED_PIN);
    _delay_ms(100);
    readBME280();
    PORTB &= ~(1 << LED_PIN);
  }

  return 0;
}