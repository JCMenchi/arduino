# Projects based on ATmega8535

![ATmega8535 pin layout](./ATmega8535.png "ATmega8535 pin layout DIP40").

## Set up development environment

On ubuntu install AVR toolchain

```bash
apt install avrdude gcc-avr gcc-doc avr-libc
```

To develop with [vscode](https://code.visualstudio.com/) add [PlatformIO](https://platformio.org
) vsix [extension](https://marketplace.visualstudio.com/items?itemName=platformio.platformio-ide).

## MCU configuration

### FUSE settings

To calculate fuse mask go to <https://www.engbedded.com/fusecalc/>

To read current fuse settings

```bash
avrdude -c usbtiny -p m8535 -U lfuse:r:-:h -U hfuse:r:-:h
```

To write fuse
Following command sets the clock to 8MHz internal (slow startup 64ms)

```bash
avrdude -c usbtiny -p m8535 -U lfuse:w:0xe4:m -U hfuse:w:0xd9:m
```

### Read/Write memory

Read EEPROM and dump on stdout (-) in intel hex format

```bash
avrdude -c usbtiny -p m8535 -U eeprom:r:-:i 
```

Write firmware.hex to flash with [AVR pocket programmer](https://www.sparkfun.com/products/9825)

```bash
avrdude -c usbtiny -p m8535 -U flash:w:firmware.hex:i 
```
