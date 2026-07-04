#include "gpio.h"

uint16_t readADC(uint8_t channel) {
    // configure ADC
    ADCSRA = _BV(ADEN) | _BV(ADPS2) | _BV(ADPS1) | _BV(ADPS0); // Enable ADC, prescaler 128
    // select channel - Use AVcc for reference - ADLAR=0  ADCH contains the 2 MSB, ADCL contains the 8 LSB
    ADMUX = _BV(REFS0) | (channel & 0x0F); // select ADC channel for VRX
    // Start an ADC conversion by setting ADSC bit (bit 6)
	ADCSRA = ADCSRA | (1 << ADSC);	
	// Wait until the ADSC bit has been cleared
	while(ADCSRA & (1 << ADSC));
    // read values from ADC registers (ADLAR=0, so ADCL contains the 8 LSB, ADCH contains the 2 MSB)
    uint8_t low  = ADCL;
    uint8_t high = ADCH;
    return (high << 8) | low;
}