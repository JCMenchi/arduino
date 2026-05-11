#ifndef __NUNCHUK_H__
#define __NUNCHUK_H__

#include <stdint.h>
#include <math.h>

/**
 * @file nunchuk.h
 * @brief Driver for Wii Nunchuk controller via I2C interface.
 * 
 * Provides an interface to communicate with a Wii Nunchuk controller connected
 * via the I2C bus. The Nunchuk reports joystick position, 3-axis accelerometer
 * data, and button states. The driver handles device initialization, calibration,
 * and data reading.
 * 
 * For more details on the Nunchuk protocol, see:
 * http://wiibrew.org/wiki/Wiimote/Extension_Controllers/Nunchuck
 */

/// @defgroup JoystickPositionChars Joystick Position Constants
/// @brief Character representations of joystick directional positions.
/// @{

/** Joystick pointing North (up) */
const char NUNCHUK_JOYSTICK_NORTH = '^';

/** Joystick pointing North-West (up-left) */
const char NUNCHUK_JOYSTICK_NORTH_WEST = '\\';

/** Joystick pointing North-East (up-right) */
const char NUNCHUK_JOYSTICK_NORTH_EAST = '/';

/** Joystick pointing West (left) */
const char NUNCHUK_JOYSTICK_WEST = '<';

/** Joystick pointing East (right) */
const char NUNCHUK_JOYSTICK_EAST = '>';

/** Joystick pointing South (down) */
const char NUNCHUK_JOYSTICK_SOUTH = 'v';

/** Joystick pointing South-West (down-left) */
const char NUNCHUK_JOYSTICK_SOUTH_WEST = ',';

/** Joystick pointing South-East (down-right) */
const char NUNCHUK_JOYSTICK_SOUTH_EAST = '`';

/** Joystick in center position */
const char NUNCHUK_JOYSTICK_CENTER = 'x';

/** Joystick position unknown or out of range */
const char NUNCHUK_JOYSTICK_UNKNOWN = '?';

/// @}

/// @defgroup BufferSizes I2C Buffer Sizes
/// @brief Constants defining I2C message buffer sizes.
/// @{

/** Size of the data buffer for position reads (6 bytes) */
#define NUNCHUK_BUFFER_SIZE 6

/** Size of the calibration data buffer (16 bytes) */
#define NUNCHUK_CALIBRATION_BUFFER_SIZE 16

/// @}

/**
 * @class Nunchuk
 * @brief Interface driver for Wii Nunchuk controller.
 * 
 * Manages communication with a Wii Nunchuk controller via I2C. Handles device
 * initialization, calibration data retrieval, and periodic updates of sensor data.
 * Provides access to joystick position, 3-axis acceleration, button states, and
 * derived tilt angles.
 */
class Nunchuk {
public:

  /**
   * @brief Constructor for Nunchuk controller.
   * 
   * Initializes member variables with default values. Call initialize() to
   * establish I2C communication with the physical device.
   */
  Nunchuk() : _joystick_prev_position(NUNCHUK_JOYSTICK_UNKNOWN), _buttons(3) {}
  /**
   * @brief Initializes the Nunchuk controller.
   * 
   * Establishes I2C communication with the Nunchuk device, configures it for
   * unencrypted data mode, verifies the device ID, and retrieves calibration data.
   * 
   * @return true if initialization was successful, false if device not found or
   *         communication error occurred.
   */
  bool initialize();

  /**
   * @brief Updates sensor data from the Nunchuk.
   * 
   * Reads the latest position and button data from the device. Should be called
   * periodically to keep sensor data current.
   * 
   * @return true if joystick position or button state changed since last update,
   *         false if no change detected.
   */
  bool update();
  
  /**
   * @brief Determines the joystick's directional position.
   * 
   * Analyzes the current joystick coordinates and returns one of the eight
   * directional positions (N, NE, E, SE, S, SW, W, NW), center, or unknown.
   * 
   * @return A character constant representing the joystick position
   *         (NUNCHUK_JOYSTICK_*). Positions are divided into regions using
   *         1/6 of the joystick range as the dead zone.
   */
  char get_joystick_position();

  /// @defgroup JoystickAccessors Joystick Data Accessors
  /// @brief Methods to read raw joystick position values.
  /// @{

  /**
   * @brief Gets the raw X coordinate of the joystick.
   * @return X position (0-255), where 128 is center.
   */
  inline uint8_t joystick_x() const { return _buffer[0]; }

  /**
   * @brief Gets the raw Y coordinate of the joystick.
   * @return Y position (0-255), where 128 is center.
   */
  inline uint8_t joystick_y() const { return _buffer[1]; }

  /// @}

  /// @defgroup JoystickCalibration Joystick Calibration Data
  /// @brief Methods to access joystick calibration limits and center.
  /// @{

