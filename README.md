# Arduino AVR Microcontroller Projects

A comprehensive collection of C/C++ projects, libraries, and drivers for ATmega and ATtiny [AVR](https://en.wikipedia.org/wiki/AVR_microcontrollers) microcontrollers series.
This repository provides low-level, hardware-focused implementations with minimal overhead.

## 📋 Project Structure

```text
.
├── ATmega1284p/          # 16KB RAM, 128KB Flash - Advanced projects
├── ATmega8515/           # 512B RAM, 8KB Flash - Mid-range projects
├── ATmega8535/           # 512B RAM, 8KB Flash - Sensor & display projects
├── ATtiny45/             # 256B RAM, 4KB Flash - Minimal projects
├── ATtiny84/             # 512B RAM, 8KB Flash - Enhanced ATtiny projects
├── UNO/                  # Arduino UNO (ATmega328P) - Arduino-based projects
└── common/               # Shared libraries for all platforms
```

---

## 🎯 Quick Start

### Hardware

- **PC/Laptop** - Linux Ubuntu
- **AVR Microcontroller Board** - Any from this repository (e.g., Arduino UNO, ATmega1284p)
- **USB Programmer** - One of:
  - USB cable (for Arduino UNO with built-in bootloader)
  - USBtiny or USBasp programmer (for bare ATmega chips)

### Installation

**Ubuntu:**

```bash
sudo apt install avrdude gcc-avr gcc-doc avr-libc
```

**Development Environment:**

1. Install [Visual Studio Code](https://code.visualstudio.com/)

    ```bash
    # Install VS Code
    sudo snap install code
    ```

2. Add [PlatformIO IDE](https://marketplace.visualstudio.com/items?itemName=platformio.platformio-ide) extension
  
    - Open **Extensions** (Ctrl+Shift+X)
    - Search for "PlatformIO IDE"
    - Click **Install**

3. Add [PlatformIO CLI](https://docs.platformio.org/en/latest/core/installation/shell-commands.html#piocore-install-shell-commands) using link.

    ```bash
    ln -s ~/.platformio/penv/bin/platformio ~/.local/bin/platformio
    ln -s ~/.platformio/penv/bin/pio ~/.local/bin/pio
    ln -s ~/.platformio/penv/bin/piodebuggdb ~/.local/bin/piodebuggdb
    ```

4. Setup permission to allow USB access to your user account.

    ```bash
    # Add user to dialout group
    sudo usermod -aG dialout $USER
    ```

### First Build

```bash
cd ATmega1284p/invaders          # Navigate to a project
pio run                          # Build with PlatformIO
```

---

## 🧇 Microcontroller Overview

| MCU                         | RAM   | Flash  | EEPROM | Pins     | Speed  | Use Case                       |
|-----------------------------|------:|-------:|-------:|---------:|-------:|--------------------------------|
| [ATmega1284p](ATmega1284p/) | 16 KB | 128 KB | 4 KB   | 40 (DIP) |  8 MHz | Complex projects, games        |
| [ATmega8535](ATmega8535/)   | 512 B | 8 KB   | 512 B  | 40 (DIP) |  8 MHz | Sensors, displays, robotics    |
| [ATmega8515](ATmega8515/)   | 512 B | 8 KB   | 512 B  | 40 (DIP) |  8 MHz | Mid-range projects             |
| [ATtiny84](ATtiny84/)       | 512 B | 8 KB   | 512 B  | 14 (DIP) |  8 MHz | Resource-constrained systems   |
| [ATtiny45](ATtiny45/)       | 256 B | 4 KB   | 256 B  | 8 (DIP)  |  8 MHz | Minimal footprint projects     |
| [UNO/ATmega32p](UNO/)       | 2 KB  | 32 KB  | 1 KB   | 28       | 16 MHz | Arduino ecosystem, prototyping |

---

## 📚 Common Libraries

See [common/README.md](common/readme.md) for comprehensive library documentation.

### Available Libraries

| Library                          | Purpose                     |
|----------------------------------|-----------------------------|
| [**SPI**](common/spi/)           | Serial Peripheral Interface |
| [**I2C**](common/i2c/)           | Two-Wire Interface          |
| [**TinySPI**](common/tinyspi/)   | USI-based SPI               |
| [**NRF24L01**](common/nrf24l01/) | 2.4GHz wireless module      |
| [**OLED**](common/oled/)         | SSD1306 display driver      |
| [**BME280**](common/bme280/)     | Environmental sensor        |
| [**HC-SR04**](common/hcsr04/)    | Ultrasonic distance sensor  |
| [**TFT**](common/tft/)           | TFT display controller      |
| [**Nunchuk**](common/nunchuk/)   | Wii Nunchuk controller      |
| [**AVRTools**](common/avrtools/) | Low-level utilities         |

---

## 🔧 Programming & Configuration

### Programmer Support

- **AVR Pocket Programmer** (red sparkfun PCB USB id 1781:0c9f Multiple Vendors USBtiny)
- **AVR USBasp Programmer** (blue PCB USB id 16c0:05dc Van Ooijen Technische Informatica)
- **Arduino as ISP**
- **AVRISP mkII**

### Fuse Configuration

Use the [Engbedded Fuse Calculator](https://www.engbedded.com/fusecalc/) to generate fuse values.

Example: Set ATmega1284p to 8MHz internal clock (64ms startup)

```bash
avrdude -c usbtiny -p m1284p -U lfuse:w:0xe2:m -U hfuse:w:0x99:m
```

### Reading/Writing Memory

**Read EEPROM:**

```bash
avrdude -c usbtiny -p m1284p -U eeprom:r:-:i
```

**Write Firmware:**

```bash
avrdude -c usbtiny -p m1284p -U flash:w:firmware.hex:i
```

---

## 📂 Project Layout

Each MCU folder contains:

```text
MCU/
├── readme.md                  # MCU-specific documentation
├── MCU-datasheet.pdf          # Full datasheet
├── MCU-pinout.png             # Visual pinout reference
├── Project1/
│   ├── platformio.ini         # PlatformIO configuration
│   ├── include/               # Header files
│   ├── src/                   # Source code
│   ├── lib/                   # Project-specific libraries
│   └── test/                  # Unit tests
└── Project2/
    └── ...
```

---

## 🛠️ Development Workflow

### Build & Upload

```bash
# Navigate to project
cd ATmega1284p/invaders

# Build
pio run

# Upload
pio run -t upload

# Monitor serial output and send command
pio device monitor --echo
```

---

## 📋 MCU Documentation

- **[ATmega1284p](ATmega1284p/readme.md)** - 128KB Flash, 16KB RAM
- **[ATmega8535](ATmega8535/readme.md)** - 8KB Flash, 512B RAM
- **[ATmega8515](ATmega8515/readme.md)** - 8KB Flash, 512B RAM
- **[ATtiny84](ATtiny84/readme.md)** - 8KB Flash, 512B RAM, 14 pins
- **[ATtiny45](ATtiny45/readme.md)** - 4KB Flash, 256B RAM, 8 pins
- **[UNO/ATmega32](UNO/readme.md)** - 32KB Flash, 2KB RAM (Arduino compatible)
- **[Common Libraries](common/readme.md)** - Shared drivers and utilities

---

## 🔗 Resources

- [AVR Instruction Set Manual](https://ww1.microchip.com/downloads/en/DeviceDoc/AVR-InstructionSet-Manual-DS40002198.pdf)
- [PlatformIO Documentation](https://docs.platformio.org/)
- [AVRDude Manual](https://www.nongnu.org/avrdude/user-manual/avrdude.html)
- [Engbedded Fuse Calculator](https://www.engbedded.com/fusecalc/)

---

## 📝 Notes

- All projects use **C/C++** with minimal dependencies
- **PlatformIO** is the recommended build system
- **AVRDude** is used for programming
- Some projects include **simavr** simulation support
- Emphasis on **direct hardware control** over abstraction layers
