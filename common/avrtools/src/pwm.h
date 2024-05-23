#include <avr/io.h>
#include <stdint.h>

#if defined(__AVR_ATmega8535__) || defined(__AVR_ATmega8515__)
// const uint8_t PWM_OC0 = 0; // 0A is used for couting millisecond do not use
#endif

#if defined(__AVR_ATmega1284P__) || defined(__AVR_ATmega328P__) || defined(__AVR_ATtiny45__)
// const uint8_t PWM_OC0A = 1; // 0A is used for couting millisecond do not use
const uint8_t PWM_OC0B = 2; // do not use if SPI is used on ATmega1284P
#endif

#if defined(__AVR_ATmega8535__) || defined(__AVR_ATmega8515__) || defined(__AVR_ATmega1284P__) || defined(__AVR_ATmega328P__) || defined(__AVR_ATtiny45__)
const uint8_t PWM_OC1A = 3;
const uint8_t PWM_OC1B = 4; // do not use if SPI is used on ATmega328P
#endif

#if defined(__AVR_ATmega1284P__) || defined(__AVR_ATmega328P__)
const uint8_t PWM_OC2A = 5; // do not use if SPI is used on ATmega328P
const uint8_t PWM_OC2B = 6;
#endif

#if defined(__AVR_ATmega1284P__)
const uint8_t PWM_OC3A = 7; // do not use if SPI is used on ATmega1284P
const uint8_t PWM_OC3B = 8; // do not use if SPI is used on ATmega1284P
#endif

void enablePWM(uint8_t pwm_pin);
void setPWM(uint8_t pwm_pin, uint8_t value);

void enableServoPWM(uint8_t pwm_pin);
void setServoPWM(uint8_t pwm_pin, uint16_t value);