  /** @brief Gets the minimum X joystick value from calibration. */
  inline uint8_t joystick_x_min() const { return _jx_min; }

  /** @brief Gets the maximum X joystick value from calibration. */
  inline uint8_t joystick_x_max() const { return _jx_max; }

  /** @brief Gets the center X joystick value from calibration. */
  inline uint8_t joystick_x_center() const { return _jx_center; }
  
  /** @brief Gets the minimum Y joystick value from calibration. */
  inline uint8_t joystick_y_min() const { return _jy_min; }

  /** @brief Gets the maximum Y joystick value from calibration. */
  inline uint8_t joystick_y_max() const { return _jy_max; }

  /** @brief Gets the center Y joystick value from calibration. */
  inline uint8_t joystick_y_center() const { return _jy_center; }

  /// @}

  /**
   * @brief Calculates the joystick displacement magnitude.
   * 
   * Computes the distance from the center point to the current position,
   * normalized to 0-255.
   * 
   * @return Joystick strength as a value from 0 (at center) to 255
   *         (at maximum displacement, approximately at the edge).
   */
  uint8_t joystick_strength();

  /// @defgroup AccelerometerGForce Accelerometer G-Force Calculations
  /// @brief Methods to read acceleration in units of gravitational acceleration (g).
  /// Uses calibration data to convert raw acceleration values.
  /// @{

  /**
   * @brief Gets the acceleration along the X axis in units of gravity.
   * @return Acceleration in g (where 1g ≈ 9.8 m/s²).
   *         Positive values indicate upward/forward acceleration.
   */
  inline float x_g() const {
    return ((float)x_acceleration() - (float)_ax_0g) * _ax_res;
  }

  /**
   * @brief Gets the acceleration along the Y axis in units of gravity.
   * @return Acceleration in g (where 1g ≈ 9.8 m/s²).
   *         Positive values indicate rightward acceleration.
   */
  inline float y_g() const {
    return ((float)y_acceleration() - (float)_ay_0g) * _ay_res;
  }

  /**
   * @brief Gets the acceleration along the Z axis in units of gravity.
   * @return Acceleration in g (where 1g ≈ 9.8 m/s²).
   *         Positive values indicate upward acceleration.
   */
  inline float z_g() const {
    return ((float)z_acceleration() - (float)_az_0g) * _az_res;
  }

  /// @}

  /// @defgroup TiltAngles Tilt Angle Calculations
  /// @brief Methods to calculate orientation angles from acceleration data.
  /// Uses atan2 to compute tilt angles around each axis.
  /// @{

  /**
   * @brief Calculates the tilt angle around the X axis.
   * 
   * Based on Y and Z acceleration components. Useful for detecting
   * forward/backward tilt or roll motion.
   * 
   * @return Tilt angle in degrees (-180 to 180).
   */
  inline int16_t x_tilt() const {
    float d = sqrt(y_g() * y_g() + z_g() * z_g() );
    return atan2(x_g(), d)*360/M_PI;
  }

  /**
   * @brief Calculates the tilt angle around the Y axis.
   * 
   * Based on X and Z acceleration components. Useful for detecting
   * left/right tilt or pitch motion.
   * 
   * @return Tilt angle in degrees (-180 to 180).
   */
  inline int16_t y_tilt() const {
    float d = sqrt(x_g() * x_g() + z_g() * z_g() );
    return atan2(y_g(), d)*360/M_PI;
  }

  /**
   * @brief Calculates the tilt angle around the Z axis.
   * 
   * Based on X and Y acceleration components. Useful for detecting
   * rotation or yaw motion.
   * 
   * @return Tilt angle in degrees (0 to 180).
   */
  inline int16_t z_tilt() const {
    float d = sqrt(y_g() * y_g() + x_g() * x_g() );
    return atan2(d, z_g())*360/M_PI;
  }

  /// @}

  /// @defgroup RawAcceleration Raw Accelerometer Data
  /// @brief Methods to access raw 10-bit accelerometer readings.
  /// @{

  /**
   * @brief Gets the raw 10-bit acceleration reading on the X axis.
   * 
   * Raw value from the accelerometer before calibration conversion.
   * Use x_g() to get calibrated acceleration in gravitational units.
   * 
   * @return Raw acceleration value (typically 0-1023).
   */
  inline int16_t x_acceleration() const {
    return ((uint16_t)(_buffer[2]) << 2) | ((_buffer[5] >> 2) & 0x03);
  }

  /**
   * @brief Gets the raw 10-bit acceleration reading on the Y axis.
   * 
   * Raw value from the accelerometer before calibration conversion.
   * Use y_g() to get calibrated acceleration in gravitational units.
   * 
   * @return Raw acceleration value (typically 0-1023).
   */
  inline int16_t y_acceleration() const {
    return ((uint16_t)(_buffer[3]) << 2) | ((_buffer[5] >> 4) & 0x03);
  }

