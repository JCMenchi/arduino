/**
 * @file BME280.h
 * @brief Driver for Bosch BME280/BMP280 environmental sensor
 *
 * This module provides functions to interface with the BME280 (or BMP280 variant)
 * environmental sensor via I2C. The BME280 is a combined sensor that measures:
 * - Temperature (in Celsius)
 * - Humidity (relative humidity %RH)
 * - Atmospheric pressure (in Pascals)
 *
 * The module provides both individual sensor readings and a combined read function.
 * Measurements are returned in fixed-point formats as specified by the Bosch datasheet:
 * - Temperature: 0.01°C resolution
 * - Humidity: Q22.10 format (divide by 1024 for %RH)
 * - Pressure: Q24.8 format (divide by 256 for Pa)
 *
 * Typical usage:
 * @code
 * if (BME280_begin()) {  // Initialize sensor
 *     int32_t temp_raw, pres_raw;
 *     uint16_t hum_raw;
 *     BME280_read(pres_raw, temp_raw, hum_raw);
 *     
 *     double temp_c = temp_raw / 100.0;
 *     double hum_pct = hum_raw / 1024.0;
 *     double pres_pa = pres_raw / 256.0;
 * }
 * @endcode
 *
 * @note This module uses I2C for communication. Ensure I2C is initialized before
 *       calling BME280_begin().
 * @note The module supports both BME280 (with humidity) and BMP280 (pressure/temperature only)
 *       sensor variants.
 *
 * @see https://www.bosch-sensortec.com/products/environmental-sensors/humidity-sensors-bme280/
 */

#ifndef _BME_280_H
#define _BME_280_H

#include <stdint.h>

/**
 * @name Chip Model Identifiers
 * Used to identify which variant of the Bosch sensor is connected.
 * @{
 */

/** @brief Unknown or unidentified chip model */
#define ChipModel_UNKNOWN 0

/** @brief Bosch BMP280 (pressure and temperature only) */
#define ChipModel_BMP280 0x58

/** @brief Bosch BME280 (pressure, temperature, and humidity) */
#define ChipModel_BME280 0x60

/** @} */


/**
 * @brief Initializes the BME280 sensor via I2C communication.
 *
 * Probes the I2C bus for a BME280 or BMP280 sensor, reads calibration data from
 * the device, and prepares it for measurements. This function must be called once
 * before any sensor readings can be obtained.
 *
 * @return true if a supported sensor (BME280 or BMP280) was successfully detected
 *         and initialized; false if initialization failed or no sensor was found.
 *
 * @note The I2C interface (TinyI2CMaster) must be initialized before calling this.
 * @note If this function returns false, do not call other BME280 functions as
 *       the sensor is not ready.
 */
bool BME280_begin();

/**
 * @brief Reads pressure and temperature from the sensor.
 *
 * Performs a single measurement cycle and returns the pressure and temperature
 * values. This is a lower-level function; use BME280_read() for a complete read
 * that includes humidity.
 *
 * @param[out] temp Temperature reading in fixed-point format
 *             - Resolution: 0.01°C per unit
 *             - To get Celsius: divide by 100
 *             - Example: value 2137 = 21.37°C
 *             - Range: -4000 to 8500 (approx -40°C to +85°C)
 *
 * @return Pressure reading in fixed-point Q24.8 format
 *         - Upper 24 bits: integer Pa value
 *         - Lower 8 bits: fractional part (256 = 1 Pa)
 *         - To get Pascals: divide by 256
 *         - Example: return 24674867 = 96386.2 Pa = 963.862 hPa
 *         - Range: 30000 Pa (300 hPa) to 110000 Pa (1100 hPa)
 *
 * @note Only use this function if you specifically need only pressure and temperature.
 *       For complete readings, use BME280_read() instead.
 * @note Humidity sensors (BME280 only) are not read by this function.
 */
int32_t BME280_pres(int32_t &temp);

