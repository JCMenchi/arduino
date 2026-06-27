
#include <TinyI2CMaster.h>
#include <util/delay.h>

#ifdef HAS_SERIAL
#include <usart_serial.h>
#endif

#ifdef HAS_INT0_SERIAL
#include <int0_serial.h>
#include <avr/pgmspace.h>
#endif

#include "nunchuk.h"

/**
 * @file nunchuk.cpp
 * @brief Implementation of Wii Nunchuk controller driver.
 * 
 * Provides I2C communication with a Wii Nunchuk controller, including:
 * - Device initialization and verification
 * - Calibration data retrieval
 * - Periodic sensor data updates
 * - Joystick position analysis
 * - Debug output via serial port
 */

/**
 * @defgroup I2CProtocol I2C Protocol Constants
 * @brief Low-level I2C protocol constants for Nunchuk communication.
 * @{
 */

/** Typical calibration data for reference (not used by driver):
 *  
 *  Joystick calibration:
 *    min center max
 *  JX   26   128  228
 *  JY   31   128  222
 *  
 *  Accelerometer calibration:
 *         X   Y   Z
 *  Acc 0G 504 513 516
 *  Acc 1G 708 710 724
 */

/** I2C address of the Nunchuk device */
#define NUNCHUK_I2C_ID 0x52

/** I2C register address containing device identifier */
#define WII_DEVICE_REGISTER 0xFA

/** I2C register address for current position/button data */
#define WII_POSITION_REGISTER 0x00

/** I2C register address for calibration data */
#define WII_CALIBRATION_REGISTER 0x20

/** Device ID part 1: identifies the Nunchuk controller */
const uint32_t NUNCHUK_DEVICE_ID_PART1 = 0x0000A420;

/** Device ID part 2: second part of the device identifier */
const uint32_t NUNCHUK_DEVICE_ID_PART2 = 0x00000000;

/** Use unencrypted data mode (encrypted mode would require decoding) */
#define NOT_ENCRYPTED

/// @}

/**
 * @brief Initializes the Nunchuk controller.
 * 
 * Performs the following steps:
 * 1. Initializes the I2C interface
 * 2. Sends initialization sequence to put the Nunchuk in unencrypted mode
 * 3. Reads and verifies the device ID (must match NUNCHUK_DEVICE_ID_PART1/2)
 * 4. Retrieves calibration data from device memory
 * 5. Performs an initial data update
 * 
 * The unencrypted mode initialization sequence is:
 * - Write 0xF0 and 0x55 to register (init unencrypted mode)
 * - Write 0xFB and 0x00 to register (zero address)
 * 
 * If HAS_SERIAL is defined, debug messages are sent to serial output.
 * 
 * @return true if the device was successfully found and initialized, 
 *         false if I2C communication failed or device ID mismatch
 * 
 * @see get_calibration()
 * @see update()
 */
