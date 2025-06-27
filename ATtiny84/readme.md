# Projects based on ATtiny84

Features (from datasheet)

High Performance, Low Power AVR® 8-Bit Microcontroller

- Advanced RISC Architecture
  - 120 Powerful Instructions – Most Single Clock Cycle Execution
  - 32 x 8 General Purpose Working Registers
  - Fully Static Operation
- Non-Volatile Program and Data Memories
  - 2/4/8K Bytes of In-System Programmable Program Memory Flash
- Endurance: 10,000 Write/Erase Cycles
  - 128/256/512 Bytes of In-System Programmable EEPROM
- Endurance: 100,000 Write/Erase Cycles
  - 128/256/512 Bytes of Internal SRAM
  - Data Retention: 20 years at 85°C / 100 years at 25°C
  - Programming Lock for Self-Programming Flash & EEPROM Data Security
- Peripheral Features
  - One 8-Bit and One 16-Bit Timer/Counter with Two PWM Channels, Each
  - 10-bit ADC
- 8 Single-Ended Channels
- 12 Differential ADC Channel Pairs with Programmable Gain (1x / 20x)
  - Programmable Watchdog Timer with Separate On-chip Oscillator
  - On-chip Analog Comparator
  - Universal Serial Interface
- Special Microcontroller Features
  - debugWIRE On-chip Debug System
  - In-System Programmable via SPI Port
  - Internal and External Interrupt Sources: Pin Change Interrupt on 12 Pins
  - Low Power Idle, ADC Noise Reduction, Standby and Power-Down Modes
  - Enhanced Power-on Reset Circuit
  - Programmable Brown-out Detection Circuit
  - Internal Calibrated Oscillator
  - On-chip Temperature Sensor
- I/O and Packages
  - Available in 20-Pin QFN/MLF & 14-Pin SOIC and PDIP
  - Twelve Programmable I/O Lines

![ATtiny84 pin layout](./attiny84.png "ATtiny84 pin layout PDIP14").

## Set up development environment

On ubuntu install AVR toolchain

```bash
apt install avrdude gcc-avr gcc-doc avr-libc
```

Make sure to be in dialout group to be able to use different USB serial interface.

```bash
sudo usermod -aG dialout <username>
```

To develop with [vscode](https://code.visualstudio.com/) add [PlatformIO](https://platformio.org
) vsix [extension](https://marketplace.visualstudio.com/items?itemName=platformio.platformio-ide).

## MCU configuration

### FUSE settings

To calculate fuse mask go to [Engbedded Fuse Calculator](https://www.engbedded.com/fusecalc/)

To read current fuse settings

```bash
avrdude -c usbtiny -p t84 -U lfuse:r:-:h -U hfuse:r:-:h -U efuse:r:-:h
```

To write fuse
Following command sets the clock to 8MHz internal (slow startup 64ms) without divider.
Default factory settings divide by 8, so the clock is 1MHz.
Classical arduino lib assume attiny84 F_CPU is 8MHz.

```bash
avrdude -c usbtiny -p t84 -U lfuse:w:0xE2:m -U hfuse:w:0xDF:m -U efuse:w:0xff:m
```

### Read/Write memory

Read EEPROM and dump on stdout (-) in intel hex format

```bash
avrdude -c usbtiny -p t84 -U eeprom:r:-:i 
```

Write firmware.hex to flash with [AVR pocket programmer](https://www.sparkfun.com/products/9825)

```bash
avrdude -c usbtiny -p t84 -U flash:w:firmware.hex:i 
```

Raed flash memory

```bash
avrdude -c usbtiny -p t84 -U flash:r:-:i 
```
