Tips for ATmega8515

The ATmega8515 provides the following features: 8K bytes of In-System Programmable
Flash with Read-While-Write capabilities, 512 bytes EEPROM, 512 bytes SRAM, an
External memory interface, 35 general purpose I/O lines, 32 general purpose working
registers, two flexible Timer/Counters with compare modes, Internal and External inter-
rupts, a Serial Programmable USART, a programmable Watchdog Timer with internal
Oscillator, a SPI serial port, and three software selectable power saving modes. The Idle
mode stops the CPU while allowing the SRAM, Timer/Counters, SPI port, and Interrupt
system to continue functioning. The Power-down mode saves the Register contents but
freezes the Oscillator, disabling all other chip functions until the next interrupt or hard-
ware reset. In Standby mode, the crystal/resonator Oscillator is running while the rest of
the device is sleeping. This allows very fast start-up combined with low-power

Dev env

apt install avrdude gcc-avr gcc-doc avr-libc

FUSE settings

To calculate fuse mask go to https://www.engbedded.com/fusecalc/

To read current fuse settings

avrdude -c usbtiny -p m8515 -U lfuse:r:-:i -U hfuse:r:-:i

To write fuse
avrdude -c usbtiny -p m8515 -U lfuse:w:0xe4:m -U hfuse:w:0xd9:m 

this set the clock to 8MHz internal (slow startup 64ms)

Reading memory

read eeprom and dump on stdout (-) in intel hex format
avrdude -c usbtiny -p m8515 -U eeprom:r:-:i 

Write firmware.hex to flash
avrdude -c usbtiny -p m8515 -U flash:w:firmware.hex:i 