bool Nunchuk::initialize() {

  // Init I2C com
  TinyI2C.init(true);

  // normal init sequence; data is encrypted
  // for unencrypted use: START 0xF0, 0x55, STOP - START, 0xFB, 0x00, STOP
  bool con = TinyI2C.start(NUNCHUK_I2C_ID, 0);
  if (!con) {
#if defined(HAS_SERIAL) && defined(NUNCHUK_DEBUG)
    USART_WriteString("I2C connect error\n");
#endif
    return false;
  }

#ifndef NOT_ENCRYPTED
  TinyI2C.write(0x40);
  TinyI2C.write(0x00);
#else
  TinyI2C.write(0xF0);
  TinyI2C.write(0x55);
  TinyI2C.stop();
  TinyI2C.start(NUNCHUK_I2C_ID, 0);
  TinyI2C.write(0xFB);
  TinyI2C.write(0x00);
#endif
  TinyI2C.stop();
  _delay_ms(10);

  // Read device type
  TinyI2C.start(NUNCHUK_I2C_ID, 0);
  TinyI2C.write(WII_DEVICE_REGISTER);
  TinyI2C.stop();
  _delay_ms(10);

  TinyI2C.start(NUNCHUK_I2C_ID, NUNCHUK_BUFFER_SIZE);
  uint32_t readID1 = 0;
  uint32_t readID2 = 0;

  readID1 = TinyI2C.read();
  readID1 <<= 8;
  uint8_t r = TinyI2C.read();
  readID1 |= r;
  readID1 <<= 8;
  r = TinyI2C.read();
  readID1 |= r;
  readID1 <<= 8;
  r = TinyI2C.read();
  readID1 |= r;

  readID2 = TinyI2C.read();
  readID2 <<= 8;
  r = TinyI2C.read();
  readID2 |= r;
  TinyI2C.stop();

  if (readID1 == NUNCHUK_DEVICE_ID_PART1 && readID2 == NUNCHUK_DEVICE_ID_PART2) {
    #if defined(HAS_SERIAL) && defined(NUNCHUK_DEBUG)
      USART_WriteString("# Wii Nunchuk is OK.\n");
    #endif
    // read calibration data
    _delay_ms(100);
    get_calibration();

    // read current data
    _delay_ms(100);
    update();
  } else {
    #if defined(HAS_SERIAL) && defined(NUNCHUK_DEBUG)
    USART_WriteString("Not a Nunchuk. ID found: ");
    USART_WriteUInt(readID1, 16);
    USART_WriteString(" ");
    USART_WriteUInt(readID2, 16);
    USART_WriteString("\n");
    #endif
    return false;
  }

  return true;
}

/**
 * @brief Updates sensor data from the Nunchuk.
 * 
 * Performs the following operations:
 * 1. Sends read request to position register (0x00)
 * 2. Reads 6 bytes of data: joystick_x, joystick_y, accel_x_high, accel_y_high,
 *    accel_z_high, and button_flags/accel_low
 * 3. Analyzes current joystick position using get_joystick_position()
 * 4. Checks if position or button state changed since last update
 * 5. If HAS_SERIAL is defined and buttons are pressed, outputs button state
 * 
 * The 6-byte data structure:
 * - Byte 0: Joystick X (8 bits)
 * - Byte 1: Joystick Y (8 bits)
 * - Byte 2: Accelerometer X (high 8 bits)
 * - Byte 3: Accelerometer Y (high 8 bits)
 * - Byte 4: Accelerometer Z (high 8 bits)
 * - Byte 5: (2 bits accel X low) (2 bits accel Y low) (2 bits accel Z low) 
 *           (1 bit Z button) (1 bit C button)
 * 
 * @return true if joystick position or button state changed since last update,
 *         false if no change detected
 * 
 * @see get_joystick_position()
 */
bool Nunchuk::update() {
  TinyI2C.start(NUNCHUK_I2C_ID, 0);
  TinyI2C.write(WII_POSITION_REGISTER);
  TinyI2C.stop();
  _delay_us(500);

  TinyI2C.start(NUNCHUK_I2C_ID, NUNCHUK_BUFFER_SIZE);
  _buffer[0] = TinyI2C.read();
  _buffer[1] = TinyI2C.read();
  _buffer[2] = TinyI2C.read();
  _buffer[3] = TinyI2C.read();
  _buffer[4] = TinyI2C.read();
  _buffer[5] = TinyI2C.read();
  TinyI2C.stop();
  

  char joystick_pos = this->get_joystick_position();
  if ((_joystick_prev_position != joystick_pos) || (_buttons != (_buffer[5] & 0x3))) {
    _joystick_prev_position = joystick_pos;
    _buttons = _buffer[5] & 0x3;

    #if defined(HAS_SERIAL) && defined(NUNCHUK_DEBUG)
    if (_buttons != 3) {
      USART_WriteString("Button: ");
      USART_WriteString(z_button()?"Z":"z");
      USART_WriteString(" ");
      USART_WriteString(c_button()?"C":"c");
      USART_WriteString(" ");
      USART_WriteUInt(_buttons, 2);
      USART_WriteString("\n");
    } 
    #endif

    return true;
  }

  return false;
}