/**
 * @brief Reads humidity and temperature from the sensor.
 *
 * Performs a single measurement cycle and returns the humidity and temperature
 * values. This is a lower-level function; use BME280_read() for a complete read
 * that includes pressure.
 *
 * @param[out] temp Temperature reading in fixed-point format
 *             - Resolution: 0.01°C per unit
 *             - To get Celsius: divide by 100
 *             - Example: value 2137 = 21.37°C
 *             - Range: -4000 to 8500 (approx -40°C to +85°C)
 *
 * @return Humidity reading in fixed-point Q22.10 format
 *         - Upper 22 bits: integer %RH value
 *         - Lower 10 bits: fractional part (1024 = 1%)
 *         - To get %RH: divide by 1024
 *         - Example: return 47445 = 47445/1024 = 46.333 %RH
 *         - Range: 0 to 102400 (0% to 100%)
 *
 * @note Only use this function if you specifically need only humidity and temperature.
 *       For complete readings, use BME280_read() instead.
 * @note This function only works with BME280 sensors that have a humidity module.
 *       BMP280 sensors will return unreliable humidity values.
 */
uint16_t BME280_hum(int32_t &temp);

/**
 * @brief Performs a complete sensor reading of all available measurements.
 *
 * Reads all available sensor data in a single operation: pressure, temperature,
 * and humidity (if supported by the sensor). This is the recommended function
 * for most applications.
 *
 * @param[out] pressure Temperature reading in fixed-point format
 *             - Resolution: 0.01°C per unit
 *             - To get Celsius: divide by 100
 *             - Example: value 2137 = 21.37°C
 *             - Range: -4000 to 8500 (approx -40°C to +85°C)
 *
 * @param[out] temperature Pressure reading in fixed-point Q24.8 format
 *             - Upper 24 bits: integer Pa value
 *             - Lower 8 bits: fractional part (256 = 1 Pa)
 *             - To get Pascals: divide by 256
 *             - Example: value 24674867 = 96386.2 Pa = 963.862 hPa
 *             - Range: 30000 Pa to 110000 Pa
 *
 * @param[out] humidity Humidity reading in fixed-point Q22.10 format
 *             - Upper 22 bits: integer %RH value
 *             - Lower 10 bits: fractional part (1024 = 1%)
 *             - To get %RH: divide by 1024
 *             - Example: value 47445 = 46.333 %RH
 *             - Range: 0 to 102400 (0% to 100%)
 *             - Note: On BMP280 (no humidity), this value will be unreliable.
 *
 * @return void
 *
 * @note Parameters are named counter-intuitively due to legacy C function signature.
 *       The order in the parameter list does not match the order of the names.
 *       Parameters are: @c pressure, @c temperature, @c humidity (in that order)
 *
 * Example usage:
 * @code
 * int32_t pres, temp;
 * uint16_t hum;
 * BME280_read(pres, temp, hum);
 * 
 * printf("Temperature: %.2f°C\n", temp / 100.0);
 * printf("Pressure: %.2f Pa\n", pres / 256.0);
 * printf("Humidity: %.2f %%\n", hum / 1024.0);
 * @endcode
 */
void BME280_read(int32_t &pressure, int32_t &temperature, uint16_t &humidity);

/**
 * @brief Returns the chip model identifier of the detected sensor.
 *
 * Retrieves the model identifier that was detected during initialization.
 * Use this to determine whether a BME280 (with humidity) or BMP280 (without humidity)
 * is connected.
 *
 * @return uint8_t Chip model identifier
 *         - ChipModel_BME280 (0x60): Full BME280 sensor with humidity measurement
 *         - ChipModel_BMP280 (0x58): BMP280 sensor (no humidity measurement)
 *         - ChipModel_UNKNOWN (0x00): Unknown sensor or initialization failed
 *
 * Example usage:
 * @code
 * uint8_t model = BME280_chipModel();
 * if (model == ChipModel_BME280) {
 *     // Can use humidity readings
 * } else if (model == ChipModel_BMP280) {
 *     // Humidity readings will be unreliable
 * } else {
 *     // Sensor not initialized
 * }
 * @endcode
 */
uint8_t BME280_chipModel();

#endif  // _BME_280_H
