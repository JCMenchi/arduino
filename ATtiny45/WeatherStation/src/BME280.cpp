/*
BME280.cpp
This code records data from the BME280 sensor and provides an API.
This file is part of the Arduino BME280 library.
Copyright (C) 2016   Tyler Glenn

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.   See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.   If not, see <http://www.gnu.org/licenses/>.

Written: Dec 30 2015.
Last Updated: Oct 07 2017.

This header must be included in any derived code or copies of the code.

Based on the data sheet provided by Bosch for the Bme280 environmental sensor,
calibration code based on algorithms providedBosch, some unit conversations
courtesy of www.endmemo.com, altitude equation courtesy of NOAA, and dew point
equation courtesy of Brian McNoldy at http://andrew.rsmas.miami.edu.
 */

#include "BME280.h"
#include "TinyI2CMaster.h"

#define BME280_I2C_ADDR 0x76

// BME280 configuration
#define TEMP_SAMPLING 3 // use 4x sampling
#define HUM_SAMPLING 3  // use 4x sampling
#define PRES_SAMPLING 5 // use 16x oversampling

#define Mode_Sleep 0
#define Mode_Forced 1
#define Mode_Normal 3

#define BME280_MODE Mode_Normal
#define BME280_STANDBY_TIME 5 // 1000 ms
#define BME280_FILTER 5       // Filter 16
#define BME280_FILTER_OFF 1

static const uint8_t CTRL_HUM_ADDR = 0xF2;
static const uint8_t CTRL_MEAS_ADDR = 0xF4;
static const uint8_t CONFIG_ADDR = 0xF5;
static const uint8_t PRESS_ADDR = 0xF7;
static const uint8_t TEMP_ADDR = 0xFA;
static const uint8_t HUM_ADDR = 0xFD;
static const uint8_t TEMP_DIG_ADDR = 0x88;
static const uint8_t PRESS_DIG_ADDR = 0x8E;
static const uint8_t HUM_DIG_ADDR1 = 0xA1;
static const uint8_t HUM_DIG_ADDR2 = 0xE1;
static const uint8_t ID_ADDR = 0xD0;

static const uint8_t TEMP_DIG_LENGTH = 6;
static const uint8_t PRESS_DIG_LENGTH = 18;
static const uint8_t HUM_DIG_ADDR1_LENGTH = 1;
static const uint8_t HUM_DIG_ADDR2_LENGTH = 7;
static const uint8_t DIG_LENGTH = 32;
static const uint8_t SENSOR_DATA_LENGTH = 8;

uint16_t dig_T1;
int16_t dig_T2;
int16_t dig_T3;

uint8_t dig_H1;
int16_t dig_H2;
uint8_t dig_H3;
int16_t dig_H4;
int16_t dig_H5;
int8_t dig_H6;

uint16_t dig_P1;
int16_t dig_P2;
int16_t dig_P3;
int16_t dig_P4;
int16_t dig_P5;
int16_t dig_P6;
int16_t dig_P7;
int16_t dig_P8;
int16_t dig_P9;

/****************************************************************/
static bool WriteRegister(uint8_t addr, uint8_t data) {
  TinyI2C.start(BME280_I2C_ADDR, 0);
  bool ret = TinyI2C.write(addr);
  if (ret) {
    ret = TinyI2C.write(data);
  }
  TinyI2C.stop();
  return ret;
}
/****************************************************************/
static bool ReadRegister(uint8_t addr, uint8_t data[], uint8_t length) {
  // write register address
  TinyI2C.start(BME280_I2C_ADDR, 0);
  TinyI2C.write(addr);

  // read data
  TinyI2C.restart(BME280_I2C_ADDR, length);
  for (uint8_t i = 0; i < length; ++i) {
    data[i] = TinyI2C.read();
  }

  TinyI2C.stop();

  return true;
}
/****************************************************************/
static void WriteSettings() {
  uint8_t ctrlHum, ctrlMeas, config;

  // ctrl_hum register. (ctrl_hum[2:0] = Humidity oversampling rate.)
  ctrlHum = HUM_SAMPLING;
  // ctrl_meas register. (ctrl_meas[7:5] = temperature oversampling rate,
  // ctrl_meas[4:2] = pressure oversampling rate, ctrl_meas[1:0] = mode.)
  ctrlMeas = (TEMP_SAMPLING << 5) | (PRES_SAMPLING << 2) | BME280_MODE;
  // config register. (config[7:5] = standby time, config[4:2] = filter,
  // ctrl_meas[0] = spi enable.)
  config = (BME280_STANDBY_TIME << 5) | (BME280_FILTER << 2);

  WriteRegister(CTRL_HUM_ADDR, ctrlHum);
  WriteRegister(CTRL_MEAS_ADDR, ctrlMeas);
  WriteRegister(CONFIG_ADDR, config);
}