/**
 * @brief Retrieves and processes calibration data from the Nunchuk.
 * 
 * Reads 16 bytes of calibration data from the Nunchuk device and extracts:
 * 
 * **Accelerometer 0g calibration (bytes 0-3):**
 * - X, Y, Z 8-bit values (bytes 0-2)
 * - Low 2 bits combined from byte 3
 * - Result: 10-bit values for zero-gravity reference
 * 
 * **Accelerometer 1g calibration (bytes 4-7):**
 * - X, Y, Z 8-bit values (bytes 4-6)
 * - Low 2 bits combined from byte 7
 * - Result: 10-bit values for one-gravity reference
 * 
 * **Joystick calibration (bytes 8-13):**
 * - X axis: max, min, center (bytes 8-10)
 * - Y axis: max, min, center (bytes 11-13)
 * 
 * **Checksum (bytes 14-15):** Read but not used
 * 
 * After reading, the method calculates resolution factors for accelerometer
 * calibration conversion from raw values to gravitational units (g):
 * - _ax_res = 1.0 / (_ax_1g - _ax_0g)
 * - _ay_res = 1.0 / (_ay_1g - _ay_0g)
 * - _az_res = 1.0 / (_az_1g - _az_0g)
 * 
 * @return true (always succeeds if I2C communication works)
 * 
 * @note Called automatically during initialize(). Should not be called directly
 *       unless recalibration is needed.
 * 
 * @see initialize()
 */
bool Nunchuk::get_calibration() {
  TinyI2C.start(NUNCHUK_I2C_ID, 0);
  TinyI2C.write(WII_CALIBRATION_REGISTER);
  TinyI2C.stop();
  _delay_ms(10);

  uint8_t r = 0;

  TinyI2C.start(NUNCHUK_I2C_ID, NUNCHUK_CALIBRATION_BUFFER_SIZE);
  // X,Y,Z 0g
  _ax_0g = TinyI2C.read();
  _ay_0g = TinyI2C.read();
  _az_0g = TinyI2C.read();
  r = TinyI2C.read();
  _ax_0g <<= 2;
  _ax_0g |= ((r >> 2) & 0x03);
  _ay_0g <<= 2;
  _ay_0g |= ((r >> 4) & 0x03);
  _az_0g <<= 2;
  _az_0g |= ((r >> 6) & 0x03);

  // X,Y,Z 1g
  _ax_1g = TinyI2C.read();
  _ay_1g = TinyI2C.read();
  _az_1g = TinyI2C.read();
  r = TinyI2C.read();
  _ax_1g <<= 2;
  _ax_1g |= ((r >> 2) & 0x03);
  _ay_1g <<= 2;
  _ay_1g |= ((r >> 4) & 0x03);
  _az_1g <<= 2;
  _az_1g |= ((r >> 6) & 0x03);

  // Joystick X
  _jx_max = TinyI2C.read();
  _jx_min = TinyI2C.read();
  _jx_center = TinyI2C.read();
  // Joystick Y
  _jy_max = TinyI2C.read();
  _jy_min = TinyI2C.read();
  _jy_center = TinyI2C.read();
  // checksum
  r = TinyI2C.read();
  r = TinyI2C.read();

  TinyI2C.stop();

  // derive value
  _ax_res = 1.0f/((float)_ax_1g-(float)_ax_0g);
  _ay_res = 1.0f/((float)_ay_1g-(float)_ay_0g);
  _az_res = 1.0f/((float)_az_1g-(float)_az_0g);

  return true;
}

