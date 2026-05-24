/**
 * @file hcsr04.h
 * @brief Driver for HC-SR04 Ultrasonic Distance Sensor
 *
 * This module provides a class-based interface for the popular HC-SR04 ultrasonic
 * distance sensor. The sensor uses ultrasonic waves to measure the distance to objects,
 * enabling contactless distance measurement for robotics and automation applications.
 *
 * ### Operating Principle
 *
 * The HC-SR04 works by:
 * 1. Sending a 10 µs trigger pulse to the TRIG pin
 * 2. Emitting an ultrasonic pulse (40 kHz)
 * 3. Measuring the time for the echo to return on the ECHO pin
 * 4. Calculating distance: distance = (echo_time / 2) × speed_of_sound
 *
 * ### Typical Wiring
 *
 * ```
 * HC-SR04 Module        Microcontroller
 * ──────────────────────────────────────
 * VCC (5V)      ────→  +5V
 * TRIG          ────→  GPIO pin (configurable)
 * ECHO          ────→  GPIO pin (configurable)
 * GND           ────→  GND
 * ```
 *
 * **Important:** The HC-SR04 operates at 5V logic levels. If your microcontroller
 * is 3.3V, use a voltage divider or level shifter on the ECHO pin to avoid damage.
 *
 * ### Features
 *
 * - Non-blocking distance measurements
 * - Configurable timeout to prevent hanging on failed measurements
 * - Simple distance reading in centimeters
 * - Support for multiple sensor instances
 * - Minimum distance: ~2 cm
 * - Maximum distance: ~400 cm (varies with environment)
 * - Measurement frequency: Up to 60 Hz
 *
 * ### Usage Example
 *
 * @code
 * // Create sensor instance with trigger on PC0, echo on PC1
 * HC_SR04 sensor(0, 1);
 *
 * // Initialize pins
 * sensor.setup();
 *
 * // Main loop
 * while (1) {
 *     int16_t distance = sensor.read();
 *     if (distance > 0) {
 *         printf("Distance: %d cm\n", distance);
 *     } else {
 *         printf("Measurement failed or timed out\n");
 *     }
 *     // Wait before next measurement
 *     _delay_ms(100);
 * }
 * @endcode
 *
 * ### Pin Configuration
 *
 * The sensor is configured to use a specific GPIO port (PORTC by default) set
 * via the HC_SR04_PORTID macro. To use a different port, define it before including
 * this header:
 *
 * @code
 * #define HC_SR04_PORTID B    // Use PORTB instead of PORTC
 * #include "hcsr04.h"
 * @endcode
 *
 * @see HC_SR04 class for API details
 * @note Requires gpio.h header and appropriate GPIO support
 * @note Designed for AVR microcontrollers (ATmega series)
 */

/*
    Driver for HC-­SR04 Ultrasonic Sensor

      +-----------------------+
      |   HC-SR04 Module      |
      |   __           __     |
      |  (  )         (  )    |  <-- Ultrasonic Transducers
      |                      |
      |  VCC  TRIG  ECHO  GND |  <-- Pins (bottom view)
      +-----------------------+
         |     |     |     |
        VCC   TRIG  ECHO  GND

  VCC: Power (5V)
  TRIG: Trigger Input
  ECHO: Echo Output
  GND: Ground

  This library provides a simple interface to read distance measurements from the HC-SR04 ultrasonic sensor.
  * The class allows setting the trigger and echo pins, reading distance measurements, and configuring a timeout for readings.
  * The distance is measured in centimeters.
  * The timeout can be adjusted to prevent blocking indefinitely if no echo is received.
  * The default timeout is set to 20,000 microseconds (same as 344 cm).
  * The read method returns the distance in centimeters or 0 if the measurement fails or times out.
  * The sensor operates by sending a short ultrasonic pulse and measuring the time it takes for the echo to return.
  * The sensor should be connected to a microcontroller with the specified trigger and echo pins.
  * The sensor's maximum range is typically around 400 cm, but this can vary based on environmental conditions.
  * The sensor should be used in an environment free from obstacles that could interfere with the ultrasonic waves.
  * The library is designed to be used with AVR microcontroller.
*/

#ifndef HC_SR04_H
#define HC_SR04_H

#include <gpio.h>
#include <stdint.h>

#ifndef HC_SR04_PORTID
#define HC_SR04_PORTID C
#endif

