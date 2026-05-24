# AVRTools Library - User Manual

## Table of Contents

- [Overview](#overview)
- [Complete Application Examples](#complete-application-examples)
  - [Example 1: LED Blink](#example-1-led-blink)
  - [Example 2: Serial Echo](#example-2-serial-echo)
  - [Example 3: PWM LED Fading](#example-3-pwm-led-fading)
  - [Example 4: Button Debounce with Serial](#example-4-button-debounce-with-serial)
- [Common Patterns](#common-patterns)
  - [Debouncing Input](#debouncing-input)
  - [Periodic Task Scheduling](#periodic-task-scheduling)
  - [Serial Command Parsing](#serial-command-parsing)
- [Performance Considerations](#performance-considerations)
  - [Memory Usage](#memory-usage)
- [Device Compatibility](#device-compatibility)
- [References](#references)

---

## Overview

The AVRTools library is a comprehensive collection of utilities and drivers for AVR microcontrollers (such as ATtiny and ATmega devices). It provides simplified interfaces to common hardware features and functionality, including:

- [**GPIO Control**](#1-gpio-control-gpioh) - Low-level pin manipulation with macros
- [**Timing**](#2-millisecond-timer-millisech) - Millisecond counter with interrupt-based accuracy
- [**PWM**](#3-pwm-control-pwmh) - Pulse Width Modulation on various timer channels
- [**Serial Communication**](#4-software-serial-via-int0-int0_serialh) - Both hardware (USART) and software (INT0 bit-bang) options
- [**Sound Generation**](#6-soundtone-generation-soundh) - Tone output via PWM
- [**Framework**](#7-arduino-style-main-maincpph) - Arduino-style setup/loop pattern

The library abstracts hardware complexity while maintaining direct control and minimal overhead, it focus only on AVR ATmega and ATtiny devices.

---

## Module Reference

### 1. GPIO Control (gpio.h)

GPIO macros provide efficient, direct control of I/O pins without function call overhead.

#### Pin Configuration Macros

##### `GPIO_OUTPUT(PORTID, BITNUM)`

Configure a pin as output (sink or source current).

**Parameters:**

- `PORTID`: Port letter (A, B, C, etc.) without quotes
- `BITNUM`: Bit number 0-7

**Example:**

```cpp
GPIO_OUTPUT(A, 0);  // Configure PA0 as output
GPIO_OUTPUT(B, 5);  // Configure PB5 as output
```

---

##### `GPIO_INPUT(PORTID, BITNUM)`

Configure a pin as input (high impedance, floating).

**Example:**

```cpp
GPIO_INPUT(A, 1);  // Configure PA1 as input
```

---

##### `GPIO_INPUT_PULLUP(PORTID, BITNUM)`

Configure a pin as input with internal pull-up resistor enabled.

**Example:**

```cpp
GPIO_INPUT_PULLUP(A, 1);  // PA1 as input with pull-up
```

---

##### `GPIO_INPUT_PULLDOWN(PORTID, BITNUM)`

Configure a pin as input with internal pull-down resistor enabled.

**Example:**

```cpp
GPIO_INPUT_PULLDOWN(A, 1);  // PA1 as input with pull-down
```

---

#### Pin Control Macros

##### `GPIO_SET_HIGH(PORTID, BITNUM)`

Set a pin to HIGH logic level (1).

**Example:**

```cpp
GPIO_SET_HIGH(A, 0);  // PA0 = HIGH (5V or 3.3V depending on device)
```

---

##### `GPIO_SET_LOW(PORTID, BITNUM)`

Set a pin to LOW logic level (0).

**Example:**

```cpp
GPIO_SET_LOW(A, 0);  // PA0 = LOW (0V)
```

---

#### Pin Reading Macros

##### `GPIO_READ(PORTID, BITNUM)`

Read the current state of a pin and return `GPIO_HIGH` or `GPIO_LOW`.

**Returns:** `GPIO_HIGH` (1) or `GPIO_LOW` (0)

**Example:**

```cpp
uint8_t state = GPIO_READ(A, 1);
if (state == GPIO_HIGH) {
    // Pin is high
}
```

---

##### `GPIO_IS_HIGH(PORTID, BITNUM)`

Check if a pin is at HIGH logic level.

**Returns:** true if pin is high, false if low

**Example:**

```cpp
if (GPIO_IS_HIGH(A, 1)) {
    // Pin is high
}
```

---

##### `GPIO_IS_LOW(PORTID, BITNUM)`

Check if a pin is at LOW logic level.

**Returns:** true if pin is low, false if high

**Example:**

```cpp
if (GPIO_IS_LOW(A, 1)) {
    // Pin is low
}
```

---

#### GPIO Example

```cpp
#include <gpio.h>

int main() {
    // Configure PA0 as output, PA1 as input with pull-up
    GPIO_OUTPUT(A, 0);
    GPIO_INPUT_PULLUP(A, 1);
    
    // Main loop
    while(1) {
        // Read input and drive output
        if (GPIO_IS_HIGH(A, 1)) {
            GPIO_SET_HIGH(A, 0);  // Echo input to output
        } else {
            GPIO_SET_LOW(A, 0);
        }
    }
    
    return 0;
}
```

---

### 2. Millisecond Timer (millisec.h)

Provides interrupt-based millisecond counting for timing and delays.

#### Functions

##### `void init_timer()`

Initialize Timer0 for millisecond counting with 1ms resolution.
This timer is always created when using the Arduino style [mainloop](./src/main.cpp.h),
which means that Timer0 cannot be used for other purpose.

**Notes:**

- Must be called once during system initialization
- Configures Timer0 and enables its interrupt
- Global interrupts must be enabled after calling this
- Automatically setup by Arduino style main loop

**Example:**

```cpp
void setup() {
    init_timer();
    sei();  // Enable global interrupts
}
```

---

##### `uint32_t milliseconds()`

Get elapsed time in milliseconds since `init_timer()` was called.

**Returns:** 32-bit unsigned millisecond counter

**Notes:**

- Counter overflows after ~49.7 days (2³² milliseconds)
- Wraps around to 0 automatically
- Non-blocking operation

**Example:**

```cpp
uint32_t start = milliseconds();

// Do something...

uint32_t elapsed = milliseconds() - start;
if (elapsed > 1000) {
    // More than 1 second passed
}
```

---

#### Timer Example

```cpp
#include <millisec.h>
#include <avr/interrupt.h>
#include <gpio.h>

void setup() {
    init_timer();
    sei();  // Enable interrupts
    GPIO_OUTPUT(A, 0);
}

void loop() {
    static uint32_t lastBlink = 0;
    uint32_t now = milliseconds();
    
    if (now - lastBlink > 500) {
        lastBlink = now;
        GPIO_SET_HIGH(A, 0);  // Toggle LED every 500ms
    }
    
    if (now - lastBlink > 250) {
        GPIO_SET_LOW(A, 0);
    }
}

int main() {
    setup();
    while(1) {
        loop();
    }
    return 0;
}
```

---

### 3. PWM Control (pwm.h)

Provides Pulse Width Modulation functionality for various timer channels.

#### Channel Constants

Available PWM channels depend on the microcontroller:

**ATtiny45:**

- `PWM_OC0B` - Timer0 Channel B
- `PWM_OC1A` - Timer1 Channel A
- `PWM_OC1B` - Timer1 Channel B

**ATmega328P:**

- `PWM_OC0B` - Timer0 Channel B
- `PWM_OC1A` - Timer1 Channel A
- `PWM_OC1B` - Timer1 Channel B
- `PWM_OC2A` - Timer2 Channel A
- `PWM_OC2B` - Timer2 Channel B

**ATmega1284P:**

- `PWM_OC0B` - Timer0 Channel B
- `PWM_OC1A` - Timer1 Channel A
- `PWM_OC1B` - Timer1 Channel B
- `PWM_OC2A` - Timer2 Channel A
- `PWM_OC2B` - Timer2 Channel B
- `PWM_OC3A` - Timer3 Channel A
- `PWM_OC3B` - Timer3 Channel B

**Note:** `PWM_OC0A` is reserved for millisecond timing and not available.

---

#### Standard PWM Functions

##### `void enablePWM(uint8_t pwm_pin)`

Enable standard PWM on the specified timer channel.

**Parameters:**

- `pwm_pin`: Channel constant (PWM_OC1A, PWM_OC2B, etc.)

**Notes:**

- 8-bit resolution (0-255)
- Sets up timer in PWM mode
- Pin becomes an output

**Example:**

```cpp
enablePWM(PWM_OC1A);  // Enable PWM on Timer1 Channel A
```

---

##### `void setPWM(uint8_t pwm_pin, uint8_t value)`

Set the duty cycle for standard PWM.

**Parameters:**

- `pwm_pin`: Channel constant
- `value`: Duty cycle (0-255)
  - 0 = 0% (always off)
  - 128 = 50% (50% on, 50% off)
  - 255 = 100% (always on)

**Example:**

```cpp
setPWM(PWM_OC1A, 128);  // 50% duty cycle
setPWM(PWM_OC1A, 200);  // ~78% duty cycle
setPWM(PWM_OC1A, 0);    // Off
```

---

#### Servo PWM Functions

##### `void enableServoPWM(uint8_t pwm_pin)`

Enable servo-compatible PWM (20ms period) on the specified channel.

**Parameters:**

- `pwm_pin`: Must be a 16-bit timer (PWM_OC1A, PWM_OC1B, PWM_OC3A, or PWM_OC3B)

**Notes:**

- Uses 20ms period (standard servo frequency)
- Variable pulse width: typically 1-2ms for servo control
- Requires 16-bit timer (8-bit timers won't work for servo)

**Example:**

```cpp
enableServoPWM(PWM_OC1A);  // Enable servo PWM on Timer1A
```

---

##### `void setServoPWM(uint8_t pwm_pin, uint16_t value)`

Set the pulse width for servo PWM.

**Parameters:**

- `pwm_pin`: Channel constant
- `value`: Pulse width in timer ticks
  - Typical range: 1000-2000 (1ms-2ms at 16MHz)
  - Maps to servo positions: ~90° range

**Notes:**

- Exact range depends on clock frequency and prescaler
- 16-bit value allows fine-grained control

**Example:**

```cpp
setServoPWM(PWM_OC1A, 1500);  // Center servo position
setServoPWM(PWM_OC1A, 1000);  // Minimum (typically -45°)
setServoPWM(PWM_OC1A, 2000);  // Maximum (typically +45°)
```

---

#### PWM Example

```cpp
#include <pwm.h>
#include <millisec.h>

void setup() {
    init_timer();
    enablePWM(PWM_OC1A);      // Enable standard PWM
    enableServoPWM(PWM_OC1B); // Enable servo PWM
}

void loop() {
    static uint32_t lastUpdate = 0;
    uint32_t now = milliseconds();
    
    if (now - lastUpdate > 10) {
        lastUpdate = now;
        
        // Sweep standard PWM 0-255
        static uint8_t duty = 0;
        setPWM(PWM_OC1A, duty++);
        
        // Sweep servo 1000-2000
        static uint16_t servo = 1000;
        servo += 10;
        if (servo > 2000) servo = 1000;
        setServoPWM(PWM_OC1B, servo);
    }
}

int main() {
    setup();
    while(1) loop();
}
```

---

### 4. Software Serial via INT0 (int0_serial.h)

Software-based serial communication using INT0 interrupt for receive and bit-banging for transmit.

#### Compile time configuration

| Macro                     | Default | Description                                                        |
|---------------------------|---------|--------------------------------------------------------------------|
| INT0_SERIAL_TRANSMIT_PORT | B       | Letter of port used for transmit pin</br>INT0 is usually on PORT B |
| INT0_SERIAL_CMD_BUF_SIZE  | 32      | Max size of receive buffer</br>Make it small to save RAM           |

#### Initialization

##### `void INT0_Init(uint8_t tpin, volatile void (*INT0_rec_cb)(uint8_t))`

Initialize INT0 serial communication at 9600 baud.

**Parameters:**

- `tpin`: Transmit pin bit number (0-7) on INT0_SERIAL_TRANSMIT_PORT
- `INT0_rec_cb`: Callback function for received bytes

**Notes:**

- Fixed 9600 baud rate (this is the only supported baud rate)
- Requires proper timer configuration
- Callback is called from interrupt context

**Example:**

```cpp
void serialRxCallback(uint8_t data) {
    // Handle received byte
}

int main() {
    INT0_Init(PB1, serialRxCallback); // transmit on PB1, receive on INT0 pin check datasheet
    sei();  // Enable interrupts
    
    // Main code...
}
```

---

#### Transmission Functions

##### `uint8_t INT0_Transmit(uint8_t data)`

Transmit a single byte via INT0 serial.

**Parameters:**

- `data`: Byte to send

**Returns:** 1 if successful, 0 if error

**Example:**

```cpp
INT0_Transmit('A');  // Send character 'A'
INT0_Transmit(0x0D); // Send carriage return
```

---

##### `void INT0_WriteString(const char *str)`

Write a null-terminated string from RAM.

**Example:**

```cpp
INT0_WriteString("Hello World\n");
```

---

##### `void INT0_WritePString(const char *str)`

Write a null-terminated string from program memory (PROGMEM).

**Example:**

```cpp
INT0_WritePString(PSTR("Hello from PROGMEM\n"));
```

---

##### `void INT0_WriteInt(int32_t i, uint8_t base = 10)`

Write a signed 32-bit integer.

**Parameters:**

- `i`: Integer value
- `base`: Number base (10=decimal, 16=hexadecimal, 2=binary, etc.)

**Example:**

```cpp
INT0_WriteInt(-1234);        // Decimal: -1234
INT0_WriteInt(255, 16);      // Hexadecimal: FF
INT0_WriteInt(7, 2);         // Binary: 111
```

---

##### `void INT0_WriteUInt(uint32_t i, uint8_t base = 10)`

Write an unsigned 32-bit integer.

**Example:**

```cpp
INT0_WriteUInt(4294967295);  // Unsigned decimal
INT0_WriteUInt(255, 16);     // Hexadecimal: FF
```

---

##### `void INT0_WriteChar(char d)`

Write a single character.

**Example:**

```cpp
INT0_WriteChar('X');
INT0_WriteChar('\n');
```

---

##### `void INT0_WriteFloat(float d, uint8_t width = 11, uint8_t prec = 2)`

Write a floating-point number.

**Parameters:**

- `d`: Float value
- `width`: Field width (default: 11)
- `prec`: Decimal precision (default: 2)

**Example:**

```cpp
INT0_WriteFloat(3.14159);       // Outputs: "3.14"
INT0_WriteFloat(123.456, 8, 3); // Outputs: "123.456"
```

---

#### Command Buffering

##### `INT0_SerialCommandMgr::hasCommand()`

Check if a complete command line is available.

**Returns:** 1 if command ready (line received with \n), 0 otherwise

**Example:**

```cpp
if (INT0_SerialCommandMgr::hasCommand()) {
    const char *cmd = INT0_SerialCommandMgr::command();
    // Process command...
}
```

---

##### `INT0_SerialCommandMgr::command()`

Retrieve and consume the current command line.

**Returns:** Pointer to command string, or NULL if no command available

**Note:**

- Returns pointer to static buffer (copy if needed). Buffer may be changed by INT0 interrupt.
- Resets command buffer state after return
- Buffer is null-terminated
- Buffer size is defined by macro INT0_SERIAL_CMD_BUF_SIZE (default size 32)

**Example:**

```cpp
const char *cmd = INT0_SerialCommandMgr::command();
if (cmd && cmd[0] == 'S') {
    // Command starts with 'S'
}
```

---

##### `INT0_SerialCommandMgr::peekCommand()`

View the current command buffer without consuming it.

**Returns:** Pointer to command buffer (may not be null-terminated if incomplete)

**Example:**

```cpp
const char *cmd = INT0_SerialCommandMgr::peekCommand();
// Inspect command without consuming it
```

---

#### INT0 Serial Example

```cpp
#include <int0_serial.h>
#include <avr/interrupt.h>

void serialRxCallback(uint8_t data) {
    // Called when byte received
}

void setup() {
    INT0_Init(PB1, serialRxCallback);
    sei();  // Enable interrupts
}

void loop() {
    if (INT0_SerialCommandMgr::hasCommand()) {
        const char *cmd = INT0_SerialCommandMgr::command();
        
        INT0_WritePString(PSTR("Received: "));
        INT0_WriteString(cmd);
        INT0_WriteChar('\n');
    }
}

int main() {
    setup();
    while(1) loop();
}
```

---

### 5. Hardware USART Serial (usart_serial.h)

Hardware-based serial communication using USART peripheral.

#### USART Compile time configuration

| Macro                | Default | Description                                                        |
|----------------------|---------|--------------------------------------------------------------------|
| SERIAL_CMD_BUF_SIZE  | 32      | Max size of receive buffer</br>Make it small to save RAM           |

#### USART Initialization

##### `void USART_Init(uint8_t baudrate, volatile void (*usart_rec_cb)(uint8_t, bool))`

Initialize USART serial communication.

**Parameters:**

- `baudrate`: Baud rate selection
  - `BAUD_RATE_9600` - 9600 bps
  - `BAUD_RATE_57600` - 57600 bps
  - `BAUD_RATE_115200` - 115200 bps
- `usart_rec_cb`: Callback function for received bytes
  - Signature: `void callback(uint8_t data, bool error)`
  - `error` = true if framing or overrun error occurred

**Example:**

```cpp
void serialRxCallback(uint8_t data, bool error) {
    if (!error) {
        // Valid byte received
    } else {
        // Error occurred (framing, overrun, etc.)
    }
}

USART_Init(BAUD_RATE_115200, serialRxCallback);
```

---

#### USART Transmission Functions

##### `void USART_Transmit(uint8_t data)`

Transmit a single byte via USART.

**Example:**

```cpp
USART_Transmit('A');  // Send character
USART_Transmit(0xFF); // Send byte
```

---

##### `uint8_t USART_Receive()`

Receive a single byte via USART (blocking).

**Returns:** Received byte

**Notes:**

- Blocks until data is available
- Not typically used with interrupt-based receive

**Example:**

```cpp
uint8_t byte = USART_Receive();  // Wait for byte
```

---

##### `void USART_WriteString(const char *str)`

Write a null-terminated string from RAM.

**Example:**

```cpp
USART_WriteString("Hello World\n");
```

---

##### `void USART_WritePString(const char* str)`

Write a null-terminated string from program memory.

**Example:**

```cpp
USART_WritePString(PSTR("Greeting from PROGMEM\n"));
```

---

##### `void USART_WriteInt(int32_t i, uint8_t base = 10)`

Write a signed 32-bit integer.

**Example:**

```cpp
USART_WriteInt(-12345);      // Decimal
USART_WriteInt(255, 16);     // Hexadecimal
```

---

##### `void USART_WriteUInt(uint32_t i, uint8_t base = 10)`

Write an unsigned 32-bit integer.

**Example:**

```cpp
USART_WriteUInt(65535);      // Decimal
USART_WriteUInt(0xDEADBEEF, 16);  // Hexadecimal
```

---

##### `void USART_WriteChar(char d)`

Write a single character.

**Example:**

```cpp
USART_WriteChar('X');
USART_WriteChar('\n');
```

---

#### USART Example

```cpp
#include <usart_serial.h>
#include <avr/interrupt.h>

void serialRxCallback(uint8_t data, bool error) {
    if (!error) {
        USART_Transmit(data);  // Echo received byte
    }
}

void setup() {
    USART_Init(BAUD_RATE_115200, serialRxCallback);
    sei();  // Enable interrupts
}

void loop() {
    USART_WritePString(PSTR("Sending data...\n"));
    USART_WriteUInt(12345);
    USART_WriteChar('\n');
}

int main() {
    setup();
    while(1) loop();
}
```

---

### 6. Sound/Tone Generation (sound.h)

Generate tones and melodies via PWM. See [Sound Library Documentation](sound.h) for details.

**Quick Start:**

```cpp
#include <sound.h>

void setup() {
    // Play a short startup sound
    start_sound();  // Plays B5 then E6
}

void loop() {
    // Play a 1-second tone at 1000 Hz
    playNote(&PORTB, &DDRB, PORTB0, 1000, 1000);
}
```

---

### 7. Arduino-Style Main (main.cpp.h)

Provides the standard Arduino framework pattern with `setup()` and `loop()` functions.

#### Required User Functions

##### `void setup()`

Called once at startup for initialization.

```cpp
void setup() {
    GPIO_OUTPUT(A, 0);
    init_timer();
    sei();
}
```

---

##### `void loop()`

Called repeatedly in main loop.

```cpp
void loop() {
    GPIO_SET_HIGH(A, 0);
    // Main program logic
}
```

---

#### Main Loop Example

```cpp
#include <main.cpp.h>
#include <gpio.h>
#include <millisec.h>

void setup() {
    init_timer();
    GPIO_OUTPUT(A, 0);
}

void loop() {
    static uint32_t lastToggle = 0;
    uint32_t now = milliseconds();
    
    if (now - lastToggle > 500) {
        lastToggle = now;
        GPIO_SET_HIGH(A, 0);
    }
    
    if (now - lastToggle > 250) {
        GPIO_SET_LOW(A, 0);
    }
}

// Note: main() is automatically provided by main.cpp.h
```

---

## Complete Application Examples

### Example 1: LED Blink

```cpp
#include <main.cpp.h>
#include <gpio.h>
#include <millisec.h>

void setup() {
    init_timer();
    GPIO_OUTPUT(A, 0);  // LED on PA0
}

void loop() {
    static uint32_t lastToggle = 0;
    uint32_t now = milliseconds();
    
    if (now - lastToggle > 500) {
        lastToggle = now;
        GPIO_SET_HIGH(A, 0);
    }
    
    if (now - lastToggle > 250) {
        GPIO_SET_LOW(A, 0);
    }
}
```

---

### Example 2: Serial Echo

```cpp
#include <main.cpp.h>
#include <int0_serial.h>
#include <avr/interrupt.h>

void rxCallback(uint8_t data) {
    INT0_Transmit(data);  // Echo
}

void setup() {
    init_timer();
    INT0_Init(PB1, rxCallback);
    sei();
    INT0_WritePString(PSTR("INT0 Serial Ready\n"));
}

void loop() {
    if (INT0_SerialCommandMgr::hasCommand()) {
        const char *cmd = INT0_SerialCommandMgr::command();
        INT0_WritePString(PSTR("CMD: "));
        INT0_WriteString(cmd);
        INT0_WriteChar('\n');
    }
}
```

---

### Example 3: PWM LED Fading

```cpp
#include <main.cpp.h>
#include <pwm.h>
#include <millisec.h>

void setup() {
    init_timer();
    enablePWM(PWM_OC1A);
}

void loop() {
    static uint32_t lastUpdate = 0;
    uint32_t now = milliseconds();
    
    if (now - lastUpdate > 10) {
        lastUpdate = now;
        
        static uint8_t brightness = 0;
        static int8_t direction = 1;
        
        brightness += direction;
        
        if (brightness == 0 || brightness == 255) {
            direction = -direction;
        }
        
        setPWM(PWM_OC1A, brightness);
    }
}
```

---

### Example 4: Button Debounce with Serial

```cpp
#include <main.cpp.h>
#include <gpio.h>
#include <millisec.h>
#include <usart_serial.h>
#include <avr/interrupt.h>
#include <avr/pgmspace.h>

void rxCallback(uint8_t data, bool error) {
    // Handle received data if needed
}

void setup() {
    init_timer();
    GPIO_INPUT_PULLUP(A, 1);    // Button on PA1
    GPIO_OUTPUT(A, 0);           // LED on PA0
    USART_Init(BAUD_RATE_115200, rxCallback);
    sei();
    USART_WritePString(PSTR("Button Demo\n"));
}

void loop() {
    static uint32_t lastChange = 0;
    static uint8_t lastState = 1;
    
    uint8_t currentState = GPIO_READ(A, 1);
    uint32_t now = milliseconds();
    
    if (currentState != lastState && now - lastChange > 50) {
        lastState = currentState;
        lastChange = now;
        
        if (lastState == GPIO_LOW) {
            USART_WritePString(PSTR("Button pressed\n"));
            GPIO_SET_HIGH(A, 0);
        } else {
            USART_WritePString(PSTR("Button released\n"));
            GPIO_SET_LOW(A, 0);
        }
    }
}
```

---

## Common Patterns

### Debouncing Input

```cpp
uint8_t debounce(uint8_t pin, uint8_t port, uint32_t debounce_ms) {
    static uint32_t lastChange = 0;
    static uint8_t lastState = 0;
    
    uint8_t currentState = GPIO_READ(port, pin);
    uint32_t now = milliseconds();
    
    if (currentState != lastState && now - lastChange > debounce_ms) {
        lastState = currentState;
        lastChange = now;
        return 1;  // State changed after debounce
    }
    return 0;
}
```

---

### Periodic Task Scheduling

```cpp
void loop() {
    static uint32_t lastTask1 = 0;
    static uint32_t lastTask2 = 0;
    uint32_t now = milliseconds();
    
    // Task 1: Every 100ms
    if (now - lastTask1 > 100) {
        lastTask1 = now;
        task1();
    }
    
    // Task 2: Every 500ms
    if (now - lastTask2 > 500) {
        lastTask2 = now;
        task2();
    }
}
```

---

### Serial Command Parsing

```cpp
void process_command(const char *cmd) {
    if (cmd[0] == 'L' && cmd[1] == 'E' && cmd[2] == 'D') {
        uint8_t onoff = (cmd[3] == '1') ? 1 : 0;
        if (onoff) {
            GPIO_SET_HIGH(A, 0);
        } else {
            GPIO_SET_LOW(A, 0);
        }
    }
}

void loop() {
    if (INT0_SerialCommandMgr::hasCommand()) {
        const char *cmd = INT0_SerialCommandMgr::command();
        process_command(cmd);
    }
}
```

---

## Performance Considerations

### Memory Usage

- GPIO macros: No runtime memory
- Millisecond timer: 4 bytes (counter)
- Serial buffers: 32+ bytes (configurable)
- PWM: No additional memory

---

## Device Compatibility

| Feature      | ATtiny45 | ATtiny84 | ATmega8515 | ATmega8535 | ATmega328P(UNO) | ATmega1284P |
|--------------|----------|----------|------------|------------|-----------------|-------------|
| GPIO         | ✓        | ✓        | ✓          | ✓          | ✓               | ✓           |
| Timer        | ✓        | ✓        | ✓          | ✓          | ✓               | ✓           |
| PWM (8-bit)  | ✓        | ✓        | ✓          | ✓          | ✓               | ✓           |
| PWM (16-bit) | ✓        | Limited  | ✓          | ✓          | ✓               | ✓           |
| Servo PWM    | ✓        | Limited  | ✓          | ✓          | ✓               | ✓           |
| INT0 Serial  | ✓        | ✓        | ✓          | ✓          | ✓               | ✓           |
| USART        |          |          | ✓          | ✓          | ✓               | ✓           |
| Sound        | Limited  | Limited  | ✓          | ✓          | ✓               | ✓           |

---

## References

- ATtiny Datasheet: <http://ww1.microchip.com/>
- ATmega Datasheet: <http://ww1.microchip.com/>
- AVR Libc: <https://github.com/avrdudes/avr-libc/>