/**
 * @brief Calculates the joystick displacement magnitude.
 * 
 * Computes the Euclidean distance from the joystick center position to the
 * current position, then scales it to a 0-255 value.
 * 
 * Algorithm:
 * 1. Calculate offsets from center: dx = x - center_x, dy = y - center_y
 * 2. Compute magnitude: distance = sqrt(dx² + dy²)
 * 3. Normalize by dividing by 10000.0 (empirically derived scaling factor)
 * 4. Scale to 0-255 range
 * 
 * The maximum expected radius is approximately 100 pixels from center,
 * so the scaling factor of 10000.0 provides a good range mapping.
 * 
 * @return Joystick strength as a value from 0 (at center) to 255
 *         (at maximum displacement, approximately at the edge)
 * 
 * @see joystick_x()
 * @see joystick_y()
 * @see joystick_x_center()
 * @see joystick_y_center()
 */
uint8_t Nunchuk::joystick_strength() {
  int16_t dx = joystick_x() - _jx_center;
  int16_t dy = joystick_y() - _jy_center;

  return (uint8_t)(sqrt((dx*dx+dy*dy)/10000.0f) * 255); // max radius is around 100
}

/**
 * @brief Displays calibration data to the serial port.
 * 
 * Outputs the following information (only if HAS_SERIAL is defined):
 * 
 * **Joystick calibration:**
 * - JX: minimum, center, maximum raw values
 * - JY: minimum, center, maximum raw values
 * 
 * **Accelerometer 0g calibration:**
 * - X, Y, Z raw values at zero gravity
 * 
 * **Accelerometer 1g calibration:**
 * - X, Y, Z raw values at one gravity
 * 
 * **Accelerometer range and resolution (for each axis):**
 * - AccX: minimum and maximum values in g units, resolution in g/unit
 * - AccY: minimum and maximum values in g units, resolution in g/unit
 * - AccZ: minimum and maximum values in g units, resolution in g/unit
 * 
 * The min/max values assume the accelerometer can measure from -1g to +1023 units,
 * providing the full range of detectable acceleration.
 * 
 * @note Only produces output if HAS_SERIAL is defined during compilation.
 *       Output is formatted for easy reading and debugging of calibration data.
 * 
 * @see display()
 */
void Nunchuk::display_calibration() {
  #ifdef HAS_SERIAL
  USART_WriteString("Nunchuk calibration:\n");
  USART_WriteString("  JX ");
  USART_WriteUInt(_jx_min);
  USART_WriteString(" ");
  USART_WriteUInt(_jx_center);
  USART_WriteString(" ");
  USART_WriteUInt(_jx_max);
  USART_WriteString("\n");
  USART_WriteString("  JY ");
  USART_WriteUInt(_jy_min);
  USART_WriteString(" ");
  USART_WriteUInt(_jy_center);
  USART_WriteString(" ");
  USART_WriteUInt(_jy_max);
  USART_WriteString("\n");

  USART_WriteString("  Acc 0G ");
  USART_WriteUInt(_ax_0g);
  USART_WriteString(" ");
  USART_WriteUInt(_ay_0g);
  USART_WriteString(" ");
  USART_WriteUInt(_az_0g);
  USART_WriteString("\n");

  USART_WriteString("  Acc 1G ");
  USART_WriteUInt(_ax_1g);
  USART_WriteString(" ");
  USART_WriteUInt(_ay_1g);
  USART_WriteString(" ");
  USART_WriteUInt(_az_1g);
  USART_WriteString("\n");

  float ming = -1.0f * (float)_ax_0g * _ax_res;
  float maxg = (1024.0f - _ax_0g) * _ax_res;

  USART_WriteString("  AccX min=");
  USART_WriteFloat(ming, 4, 1);
  USART_WriteString(" max=");
  USART_WriteFloat(maxg, 4, 1);
  USART_WriteString(" res=");
  USART_WriteFloat(_ax_res, 6, 3);
  USART_WriteString("\n");

  ming = -1.0f * (float)_ay_0g * _ay_res;
  maxg = (1024.0f - _ay_0g) * _ay_res;

  USART_WriteString("  AccY min=");
  USART_WriteFloat(ming, 4, 1);
  USART_WriteString(" max=");
  USART_WriteFloat(maxg, 4, 1);
  USART_WriteString(" res=");
  USART_WriteFloat(_ay_res, 6, 3);
  USART_WriteString("\n");

  ming = -1.0f * (float)_az_0g * _az_res;
  maxg = (1024.0f - _az_0g) * _az_res;

  USART_WriteString("  AccZ min=");
  USART_WriteFloat(ming, 4, 1);
  USART_WriteString(" max=");
  USART_WriteFloat(maxg, 4, 1);
  USART_WriteString(" res=");
  USART_WriteFloat(_az_res, 6, 3);
  USART_WriteString("\n");

  #elif defined(HAS_INT0_SERIAL)
  INT0_WriteString("Nunchuk calibration:\n");
  INT0_WriteString("  JX ");
  INT0_WriteUInt(_jx_min);
  INT0_WriteString(" ");
  INT0_WriteUInt(_jx_center);
  INT0_WriteString(" ");
  INT0_WriteUInt(_jx_max);
  INT0_WriteString("\n");
  INT0_WriteString("  JY ");
  INT0_WriteUInt(_jy_min);
  INT0_WriteString(" ");
  INT0_WriteUInt(_jy_center);
  INT0_WriteString(" ");
  INT0_WriteUInt(_jy_max);
  INT0_WriteString("\n");

  // do not show accelerometer calibration on INT0 serial, to save time and space
  // use of INT0 as serial means that we are on a resource-constrained device, so we prioritize joystick calibration output
  
  #endif
}

