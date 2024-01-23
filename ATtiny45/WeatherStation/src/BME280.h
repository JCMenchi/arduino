#ifndef TG_BME_280_H
#define TG_BME_280_H

#include <stdint.h>

#define ChipModel_UNKNOWN 0
#define ChipModel_BMP280 0x58
#define ChipModel_BME280 0x60

bool BME280_begin();

int32_t BME280_pres(int32_t &temp);
uint16_t BME280_hum(int32_t &temp);
void BME280_read(int32_t &pressure, int32_t &temperature, uint16_t &humidity);

uint8_t BME280_chipModel();

#endif // TG_BME_280_H