/****************************************************************/
static void ReadTrim() {
  uint8_t ord(0);
  uint8_t m_dig[32];

  // Temp. Dig
  ReadRegister(TEMP_DIG_ADDR, &m_dig[ord], TEMP_DIG_LENGTH);
  ord += TEMP_DIG_LENGTH;

  // Pressure Dig
  ReadRegister(PRESS_DIG_ADDR, &m_dig[ord], PRESS_DIG_LENGTH);
  ord += PRESS_DIG_LENGTH;

  // Humidity Dig 1
  ReadRegister(HUM_DIG_ADDR1, &m_dig[ord], HUM_DIG_ADDR1_LENGTH);
  ord += HUM_DIG_ADDR1_LENGTH;

  // Humidity Dig 2
  ReadRegister(HUM_DIG_ADDR2, &m_dig[ord], HUM_DIG_ADDR2_LENGTH);
  ord += HUM_DIG_ADDR2_LENGTH;

  dig_T1 = (m_dig[1] << 8) | m_dig[0];
  dig_T2 = (m_dig[3] << 8) | m_dig[2];
  dig_T3 = (m_dig[5] << 8) | m_dig[4];

  dig_H1 = m_dig[24];
  dig_H2 = (m_dig[26] << 8) | m_dig[25];
  dig_H3 = m_dig[27];
  dig_H4 = (m_dig[28] << 4) | (0x0F & m_dig[29]);
  dig_H5 = (m_dig[30] << 4) | ((m_dig[29] >> 4) & 0x0F);
  dig_H6 = m_dig[31];

  dig_P1 = (m_dig[7] << 8) | m_dig[6];
  dig_P2 = (m_dig[9] << 8) | m_dig[8];
  dig_P3 = (m_dig[11] << 8) | m_dig[10];
  dig_P4 = (m_dig[13] << 8) | m_dig[12];
  dig_P5 = (m_dig[15] << 8) | m_dig[14];
  dig_P6 = (m_dig[17] << 8) | m_dig[16];
  dig_P7 = (m_dig[19] << 8) | m_dig[18];
  dig_P8 = (m_dig[21] << 8) | m_dig[20];
  dig_P9 = (m_dig[23] << 8) | m_dig[22];
}

// Returns temperature in °C, resolution is 0.01 DegC. Output value of “5123” equals 51.23 °C.
static int32_t CalculateTemperature(int32_t raw, int32_t &t_fine) {
  // Code based on calibration algorthim provided by Bosch.
  int32_t var1, var2, final;
  var1 = ((((raw >> 3) - ((int32_t)dig_T1 << 1))) * ((int32_t)dig_T2)) >> 11;
  var2 =
      (((((raw >> 4) - ((int32_t)dig_T1)) * ((raw >> 4) - ((int32_t)dig_T1))) >>
        12) *
       ((int32_t)dig_T3)) >>
      14;

  t_fine = var1 + var2;
  final = (t_fine * 5 + 128) >> 8;
  return final;
}

// Returns humidity in %RH as unsigned 32 bit integer in Q22.10 format (22 integer and 10 fractional bits).
// Output value of “47445” represents 47445/1024 = 46.333 %RH
static uint16_t CalculateHumidity(uint32_t raw, int32_t t_fine) {
  // Code based on calibration algorthim provided by Bosch.
  int32_t var1;

  var1 = (t_fine - ((int32_t)76800));
  var1 = (((((raw << 14) - (((int32_t)dig_H4) << 20) -
             (((int32_t)dig_H5) * var1)) +
            ((int32_t)16384)) >>
           15) *
          (((((((var1 * ((int32_t)dig_H6)) >> 10) *
               (((var1 * ((int32_t)dig_H3)) >> 11) + ((int32_t)32768))) >>
              10) +
             ((int32_t)2097152)) *
                ((int32_t)dig_H2) +
            8192) >>
           14));
  var1 = (var1 -
          (((((var1 >> 15) * (var1 >> 15)) >> 7) * ((int32_t)dig_H1)) >> 4));
  var1 = (var1 < 0 ? 0 : var1);
  var1 = (var1 > 419430400 ? 419430400 : var1);

  return (uint16_t)(((uint32_t)(var1 >> 12)) / 1024.0 * 100);
}