/**
 * @brief Displays current sensor readings to the serial port.
 * 
 * Outputs the following information (only if HAS_SERIAL is defined):
 * 
 * **Joystick position:**
 * - Current X and Y coordinates (raw 0-255 values)
 * 
 * **Accelerometer readings:**
 * - Raw 10-bit acceleration values for X, Y, Z axes
 * - Use x_g(), y_g(), z_g() methods to get calibrated g-force values
 * 
 * **Button states:**
 * - Z button: displayed as 'Z' if pressed, 'z' if not pressed
 * - C button: displayed as 'C' if pressed, 'c' if not pressed
 * 
 * Typical output:
 * ```
 * Nunchuk info:
 * joystick: 128, 130
 * accel: 512, 510, 516
 * Button: z c
 * ```
 * 
 * @note Only produces output if HAS_SERIAL is defined during compilation.
 *       Intended for real-time monitoring of sensor data during development.
 * 
 * @see display_calibration()
 * @see x_g(), y_g(), z_g() for calibrated acceleration values
 */
void Nunchuk::display() {
#ifdef HAS_SERIAL
  USART_WriteString("Nunchuk info:\n");

  USART_WriteString("joystick: ");
  USART_WriteUInt(joystick_x());
  USART_WriteString(", ");
  USART_WriteUInt(joystick_y());
  USART_WriteString("\n");

  USART_WriteString("accel: ");
  USART_WriteUInt(x_acceleration());
  USART_WriteString(", ");
  USART_WriteUInt(y_acceleration());
  USART_WriteString(", ");
  USART_WriteUInt(z_acceleration());
  USART_WriteString("\n");

  USART_WriteString("Button: ");
  USART_WriteString(z_button()?"Z":"z");
  USART_WriteString(" ");
  USART_WriteString(c_button()?"C":"c");
  USART_WriteString("\n");
#elif defined(HAS_INT0_SERIAL)
  INT0_WriteString("Nunchuk info:\n");

  INT0_WriteString("joystick: ");
  INT0_WriteUInt(joystick_x());
  INT0_WriteString(", ");
  INT0_WriteUInt(joystick_y());
  INT0_WriteString("\n");

  INT0_WriteString("accel: ");
  INT0_WriteUInt(x_acceleration());
  INT0_WriteString(", ");
  INT0_WriteUInt(y_acceleration());
  INT0_WriteString(", ");
  INT0_WriteUInt(z_acceleration());
  INT0_WriteString("\n");

  INT0_WriteString("Button: ");
  INT0_WriteString(z_button()?"Z":"z");
  INT0_WriteString(" ");
  INT0_WriteString(c_button()?"C":"c");
  INT0_WriteString("\n");
#endif
}