  /**
   * @brief Gets the raw 10-bit acceleration reading on the Z axis.
   * 
   * Raw value from the accelerometer before calibration conversion.
   * Use z_g() to get calibrated acceleration in gravitational units.
   * 
   * @return Raw acceleration value (typically 0-1023).
   */
  inline int16_t z_acceleration() const {
    return ((uint16_t)(_buffer[4]) << 2) | ((_buffer[5] >> 6) & 0x03);
  }

  /// @}

  /// @defgroup ButtonState Button State Accessors
  /// @brief Methods to check button press states.
  /// @{

  /**
   * @brief Checks if the Z button is currently pressed.
   * @return true if Z button is pressed, false otherwise.
   */
  inline bool z_button() const { return !(_buffer[5] & 0x01); }

  /**
   * @brief Checks if the C button is currently pressed.
   * @return true if C button is pressed, false otherwise.
   */
  inline bool c_button() const { return !(_buffer[5] & 0x02); }

  /// @}

  /// @defgroup Display Debug Output
  /// @brief Methods to output debug information to serial port (if enabled).
  /// @{

  /**
   * @brief Displays current sensor readings to the serial port.
   * 
   * Outputs joystick position, acceleration values, and button states.
   * Only functional if HAS_SERIAL is defined during compilation.
   */
  void display();

  /**
   * @brief Displays calibration data to the serial port.
   * 
   * Outputs joystick calibration limits (min/center/max) and accelerometer
   * calibration data (0g and 1g reference values, resolution factors).
   * Only functional if HAS_SERIAL is defined during compilation.
   */
  void display_calibration();

  /// @}

private:
  /// @defgroup PrivateMethods Private Helper Methods
  /// @{

  /**
   * @brief Retrieves calibration data from the Nunchuk device.
   * 
   * Reads calibration values from the Nunchuk's memory including:
   * - Joystick calibration (min, max, center for X and Y)
   * - Accelerometer calibration (0g and 1g reference values for X, Y, Z)
   * 
   * Used internally by initialize() and should not be called directly.
   * 
   * @return true if calibration data was successfully read.
   */
  bool get_calibration();

  /**
   * @brief Decodes a byte from encrypted Nunchuk data.
   * 
   * Note: Currently unused as the driver uses unencrypted mode (NOT_ENCRYPTED).
   * Kept for reference in case encrypted mode is needed in the future.
   * 
   * @param b The encrypted byte to decode.
   * @return The decoded byte.
   */
  uint8_t decode_byte(uint8_t b) { return (b ^ 0x17) + 0x17; }

  /// @}

  /// @defgroup PrivateMembers Private Member Variables
  /// @{

  /** Buffer containing the last read sensor data (6 bytes) */
  uint8_t _buffer[NUNCHUK_BUFFER_SIZE];

  /** Previous joystick position, used to detect changes in update() */
  char _joystick_prev_position;

  /** Previous button state, used to detect changes in update() */
  uint8_t _buttons;

  // ---- Joystick Calibration Data ----

  /** Maximum raw joystick X value from calibration */
  uint8_t _jx_max;

  /** Minimum raw joystick X value from calibration */
  uint8_t _jx_min;

  /** Center raw joystick X value from calibration */
  uint8_t _jx_center;

  /** Maximum raw joystick Y value from calibration */
  uint8_t _jy_max;

  /** Minimum raw joystick Y value from calibration */
  uint8_t _jy_min;

  /** Center raw joystick Y value from calibration */
  uint8_t _jy_center;

  // ---- Accelerometer Calibration Data ----

  /** Raw accelerometer X value at 0g (no acceleration) from calibration */
  uint16_t _ax_0g;

  /** Raw accelerometer Y value at 0g (no acceleration) from calibration */
  uint16_t _ay_0g;

  /** Raw accelerometer Z value at 0g (no acceleration) from calibration */
  uint16_t _az_0g;

  /** Raw accelerometer X value at 1g (one unit of gravity) from calibration */
  uint16_t _ax_1g;

  /** Raw accelerometer Y value at 1g (one unit of gravity) from calibration */
  uint16_t _ay_1g;

  /** Raw accelerometer Z value at 1g (one unit of gravity) from calibration */
  uint16_t _az_1g;

  /** Conversion factor for X acceleration (raw value to g units) */
  float _ax_res;

  /** Conversion factor for Y acceleration (raw value to g units) */
  float _ay_res;

  /** Conversion factor for Z acceleration (raw value to g units) */
  float _az_res;

  /// @}
};

#endif

