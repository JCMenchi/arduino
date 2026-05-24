# ATmega8535 Microcontroller Projects

The ATmega8535 is a powerful yet compact AVR microcontroller with 8KB Flash and 512B SRAM, ideal for sensor interfacing, displays, and mid-range robotics applications.

## 📊 Specifications

| Feature          | Value                |
|------------------|----------------------|
| **Flash Memory** | 8 KB                 |
| **SRAM**         | 512 B                |
| **EEPROM**       | 512 B                |
| **Pins (DIP)**   | 40                   |
| **Max Clock**    | 16 MHz               |
| **Timers**       | 3 (16-bit & 8-bit)   |
| **PWM Channels** | 4                    |
| **ADC Channels** | 8 (10-bit)           |
| **I/O Ports**    | A, B, C, D (32 pins) |

![ATmega8535 pin layout](./ATmega8535-pinout.png "ATmega8535 pin layout DIP40")

**📄 [Full Datasheet](./ATmega8535-datasheet.pdf)**

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
| [**oled**](oled/)                 | OLED display controller   | I2C communication, SSD1306         |
| [**radiodisplay**](radiodisplay/) | Wireless display system   | NRF24L01, OLED, wireless           |
| [**tank**](tank/)                 | Mobile robot platform     | Motor control, sensors, simulation |
| [**SPI**](SPI/)                   | SPI communication demo    | High-speed serial protocol         |
| [**NRF24L01**](NRF24L01/)         | Wireless module interface | 2.4GHz communication               |

---

## 🔧 Configuration & Programming

### Fuse Settings

Use [Engbedded Fuse Calculator](https://www.engbedded.com/fusecalc/) to calculate your fuse values.

**Read current fuse settings:**

```bash
avrdude -c usbtiny -p m8535 -U lfuse:r:-:h -U hfuse:r:-:h
```

**Set to 8MHz internal clock (64ms startup):**

```bash
avrdude -c usbtiny -p m8535 -U lfuse:w:0xe4:m -U hfuse:w:0xd9:m
```

### Memory Operations

**Read EEPROM and save to file:**

```bash
avrdude -c usbtiny -p m8535 -U eeprom:r:eeprom.hex:i
```

**Write firmware to flash:**

```bash
avrdude -c usbtiny -p m8535 -U flash:w:firmware.hex:i
```

**Chip erase before flashing:**

```bash
avrdude -c usbtiny -p m8535 -e -U flash:w:firmware.hex:i
```

---

## 📦 Building & Uploading

### With PlatformIO

```bash
cd <project-name>      # e.g., cd oled
pio run                # Build
pio run -t upload      # Upload to board
pio device monitor     # View serial output
```

---

## 💾 Memory Management

- **Total Flash:** 8 KB for code
- **SRAM:** 512 B - Plan stack and data carefully
- **EEPROM:** 512 B - Persistent storage
- **Flash cycles:** 10,000 guaranteed write/erase cycles
- **EEPROM cycles:** 100,000 guaranteed write/erase cycles

**Tip:** Use `const` and `PROGMEM` for constant data to save SRAM.

---

## 📚 Common Libraries

See [../common/README.md](../common/readme.md) for library documentation:

- **I2C** - Two-wire interface for OLED, sensors
- **SPI** - High-speed serial for displays, wireless
- **NRF24L01** - 2.4GHz wireless communication
- **OLED (SSD1306)** - Display driver via I2C or SPI
- **HC-SR04** - Ultrasonic distance measurement

---

## 🛠️ Development Tips

- **Frequency:** 16 MHz at 5V (safe operation)
- **Power modes:** Multiple sleep modes for low-power applications
- **PWM:** 4 PWM channels for motor/LED control
- **Interrupts:** External interrupts on INT0/INT1
- **Simulation:** simavr available for tank project

---

## 🔗 Resources

- [PlatformIO Documentation](https://docs.platformio.org/)
- [AVRDude Manual](https://www.nongnu.org/avrdude/)