/**
 * @brief Determines the joystick's directional position.
 * 
 * Analyzes the current joystick X and Y coordinates relative to the center
 * position, dividing the joystick range into 9 regions (8 directions + center).
 * 
 * **Algorithm:**
 * 1. Calculate dead zone size: 1/6 of the joystick range for each axis
 * 2. Check if X and Y are within center region (±dead zone)
 * 3. If X is within center and Y is within center → CENTER
 * 4. If X is within center and Y is above → NORTH
 * 5. If X is within center and Y is below → SOUTH
 * 6. If X is to the right and Y is within center → EAST
 * 7. If X is to the right and Y is above → NORTH_EAST
 * 8. If X is to the right and Y is below → SOUTH_EAST
 * 9. If X is to the left and Y is within center → WEST
 * 10. If X is to the left and Y is above → NORTH_WEST
 * 11. If X is to the left and Y is below → SOUTH_WEST
 * 12. Otherwise → UNKNOWN
 * 
 * **Dead Zone:** The center region extends ±1/6 of the total range on each axis.
 * For example, if the range is 0-255, the dead zone is approximately ±42 pixels.
 * 
 * **Return Values:**
 * - '^' (NORTH), 'v' (SOUTH), '<' (WEST), '>' (EAST) for cardinal directions
 * - '\\', '/', ',' , '`' for diagonal directions (NW, NE, SW, SE)
 * - 'x' (CENTER) for centered joystick
 * - '?' (UNKNOWN) if position doesn't match any region
 * 
 * @return Character constant representing the joystick position
 *         (one of the NUNCHUK_JOYSTICK_* constants)
 * 
 * @see joystick_x()
 * @see joystick_y()
 * @see joystick_x_center()
 * @see joystick_y_center()
 */
char Nunchuk::get_joystick_position() {

  uint8_t resx = (_jx_max -_jx_min) / 6;
  uint8_t resy = (_jy_max -_jy_min) / 6;

  if (_jx_center - resx <= joystick_x() && joystick_x() <= _jx_center + resx) {
    // X center position
    if (_jy_center - resy <= joystick_y() && joystick_y() <= _jy_center + resy) {
      // Y center position
      return NUNCHUK_JOYSTICK_CENTER;
    } else if (_jy_center + resy < joystick_y()) {
      // Y north position
      return NUNCHUK_JOYSTICK_NORTH;
    } else if (joystick_y() < _jy_center - resy) {
      // Y south position
      return NUNCHUK_JOYSTICK_SOUTH;
    }
  } else if (_jx_center + resx < joystick_x()) {
    // X east position
    if (_jy_center - resy <= joystick_y() && joystick_y() <= _jy_center + resy) {
      // Y center position
      return NUNCHUK_JOYSTICK_EAST;
    } else if (_jy_center + resy < joystick_y()) {
      // Y north position
      return NUNCHUK_JOYSTICK_NORTH_EAST;
    } else if (joystick_y() < _jy_center - resy) {
      // Y south position
      return NUNCHUK_JOYSTICK_SOUTH_EAST;
    }
  } else if (joystick_x() < _jx_center - resx) {
    // X west position
    if (_jy_center - resy <= joystick_y() && joystick_y() <= _jy_center + resy) {
      // Y center position
      return NUNCHUK_JOYSTICK_WEST;
    } else if (_jy_center + resy < joystick_y()) {
      // Y north position
      return NUNCHUK_JOYSTICK_NORTH_WEST;
    } else if (joystick_y() < _jy_center - resy) {
      // Y south position
      return NUNCHUK_JOYSTICK_SOUTH_WEST;
    }
  }

  return NUNCHUK_JOYSTICK_UNKNOWN;
}