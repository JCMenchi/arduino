#ifndef TG_BME_280_H
#define TG_BME_280_H

#include <stdint.h>

#define ChipModel_UNKNOWN 0
#define ChipModel_BMP280 0x58
#define ChipModel_BME280 0x60

bool BME280_begin();

// Temperature in °C, resolution is 0.01 DegC. 
//    Output value of “5123” equals 51.23 °C.
//
// Humidity in %RH as unsigned 32 bit integer in Q22.10 format (22 integer and 10 fractional bits).
//    Output value of “47445” represents 47445/1024 = 46.333 %RH
//
// Pressure in Pa as unsigned 32 bit integer in Q24.8 format (24 integer bits and 8 fractional bits).
//    Output value of “24674867” represents 24674867/256 = 96386.2 Pa = 963.862 hPa

int32_t BME280_pres(int32_t &temp);

uint16_t BME280_hum(int32_t &temp);

void BME280_read(int32_t &pressure, int32_t &temperature, uint16_t &humidity);

uint8_t BME280_chipModel();

#endif // TG_BME_280_H