// Returns pressure in Pa as unsigned 32 bit integer in Q24.8 format (24 integer bits and 8 fractional bits).
// Output value of “24674867” represents 24674867/256 = 96386.2 Pa = 963.862 hPa
static int32_t CalculatePressure(int32_t raw, int32_t t_fine) {
  // Code based on 8bit calibration algorthim provided by Bosch.
  int32_t var1, var2;
  uint32_t p;
  var1 = (((int32_t)t_fine) >> 1) - (int32_t)64000;
  var2 = (((var1 >> 2) * (var1 >> 2)) >> 11) * ((int32_t)dig_P6);
  var2 = var2 + ((var1 * ((int32_t)dig_P5)) << 1);
  var2 = (var2 >> 2) + (((int32_t)dig_P4) << 16);
  var1 = (((dig_P3 * (((var1 >> 2) * (var1 >> 2)) >> 13)) >> 3) +
          ((((int32_t)dig_P2) * var1) >> 1)) >>
         18;
  var1 = ((((32768 + var1)) * ((int32_t)((uint16_t)dig_P1))) >> 15);
  if (var1 == 0)
    return 0;
  p = (((uint32_t)(((int32_t)1048576) - raw) - (var2 >> 12))) * 3125;
  if (p < 0x80000000)
    p = (p << 1) / ((uint32_t)var1);
  else
    p = (p / (uint32_t)var1) * 2;
  var1 = (((int32_t)dig_P9) * ((int32_t)(((p >> 3) * (p >> 3)) >> 13))) >> 12;
  var2 = (((int32_t)(p >> 2)) * ((int32_t)dig_P8)) >> 13;
  p = (uint32_t)((int32_t)p + ((var1 + var2 + dig_P7) >> 4));

  return p;
}

/****************************************************************/
bool BME280_begin() {
  ReadTrim();
  WriteSettings();

  return true;
}

int32_t BME280_pres(int32_t &temp) {
  uint8_t data[SENSOR_DATA_LENGTH];
  ReadRegister(PRESS_ADDR, data, SENSOR_DATA_LENGTH);

  int32_t rawTemp =
      ((int32_t)(data[3]) << 12) | (data[4] << 4) | (data[5] >> 4);
  uint32_t rawPressure =
      ((int32_t)(data[3]) << 12) | (data[4] << 4) | (data[5] >> 4);

  int32_t t_fine;
  temp = CalculateTemperature(rawTemp, t_fine);
  return CalculatePressure(rawPressure, t_fine);
}

uint16_t BME280_hum(int32_t &temp) {
  uint8_t data[SENSOR_DATA_LENGTH];
  ReadRegister(PRESS_ADDR, data, SENSOR_DATA_LENGTH);

  int32_t rawTemp =
      ((int32_t)(data[3]) << 12) | (data[4] << 4) | (data[5] >> 4);
  uint32_t rawHumidity = (data[6] << 8) | data[7];

  int32_t t_fine;
  temp = CalculateTemperature(rawTemp, t_fine);
  return CalculateHumidity(rawHumidity, t_fine);
}

/****************************************************************/
void BME280_read(int32_t &pressure, int32_t &temp, uint16_t &humidity) {
  uint8_t data[SENSOR_DATA_LENGTH];
  ReadRegister(PRESS_ADDR, data, SENSOR_DATA_LENGTH);

  int32_t rawPressure =
      ((int32_t)(data[0]) << 12) | (data[1] << 4) | (data[2] >> 4);
  int32_t rawTemp =
      ((int32_t)(data[3]) << 12) | (data[4] << 4) | (data[5] >> 4);
  uint32_t rawHumidity = (data[6] << 8) | data[7];

  int32_t t_fine;
  temp = CalculateTemperature(rawTemp, t_fine);
  pressure = CalculatePressure(rawPressure, t_fine);
  humidity = CalculateHumidity(rawHumidity, t_fine);
}

uint8_t BME280_chipModel() {

  uint8_t id[1];

  ReadRegister(ID_ADDR, &id[0], 1);

  switch (id[0]) {
  case ChipModel_BME280:
    return ChipModel_BME280;
  case ChipModel_BMP280:
    return ChipModel_BMP280;
  default:
    return ChipModel_UNKNOWN;
  }

  return ChipModel_UNKNOWN;
}