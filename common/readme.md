# Arduino Common Libraries - Comprehensive Overview

This directory contains a modular collection of C/C++ libraries for AVR microcontroller projects.
These libraries provide drivers for communication protocols, sensors, displays, and control systems,
with a focus on minimal overhead and direct hardware control.

---

## Table of Contents

1. [Library Organization](#library-organization)
2. [Communication Protocols](#communication-protocols)
3. [Sensors & Input Devices](#sensors--input-devices)
4. [Display Controllers](#display-controllers)
5. [Utilities & Frameworks](#utilities--frameworks)
6. [Hardware-Specific Drivers](#hardware-specific-drivers)

---

## Library Organization

All libraries are organized in subdirectories under `/common/`. Each library typically contains:

- **`src/`** - C/C++ source and header files with implementations
- **`readme.md`** - Comprehensive user manual with examples and API reference
- **Supporting files** - Datasheets, diagrams, or configuration files

---

## Communication Protocols

### SPI (Serial Peripheral Interface)

**Directory:** `spi/`  
**Purpose:** High-speed synchronous serial communication (master and slave modes)

**Features:**

- Hardware and software SPI implementations
- Master and slave mode operation
- Configurable clock speeds
- Multi-MCU support (ATmega8535, ATmega328P, ATmega1284P)
- Special features for TFT display integration (MOSI pin as input)

**Common Uses:** Display controllers (TFT), wireless modules (NRF24L01), sensor communication

**See also:** `tinyspi/` for ATtiny-specific SPI via USI

---

### I2C (Two-Wire Interface)

**Directory:** `i2c/`  
**Purpose:** Two-wire serial bus for multi-device communication

**Features:**

- I2C master-only implementation
- Standard (100 kHz) and fast (400 kHz) modes
- Bit-banging software implementation
- Clock stretching support
- ATtiny compatible

**Common Uses:** OLED displays, BME280 sensor, Wii Nunchuk controller, various I2C peripherals

---

### TinySPI (ATtiny USI-based SPI)

**Directory:** `tinyspi/`  
**Purpose:** SPI implementation for ATtiny microcontrollers with Universal Serial Interface (USI)

**Features:**

- 3-wire SPI mode via USI hardware
- Manual clock generation and chip select control
- Compatible with ATtiny45, ATtiny85, ATtiny84

**Note:** Use this instead of `spi/` when targeting ATtiny devices with USI

---

## Sensors & Input Devices

### HC-SR04 Ultrasonic Distance Sensor

**Directory:** `hcsr04/`  
**Purpose:** Non-blocking ultrasonic distance measurement (2-400 cm range)

**Features:**

- Accurate ±0.3 cm measurement
- Configurable timeout for variable range
- Non-blocking measurement API
- Error reporting (sensor failure vs. no object)
- Supports multiple sensor instances
- Optional serial debug output

**Hardware Required:** Two GPIO pins (trigger, echo) + 5V power

**Typical Use Case:** Robot obstacle detection, proximity sensing

---

### BME280 Environmental Sensor

**Directory:** `bme280/`  
**Purpose:** Combined temperature, pressure, and humidity sensing

**Features:**

- I2C communication interface
- High accuracy temperature, pressure, humidity readings
- Altitude calculation from pressure
- Configurable sampling rates and filtering
- Sea-level pressure compensation

**Hardware Required:** I2C connection (SDA, SCL pins)

**Typical Use Case:** Weather monitoring, barometric altitude measurement, climate control

---

### Wii Nunchuk Controller

**Directory:** `nunchuk/`  
**Purpose:** Input device integration with joystick, accelerometer, and buttons

**Features:**

- I2C communication with auto-detection
- Joystick position reading with directional analysis
- 3-axis accelerometer with gravity calibration
- Tilt angle calculation (roll, pitch, yaw)
- Button state detection (C and Z buttons)
- Automatic calibration data retrieval

**Hardware Required:** Wii Nunchuk controller with I2C connector, I2C master driver

**Typical Use Case:** Game controllers, motion-based input, robotic arm control

---

## Display Controllers

### ST7735 TFT Display Driver

**Directory:** `tft/`  
**Purpose:** 128×160 pixel color TFT LCD display control

**Features:**

- 16-bit RGB565 color support
- SPI communication interface
- Complete graphics library: lines, circles, rectangles, triangles
- Text rendering with bitmap fonts (5×8 pixels, scalable)
- Display rotation support (0-3 modes)
- Color inversion, sleep mode, brightness control
- Filled and outline shape primitives
- RGB bitmap drawing with clipping

**Hardware Required:** SPI connection, GPIO pins for DC and RST

**Typical Use Case:** UI displays, data visualization, menus, graphics applications

---

### OLED Display Driver (CH1115 & SSD1306)

**Directory:** `oled/`  
**Purpose:** OLED display control supporting two popular display types

**Supported Controllers:**

- **SSD1306**: 128×64 pixel monochrome display
- **CH1115**: 128×160 pixel monochrome display

**Features:**

- I2C communication interface
- Pixel and line drawing
- Text rendering with bitmap fonts
- Sprite/bitmap rendering
- Display control (contrast, brightness, inversion)
- Low power consumption

**Hardware Required:** I2C connection (SDA, SCL pins)

**Typical Use Case:** Status displays, user interfaces, debug output, data logging

---

## Utilities & Frameworks

### AVRTools - Comprehensive AVR Utilities

**Directory:** `avrtools/`  
**Purpose:** Foundation library providing low-level hardware abstractions and utilities

**Components:**

- GPIO Control (`gpio.h`)
- Millisecond Timer (`millisec.h/cpp`)
- PWM Control (`pwm.h/cpp`)
- Serial Communication - Hardware (`usart_serial.h/cpp`)
- Serial Communication - Software (`int0_serial.h/cpp`)
- Sound Generation (`sound.h/cpp`)
- Arduino-Style Framework (`main.cpp.h`)

**Use Case:** Foundation for all embedded projects; provides essential hardware abstractions

---

## Hardware-Specific Drivers

### Shift Register Driver

**Directory:** `basic/src/shiftregister.h`  
**Purpose:** Parallel-to-serial conversion for expanding GPIO outputs

**Common Uses:**

- Increasing available GPIO pins
- LED matrix control
- Display segment drivers
- Addressing in digital circuits

**Hardware Required:** Shift register IC (e.g., 74HC595), SPI or GPIO interface

---

### NRF24L01+ Wireless Transceiver

**Directory:** `nrf24l01/`  
**Purpose:** 2.4GHz wireless communication for multi-node systems

**Features:**

- Fixed and dynamic payload support (0-32 bytes)
- Auto-acknowledgment with enhanced ShockBurst
- ACK payload for bidirectional communication
- 6 receive pipes for multi-node scenarios
- Configurable RF parameters (channel, data rate, TX power)
- SPI-based communication
- GPIO control for CE (Chip Enable) and CSN (Chip Select)

**Data Rates:** 250 kbps, 1 Mbps, 2 Mbps

**Hardware Required:** NRF24L01+ module, SPI connection, 3.3V power supply

**Typical Use Case:** Wireless sensor networks, remote control, multi-robot communication

---

## Summary Table

| Library      | Type       | Communication         | Target MCU    | Key Features                      |
|--------------|------------|-----------------------|---------------|-----------------------------------|
| **avrtools** | Utilities  | N/A                   | ATmega/ATtiny | GPIO, Timing, PWM, Serial, Sound  |
| **spi**      | Protocol   | Hardware/Software SPI | ATmega        | Master/Slave mode, 3-wire         |
| **i2c**      | Protocol   | Software I2C          | ATtiny/ATmega | Master only, 100-400 kHz          |
| **tinyspi**  | Protocol   | USI SPI               | ATtiny        | 3-wire via USI hardware           |
| **hcsr04**   | Sensor     | GPIO                  | All           | Distance measurement              |
| **bme280**   | Sensor     | I2C                   | All           | Temp/Pressure/Humidity            |
| **nunchuk**  | Input      | I2C                   | All           | Joystick, Accelerometer, Buttons  |
| **tft**      | Display    | SPI                   | All           | 128×160 color LCD, Graphics       |
| **oled**     | Display    | I2C                   | All           | 128×64/160 monochrome, I2C        |
| **robot**    | Control    | GPIO/PWM              | All           | Motor control, Robot coordination |
| **nrf24l01** | Wireless   | SPI                   | All           | 2.4 GHz RF, Multi-node            |
| **basic**    | I/O Expand | SPI/GPIO              | All           | Shift register interface          |

---

## Quick Reference Links

- [AVRTools Documentation](./avrtools/readme.md)
- [SPI Library Documentation](./spi/readme.md)
- [I2C Library Documentation](./i2c/readme.md)
- [HC-SR04 Documentation](./hcsr04/readme.md)
- [BME280 Documentation](./bme280/readme.md)
- [TFT Display Documentation](./tft/readme.md)
- [OLED Display Documentation](./oled/readme.md)
- [NRF24L01 Documentation](./nrf24l01/readme.md)
- [Nunchuk Documentation](./nunchuk/readme.md)