/**
 * @brief HC-SR04 Ultrasonic Distance Sensor Driver Class
 *
 * This class provides a simple and efficient interface to the HC-SR04 ultrasonic
 * sensor. It handles the timing and control of the sensor to measure distances to
 * nearby objects using ultrasonic waves.
 *
 * ### Measurement Principle
 *
 * The sensor operates by:
 * 1. Receiving a 10 µs HIGH pulse on the trigger pin
 * 2. Emitting a 40 kHz ultrasonic burst
 * 3. Measuring the time the ECHO pin stays HIGH (pulse width)
 * 4. Calculating distance from the echo time
 *
 * The relationship between echo time and distance is:
 * ```
 * distance_cm = (echo_time_µs / 2) × 0.0343
 *             = echo_time_µs / 58.3
 * ```
 *
 * The "/ 2" accounts for the round-trip distance (pulse goes out and back).
 *
 * ### Key Characteristics
 *
 * - **Measurement Range:** 2 cm to ~400 cm
 * - **Accuracy:** ±0.3 cm (typically)
 * - **Resolution:** 0.3 cm steps
 * - **Measurement Frequency:** Up to 60 Hz
 * - **Trigger Pulse:** 10 µs (automatically handled by setup())
 * - **Max Echo Time:** ~23.5 ms (for ~400 cm distance)
 * - **Beam Angle:** ~15° (cone angle)
 *
 * @note All pins must be on the same port as defined by HC_SR04_PORTID macro
 *       (default is PORTC for AVR ATmega)
 * @note The ECHO pin input should have a pull-down resistor (integrated in GPIO_INPUT_PULLDOWN)
 * @note Call setup() after construction to configure the GPIO pins
 * @note Not suitable for object detection through liquids or soft materials
 * @note Avoid metallic obstacles near the sensor to minimize false echoes
 *
 * ### Memory Usage
 *
 * Stack/SRAM: ~4 bytes per instance (two uint8_t pins + one uint16_t timeout)
 * Flash: ~200-300 bytes for the compiled code
 */
class HC_SR04 {
   public:
    /**
     * @brief Constructs an HC_SR04 sensor instance.
     *
     * Initializes the sensor with specified GPIO pins for trigger and echo signals.
     * The pins must reside on the same port as defined by HC_SR04_PORTID macro.
     * Call setup() after construction to configure the pins for GPIO operation.
     *
     * @param[in] trig Pin number for the trigger signal (output from MCU)
     *            This pin will be configured as GPIO output
     * @param[in] echo Pin number for the echo signal (input to MCU)
     *            This pin will be configured as GPIO input with pull-down
     * @param[in] timeOut Maximum time to wait for echo response in microseconds
     *            Default: 20000 µs (~344 cm distance equivalent)
     *            Typical range: 5000-30000 µs
     *            Higher values for longer range but slower response on failure
     *
     * @note After construction, you MUST call setup() before using read()
     * @note All timing is in microseconds (µs)
     *
     * Example:
     * @code
     * // Sensor on pins C0 (trigger) and C1 (echo)
     * HC_SR04 sensor(0, 1);
     * sensor.setup();  // Must be called
     * @endcode
     */
    HC_SR04(uint8_t trig, uint8_t echo, uint16_t timeOut = 20000UL)
        : triggerPin(trig), echoPin(echo), timeout(timeOut) {}

