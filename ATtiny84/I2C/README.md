# ATtiny84 Software I2C & OLED Nunchuk Project

A lightweight ATtiny84-based firmware that interfaces with a Nintendo Wii Nunchuk controller and an SSD1306 OLED screen over a software-implemented (bit-bang) I2C bus. It includes an interrupt-driven software serial port (INT0) for debugging.

---

## Features

- **Interrupt-Driven Software Serial:** Receives commands on `PB2` (INT0) and transmits telemetry on `PA7` at the configured baud rate using the `avrtools` software serial module.
- **Wii Nunchuk Integration:** Reads 2-axis joystick coordinates, and button (`C`/`Z`) states over I2C.
- **SSD1306 OLED Support (128x32):** Displays real-time joystick coordinates and button presses directly onto the screen.
- **Heartbeat Status LED:** Visual indicator blinking on `PA0` to verify system health and stable execution loops.
- **Ultra-Lightweight Footprint:** Built directly against `avr-libc` with PlatformIO, using less than 65% of the ATtiny84's 8KB Flash.

---

## Pin Assignments

| ATtiny84 Pin | Port  | Function           | Connected To                            |
|:------------:|:-----:|--------------------|-----------------------------------------|
| **Pin 1**    | `VCC` | Power 5V           | 5V Power Supply                         |
| **Pin 2**    | `PB0` | Software I2C SCL   | Nunchuk & OLED SCL (with 3.3kΩ pull-up) |
| **Pin 3**    | `PB1` | Software I2C SDA   | Nunchuk & OLED SDA (with 3.3kΩ pull-up) |
| **Pin 4**    | `PB3` | Hardware `RESET`   | USB Programmer or GND                   |
| **Pin 5**    | `PB2` | INT0 / Serial RX   | Serial Adapter TX                       |
| **Pin 6**    | `PA7` | Software Serial TX | Serial Adapter RX                       |
| **Pin 7**    | `PA6` | MOSI               | Programmer MOSI                         |
| **Pin 8**    | `PA5` | MISO               | Programmer MISO                         |
| **Pin 9**    | `PA4` | SCK                | Programmer SCK                          |
| **Pin 10**   | `PA3` | *Unused*           | N/A                                     |
| **Pin 11**   | `PA2` | *Unused*           | N/A                                     |
| **Pin 12**   | `PA1` | *Unused*           | N/A                                     |
| **Pin 13**   | `PA0` | Heartbeat LED      | LED anode (via 330Ω resistor to GND)    |
| **Pin 14**   | `GND` | Common Ground      | System Ground                           |

---

## Building and Flashing

This project utilizes [PlatformIO](https://platformio.org/).

### Command Reference

```bash
# Compile the firmware
pio run

# Upload the firmware to the ATtiny84 using the configured usbtiny programmer
pio run -t upload

# Launch the serial monitor to view debugging logs and telemetry
pio monitor device --echo
```

*Note: The upload protocol in `platformio.ini` is set to `usbtiny` by default. You can easily switch to `usbasp` or another programmer by editing `platformio.ini`.*

---

## Internal Libraries

This repository contains modular custom libraries under the `lib/` directory, optimized specifically for low-resource AVR microcontrollers:

| Library        | Directory                                  | Description / Purpose                                                                                                                     |
|----------------|--------------------------------------------|-------------------------------------------------------------------------------------------------------------------------------------------|
| **`avrtools`** | [`lib/avrtools`](./lib/avrtools/readme.md) | Low-level GPIO register-mapped macros, </br>precise interrupt-driven millisecond timer (`millisec`), and software serial (`int0_serial`). |
| **`i2c`**      | [`lib/i2c`](./lib/i2c/readme.md)           | Lightweight software-based (bit-bang) I2C master driver </br>with automatic SCL clock-stretching support.                                 |
| **`nunchuk`**  | [`lib/nunchuk`](./lib/nunchuk/readme.md)   | Nintendo Wii Nunchuk controller interface driver, </br>handling calibration, data updates, and decoding.                                  |
| **`oled`**     | [`lib/oled`](./lib/oled/readme.md)         | High-efficiency monochrome OLED display driver </br>supporting SSD1306, CH1115, and SH1107 controllers.                                   |

---

## System Behavior & Usage

Once the firmware is running on the ATtiny84:

### 1. Initialization Phase

- The device pauses for **1500ms** to allow the I2C peripherals (Nunchuk and OLED) to initialize and stabilize.
- Welcomes the user by printing `"Welcome ATtiny84\n"` over the software serial port.
- Initializes the Wii Nunchuk. Success prints `"Nunchuk initialized.\n"`, while failure reports `"Nunchuk not found.\n"`.
- Initializes the SSD1306 OLED screen (128x32), configures the orientation, and draws the static templates: `Pos(X,Y):` and `Button:`.

### 2. Main Run Loop (Every 250ms)

- Toggles the state of the heartbeat LED connected to `PA0` (blinks every 250ms).
- Updates sensor readings from the Wii Nunchuk.
- Redraws the real-time joystick coordinates (`X` and `Y`) and button presses onto the OLED screen:
  - Displays **`C`** on the screen if the C button is held down.
  - Displays **`Z`** on the screen if the Z button is held down.
- **Serial Telemetry Trigger:** Holding the **`Z`** button down on the Nunchuk triggers a detailed report over the serial link, printing joystick coordinates, 3-axis accelerometer values, and button states.

### 3. Remote Command Handler

- The device actively listens for incoming command strings on `PB2` (using the INT0 pin interrupt).
- Upon receiving a line-terminated command:
  1. Echoes `"Received command: <received_string>\n"` back to the serial console.
  2. Immediately polls the Wii Nunchuk and dumps a full telemetry status report to the software serial port.
