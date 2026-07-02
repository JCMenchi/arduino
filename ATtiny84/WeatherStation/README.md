# ATtiny84 Weather Station

This project is a compact weather station built for the ATtiny84 MCU.
It reads temperature, humidity, and pressure from a BME280 sensor, displays the current/min/max values on an OLED display, and preserves history in EEPROM.

## Features

- ATtiny84-based weather station
- BME280 sensor support for temperature, pressure, and humidity
- OLED display support via `SSD1306Display`
- Current / minimum / maximum values displayed on-screen
- EEPROM persistence for min/max sensor history
- Optional INT0 serial command interface for debugging and EEPROM inspection

## Project Structure

- `platformio.ini` - PlatformIO environment configuration for ATtiny84
- `src/main.cpp` - Weather station application logic
- `src/eeprom_utils.h` - EEPROM layout and initialization helpers
- `lib/` - Included AVR, I2C, BME280, and OLED support libraries
- `include/README` - Shared include documentation for the workspace
- `test/README` - Test-related notes

## Hardware

Required components:

- ATtiny84 microcontroller
- BME280 environmental sensor
- OLED display compatible with the included driver (SSD1306/CH1115 style)
- 3.3V or 5V power supply depending on your sensor and display modules
- Programmer compatible with PlatformIO (USBtiny, USBasp, etc.)

Typical I2C wiring for ATtiny84:

- SDA -> PA5 or PA7 (software I2C)
- SCL -> PA4 or PA6 (software I2C)

Additional connections:

- LED indicator on PA0
- Optional INT0 serial on PA7 if `HAS_INT0_SERIAL` is enabled

## Build and Upload

The project uses PlatformIO with the `attiny84` environment.

Build the firmware:

```sh
platformio run -e attiny84
```

Upload using your programmer:

```sh
platformio run -e attiny84 -t upload
```

The current `platformio.ini` defines:

- `platform = atmelavr`
- `board = attiny84`
- `build_flags = -DSOFTWARE_I2C -DHAS_INT0 -DNRF24_DEBUG -DHAS_INT0_SERIAL -DINT0_SERIAL_TRANSMIT_PORT=A`

Adjust `upload_port` or `upload_protocol` in `platformio.ini` to match your programmer.

## Runtime Behavior

On startup, the firmware:

1. Initializes EEPROM and optionally formats it when the weather data marker is missing
2. Initializes the OLED display at I2C address `0x20`
3. Initializes the BME280 sensor via `BME280_begin()`
4. Runs a few warm-up sensor reads before entering the main loop

In the main loop:

- The LED on `PA0` blinks while reading sensor values
- Current temperature, pressure, and humidity are displayed
- Min/max values are updated and stored in EEPROM
- The loop waits 20 seconds between reads

## Display Format

The OLED shows three columns for each measurement:

- `Cur` — current reading
- `Min` — recorded minimum
- `Max` — recorded maximum

Measurement units:

- Temperature is shown with one decimal digit and a `C` suffix
- Humidity is shown with one decimal digit and a `%` suffix
- Pressure is shown with an `h` suffix to represent hPa-style readings

## EEPROM Layout

The EEPROM layout is defined in `src/eeprom_utils.h`.

- `EEPROM_WEATHER_TYPE` — marker used to identify valid weather data
- `EEPROM_WEATHER_SIZE` — allocated data size for min/max readings
- `EEPROM_DATA_START` — offset where weather values begin
- Min/max offsets for temperature, pressure, humidity

When the marker is missing or invalid, the firmware formats EEPROM and resets history values.

## Serial Commands (Optional)

If `HAS_INT0_SERIAL` is enabled, the firmware supports simple INT0 serial commands over the configured transmit pin.

Supported commands:

- `read` — prints EEPROM marker plus stored min/max values
- `reset` — clears EEPROM marker and resets min/max history
