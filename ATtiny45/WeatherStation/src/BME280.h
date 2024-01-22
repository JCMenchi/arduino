/*

BME280.h

This code records data from the BME280 sensor and provides an API.
This file is part of the Arduino BME280 library.
Copyright (C) 2016  Tyler Glenn

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.

Written: Dec 30 2015.
Last Updated: Oct 07 2017.

This code is licensed under the GNU LGPL and is open for ditrbution
and copying in accordance with the license.
This header must be included in any derived code or copies of the code.

 */

#ifndef TG_BME_280_H
#define TG_BME_280_H

#include <stdint.h>

//////////////////////////////////////////////////////////////////
/// BME280 - Driver class for Bosch Bme280 sensor
///
/// Based on the data sheet provided by Bosch for
/// the Bme280 environmental sensor.
///
class BME280
{
public:

/*****************************************************************/
/* ENUMERATIONS                                                  */
/*****************************************************************/

   enum ChipModel
   {
      ChipModel_UNKNOWN = 0,
      ChipModel_BMP280 = 0x58,
      ChipModel_BME280 = 0x60
   };

/*****************************************************************/
/* INIT FUNCTIONS                                                */
/*****************************************************************/

   /////////////////////////////////////////////////////////////////
   /// Constructor used to create the class.
   /// All parameters have default values.
   BME280() {}

   /////////////////////////////////////////////////////////////////
   /// Method used to initialize the class.
   bool begin();

/*****************************************************************/
/* ENVIRONMENTAL FUNCTIONS                                       */
/*****************************************************************/

   /////////////////////////////////////////////////////////////////
   /// Read the pressure from the BME280 and return a float with the
   /// specified unit.
   int32_t pres(int32_t& temp);

   /////////////////////////////////////////////////////////////////
   /// Read the humidity from the BME280 and return a percentage
   /// as a float.
   uint16_t hum(int32_t& temp);

   /////////////////////////////////////////////////////////////////
   /// Read the data from the BME280 in the specified unit.
   void read(
      int32_t&    pressure,
      int32_t&    temperature,
      uint16_t&    humidity);

/*****************************************************************/
/* ACCESSOR FUNCTIONS                                            */
/*****************************************************************/

   ////////////////////////////////////////////////////////////////
   /// Method used to return ChipModel.
   ChipModel chipModel();

protected:

/*****************************************************************/
/* CONSTRUCTOR INIT FUNCTIONS                                    */
/*****************************************************************/


   ///////////////////////////////////////////////////////////////
   /// Force a unfiltered measurement to populate the filter 
   /// buffer.
   void InitializeFilter();

private:

/*****************************************************************/
/* CONSTANTS                                                     */
/*****************************************************************/

   static const uint8_t CTRL_HUM_ADDR   = 0xF2;
   static const uint8_t CTRL_MEAS_ADDR  = 0xF4;
   static const uint8_t CONFIG_ADDR     = 0xF5;
   static const uint8_t PRESS_ADDR      = 0xF7;
   static const uint8_t TEMP_ADDR       = 0xFA;
   static const uint8_t HUM_ADDR        = 0xFD;
   static const uint8_t TEMP_DIG_ADDR   = 0x88;
   static const uint8_t PRESS_DIG_ADDR  = 0x8E;
   static const uint8_t HUM_DIG_ADDR1   = 0xA1;
   static const uint8_t HUM_DIG_ADDR2   = 0xE1;
   static const uint8_t ID_ADDR         = 0xD0;

   static const uint8_t TEMP_DIG_LENGTH         = 6;
   static const uint8_t PRESS_DIG_LENGTH        = 18;
   static const uint8_t HUM_DIG_ADDR1_LENGTH    = 1;
   static const uint8_t HUM_DIG_ADDR2_LENGTH    = 7;
   static const uint8_t DIG_LENGTH              = 32;
   static const uint8_t SENSOR_DATA_LENGTH      = 8;


/*****************************************************************/
/* VARIABLES                                                     */
/*****************************************************************/
   uint8_t m_dig[32];

/*****************************************************************/
/* ABSTRACT FUNCTIONS                                            */
/*****************************************************************/

   /////////////////////////////////////////////////////////////////
   /// Write values to BME280 registers.
   virtual bool WriteRegister(uint8_t addr, uint8_t data);

   /////////////////////////////////////////////////////////////////
   /// Read values from BME280 registers.
   virtual bool ReadRegister(
      uint8_t addr,
      uint8_t data[],
      uint8_t length);


/*****************************************************************/
/* WORKER FUNCTIONS                                              */
/*****************************************************************/

   /////////////////////////////////////////////////////////////////
   /// Write the settings to the chip.
   void WriteSettings(bool filter_off = false);

   /////////////////////////////////////////////////////////////////
   /// Read the the trim data from the BME280, return true if
   /// successful.
   bool ReadTrim();

   /////////////////////////////////////////////////////////////////
   /// Read the raw data from the BME280 into an array and return
   /// true if successful.
   bool ReadData(
      int32_t data[8]);


   /////////////////////////////////////////////////////////////////
   /// Calculate the temperature from the BME280 raw data and
   /// BME280 trim, return a float.
   int32_t CalculateTemperature(
      int32_t raw,
      int32_t& t_fine);

   /////////////////////////////////////////////////////////////////
   /// Calculate the humidity from the BME280 raw data and BME280
   /// trim, return a float.
   uint16_t CalculateHumidity(
      uint32_t raw,
      int32_t t_fine);

   /////////////////////////////////////////////////////////////////
   /// Calculate the pressure from the BME280 raw data and BME280
   /// trim, return a float.
   int32_t CalculatePressure(
      int32_t raw,
      int32_t t_fine);

};

#endif // TG_BME_280_H
