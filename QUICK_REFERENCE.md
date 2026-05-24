# Quick Reference Guide

Fast lookup for common commands, pins, and configurations.

---

## Microcontroller Comparison

| MCU                         | Flash | RAM  | EEPROM | Pins | Speed | Best For                          |
|-----------------------------|------:|-----:|-------:|-----:|------:|-----------------------------------|
| [ATmega1284p](ATmega1284p/) | 128 K | 16K  | 4KB    | 40   |  8MHz | Games, displays, complex projects |
| [ATmega8535](ATmega8535/)   | 8K    | 512B | 512B   | 40   |  8MHz | Sensors, robotics, displays       |
| [ATmega8515](ATmega8515/)   | 8K    | 512B | 512B   | 40   |  8MHz | External memory applications      |
| [ATtiny84](ATtiny84/)       | 8K    | 512B | 512B   | 14   |  8MHz | I2C/SPI projects, minimal size    |
| [ATtiny45](ATtiny45/)       | 4K    | 256B | 256B   | 8    |  8MHz | Ultra-minimal applications        |
| [UNO/ATmega328p](UNO/)      | 32K   | 2K   | 1KB    | 20   | 16MHz | Arduino ecosystem, prototyping    |

---

## Common AVRDude Commands

### Read Fuse Settings

```bash
# ATmega1284p
avrdude -c usbtiny -p m1284p -U lfuse:r:-:h -U hfuse:r:-:h

# ATmega8535
avrdude -c usbtiny -p m8535 -U lfuse:r:-:h -U hfuse:r:-:h

# ATtiny84
avrdude -c usbtiny -p t84 -U lfuse:r:-:h -U hfuse:r:-:h -U efuse:r:-:h

# ATtiny45
avrdude -c usbtiny -p t45 -U lfuse:r:-:h -U hfuse:r:-:h
```

### Write Fuse Settings (8MHz Internal, 64ms Startup)

```bash
# ATmega1284p
avrdude -c usbtiny -p m1284p -U lfuse:w:0xe2:m -U hfuse:w:0x99:m

# ATmega8535
avrdude -c usbtiny -p m8535 -U lfuse:w:0xe4:m -U hfuse:w:0xd9:m

# ATtiny84
avrdude -c usbtiny -p t84 -U lfuse:w:0xE2:m -U hfuse:w:0xDF:m -U efuse:w:0xff:m

# ATtiny45
avrdude -c usbtiny -p t45 -U lfuse:w:0xe2:m -U hfuse:w:0x99:m
```

### Read/Write Memory

```bash
# Read EEPROM
avrdude -c usbtiny -p m1284p -U eeprom:r:eeprom.hex:i

# Write firmware
avrdude -c usbtiny -p m1284p -U flash:w:firmware.hex:i

# Erase chip
avrdude -c usbtiny -p m1284p -e

# Read flash
avrdude -c usbtiny -p m1284p -U flash:r:flash.hex:i
```

---

## PlatformIO Commands

```bash
# Build project
pio run

# Clean build files
pio run -t clean

# Upload to board
pio run -t upload

# Monitor serial output
pio device monitor

# List boards
pio boards | grep tiny

# Verbose output
pio run -v
```

### Common Board IDs

```text
Arduino Boards:
  arduino:avr:uno              # Arduino UNO

PlatformIO:
  ATmega328P                   # m328p
  ATmega1284p                  # m1284p
  ATmega8535                   # m8535
  ATmega8515                   # m8515
  ATtiny84                     # tiny84
  ATtiny45                     # tiny45
```

---

## Programmer Device Codes (AVRDude)

```text
Microcontrollers:
  -p m1284p      # ATmega1284P
  -p m8535       # ATmega8535
  -p m8515       # ATmega8515
  -p t84         # ATtiny84
  -p t45         # ATtiny45
  -p m328p       # ATmega328P (Arduino UNO)

Programmers:
  -c usbtiny     # AVR Pocket Programmer
  -c usbasp      # USBasp ISP
  -c arduino     # Arduino as ISP (via serial)
```

---

## Compiler Flags for Size Optimization

```bash
# In platformio.ini or Makefile:
build_flags =
  -Os
  -mcall-prologues
  -ffunction-sections
  -fdata-sections
  -Wl,--gc-sections
```

---

## Register Macros (Quick Reference)

```cpp
// Set pin as output
DDRB |= (1 << DDB0);      // DDR - Data Direction Register

// Set pin HIGH
PORTB |= (1 << PORTB0);   // PORT - Output register

// Set pin LOW
PORTB &= ~(1 << PORTB0);

// Read pin
uint8_t state = (PINB >> PINB0) & 1;  // PIN - Input register
```

---

## Troubleshooting Checklist

- ✅ Correct board selected in IDE
- ✅ USB port is correct (/dev/ttyACM0)
- ✅ Device is visible: `lsusb` (Linux)
- ✅ User in dialout group: `groups $USER`
- ✅ Baud rate matches code (usually 9600 or 115200)
- ✅ Voltage is 5V (or 3.3V for some boards)
- ✅ No shorts in circuit
- ✅ Fuses set correctly (if not bootloaded)
- ✅ Code compiles without errors
- ✅ Code size fits in Flash memory
- ✅ Stack overflow (SRAM issues) unlikely

---

## Useful Tools

| Tool    | Purpose                 | Command                            |
|---------|-------------------------|------------------------------------|
| avrdude | Program microcontroller | `avrdude -c usbtiny -p m1284p ...` |
| git     | Version control         | `git clone ...`                    |
| pio     | Build & upload          | `pio run -t upload`                |

---

## Documentation Links

- [Root README](README.md) - Project overview
- [Getting Started](GETTING_STARTED.md) - Setup guide
- [Common Libraries](common/readme.md) - Library reference
- [MCU Documentation](ATmega1284p/readme.md) - ATmega1284p Board-specific guides

---

## Quick Links

- [Arduino Official](https://www.arduino.cc/)
- [Microchip Datasheets](https://www.microchip.com/)
- [AVR LibC](https://www.nongnu.org/avr-libc/)
- [PlatformIO Docs](https://docs.platformio.org/)
- [Fuse Calculator](https://www.engbedded.com/fusecalc/)

---

## Finding Your Way

**Lost? Try these:**

1. What board am I using? → See [Microcontroller Comparison](#microcontroller-comparison)
2. How do I program it? → See AVRDude commands above
3. What libraries are available? → Read [common/readme.md](common/readme.md)
4. How do I set fuses? → Search this page for "Fuse Settings"
5. What pin is what? → See MCU datasheet
6. Full documentation? → Check individual MCU folder READMEs
