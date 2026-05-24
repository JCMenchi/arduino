# ATmega1284p Microcontroller Projects

The ATmega1284p is a high-performance AVR microcontroller with 128KB Flash and 16KB SRAM,
making it ideal for complex projects including graphics, gaming, and advanced sensor applications.

## 📊 Specifications

| Feature          | Value                |
|------------------|----------------------|
| **Flash Memory** | 128 KB               |
| **SRAM**         | 16 KB                |
| **EEPROM**       | 4 KB                 |
| **Pins (DIP)**   | 40                   |
| **Max Clock**    | 20 MHz               |
| **Timers**       | 4 (16-bit & 8-bit)   |
| **PWM Channels** | 6                    |
| **ADC Channels** | 8 (10-bit)           |
| **I/O Ports**    | A, B, C, D (32 pins) |

![ATmega1284p pin layout](./ATmega1284p-pinout.png "ATmega1284p pin layout DIP40")

**📄 [Full Datasheet](./ATmega1284p-datasheet.pdf)**

---

## 🚀 Quick Start

### Installation (Ubuntu/Debian)

```bash
sudo apt install avrdude gcc-avr gcc-doc avr-libc
```

### IDE Setup

Install [Visual Studio Code](https://code.visualstudio.com/) with [PlatformIO IDE](https://marketplace.visualstudio.com/items?itemName=platformio.platformio-ide) extension.

---

## 📂 Projects in This Folder

| Project                           | Description               | Features                           |
|-----------------------------------|---------------------------|------------------------------------|
| [**invaders**](invaders/)         | Space Invaders game       | Graphics, gameplay, display output |
| [**radiodisplay**](radiodisplay/) | Wireless display receiver | NRF24L01, OLED, wireless protocol  |
| [**servo**](servo/)               | Servo motor control       | PWM, precise timing                |

---

## 🔧 Configuration & Programming

### Fuse Settings

Use [Engbedded Fuse Calculator](https://www.engbedded.com/fusecalc/) to calculate your fuse values.

**Read current fuse settings:**

```bash
avrdude -c usbtiny -p m1284p -U lfuse:r:-:h -U hfuse:r:-:h
```

**Set to 8MHz internal clock (64ms startup):**

```bash
avrdude -c usbtiny -p m1284p -U lfuse:w:0xe2:m -U hfuse:w:0x99:m
```

### Memory Operations

**Read EEPROM and save to file:**

```bash
avrdude -c usbtiny -p m1284p -U eeprom:r:eeprom.hex:i
```

**Write firmware to flash:**

```bash
avrdude -c usbtiny -p m1284p -U flash:w:firmware.hex:i
```

**Chip erase before flashing:**

```bash
avrdude -c usbtiny -p m1284p -e -U flash:w:firmware.hex:i
```

---

## 📦 Building & Uploading

### With PlatformIO

```bash
cd <project-name>      # e.g., cd invaders
pio run                # Build
pio run -t upload      # Upload to board
pio device monitor     # View serial output
```

---

## 🛠️ Development Tips

- **Max frequency:** 20 MHz at 5V
- **Low power modes:** Sleep, idle, ADC noise reduction
- **Flash:** 10,000 write/erase cycles guaranteed
- **EEPROM:** 100,000 write/erase cycles guaranteed
- **Debugging:** Use simavr simulation or JTAG debugger

---

## 📚 Common Libraries

See [../common/readme.md](../common/readme.md) for library documentation:

- **SPI** - High-speed serial communication (displays, wireless)
- **I2C** - Two-wire bus (sensors, displays)
- **NRF24L01** - 2.4GHz wireless modules
- **OLED** - SSD1306 display drivers
- **Timers** - PWM, CTC, frequency measurement

---

## 🔗 Resources

- [PlatformIO Documentation](https://docs.platformio.org/)
- [AVRDude Manual](https://www.nongnu.org/avrdude/)