    /**
     * @brief Initializes the sensor GPIO pins for operation.
     *
     * Configures the trigger and echo pins as GPIO outputs/inputs with appropriate
     * initial states. This function must be called once after construction and before
     * any distance measurements.
     *
     * Configuration performed:
     * - Trigger pin: GPIO output, initial state LOW
     * - Echo pin: GPIO input with pull-down resistor
     *
     * @return void
     *
     * @note Must be called before calling read()
     * @note This function does not start measurements; it only configures pins
     * @note Pull-down resistor on echo pin prevents floating values during waiting
     *
     * Example:
     * @code
     * HC_SR04 sensor(0, 1);
     * sensor.setup();  // Initialize pins
     * int16_t distance = sensor.read();  // Now safe to measure
     * @endcode
     */
        // Initialize the pins
        GPIO_OUTPUT(HC_SR04_PORTID, triggerPin);
        GPIO_SET_LOW(HC_SR04_PORTID, triggerPin);
        GPIO_INPUT_PULLDOWN(HC_SR04_PORTID, echoPin);
    }

    /**
     * @brief Reads the distance to the nearest object.
     *
     * Performs a complete measurement cycle:
     * 1. Sends a 10 µs trigger pulse on the trigger pin
     * 2. Waits for the echo pin to go HIGH
     * 3. Measures the duration of the HIGH pulse (echo time)
     * 4. Calculates distance from the echo time
     * 5. Returns the result or 0 on timeout/failure
     *
     * The measurement is non-blocking for successful measurements but can block
     * for up to the configured timeout value (default 20,000 µs) if no echo is
     * received.
     *
     * @return int16_t Distance in centimeters
     *         - Positive value: Distance measurement (typically 2-400 cm)
     *         - 0: Measurement failed or timed out
     *         - Negative values: Should not occur in normal operation
     *
     * @note Setup() must be called before the first read() call
     * @note Do not call read() more frequently than 60 Hz (minimum 16.6 ms between calls)
     *       The sensor needs time between measurements to avoid interference
     * @note If read() returns 0, check:
     *       1. Is an object within 2-400 cm range?
     *       2. Is the echo pin properly wired?
     *       3. Are there reflective obstacles causing multiple echoes?
     *       4. Is the supply voltage sufficient (4.5V-5.5V)?
     * @note The measurement may be inaccurate due to:
     *       - Temperature variations (speed of sound changes with temperature)
     *       - Soft surfaces that absorb ultrasonic waves
     *       - Objects at oblique angles to the beam
     *       - Metallic surfaces causing multiple reflections
     *
     * Example:
     * @code
     * HC_SR04 sensor(0, 1);  // Trigger=PC0, Echo=PC1
     * sensor.setup();
     *
     * int16_t distance = sensor.read();
     *
     * if (distance > 0) {
     *     printf("Object at %d cm\n", distance);
     * } else if (distance == 0) {
     *     printf("No object detected or measurement timeout\n");
     * }
     * @endcode
     */

    /**
     * @brief Sets the timeout value for echo reception.
     *
     * The timeout prevents the read() method from blocking indefinitely if the
     * echo signal is never received. This can happen if:
     * - No object is in range
     * - The echo pin is disconnected
     * - The sensor is not powered
     * - Environmental conditions prevent echo return
     *
     * The timeout value should be chosen based on:
     * - Maximum measurable distance: timeout = distance_cm × 58 µs
     * - Default (20000 µs) allows measurements up to ~344 cm
     *
     * ### Timeout Calculation Examples
     *
     * | Distance | Timeout µs |
     * |----------|------------|
     * | 100 cm   | ~5,800 µs  |
     * | 200 cm   | ~11,600 µs |
     * | 300 cm   | ~17,400 µs |
     * | 400 cm   | ~23,200 µs |
     * | 500 cm   | ~29,000 µs |
     *
     * @param[in] timeOut Maximum echo wait time in microseconds
     *            Range: Typically 3000-40000 µs
     *            - Too low: Legitimate measurements will timeout
     *            - Too high: Failed measurements cause long delays
     *            - Default (20000): ~344 cm range
     *
     * @return void
     *
     * @note This should be called before read() to take effect
     * @note Changing timeout between measurements is allowed
     *
     * Example:
     * @code
     * HC_SR04 sensor(0, 1);
     * sensor.setup();
     *
     * // Default 20000 µs timeout
     * int16_t dist1 = sensor.read();
     *
     * // Change to allow 500 cm range
     * sensor.setTimeout(30000);
     * int16_t dist2 = sensor.read();
     *
     * // Change to faster fail on no object (lower timeout)
     * sensor.setTimeout(5000);  // Max ~86 cm
     * int16_t dist3 = sensor.read();
     * @endcode
     */

   private:
    /** @brief GPIO pin number for trigger signal output (10 µs pulse) */
    uint8_t triggerPin;

    /** @brief GPIO pin number for echo signal input (measures pulse duration) */
    uint8_t echoPin;

    /**
     * @brief Maximum time to wait for echo response in microseconds.
     *
     * Prevents read() from blocking indefinitely if no echo is received.
     * Default 20000 µs corresponds to approximately 344 cm distance.
     * Adjust using setTimeout() based on maximum expected range.
     *
     * Relationship: timeout = max_distance_cm × 58 µs
     */
    uint16_t timeout;
};

#endif  // HC_SR04_H