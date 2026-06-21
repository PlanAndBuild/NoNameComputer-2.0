#ifndef __SENSOR_BMP390_H
#define __SENSOR_BMP390_H

#include "main.h"

// BMP390 calibration parameters structure
typedef struct {
  uint16_t T1;
  uint16_t T2;
  int8_t T3;
  int16_t P1;
  int16_t P2;
  int8_t P3;
  int8_t P4;
  uint16_t P5;
  uint16_t P6;
  int8_t P7;
  int8_t P8;
  int16_t P9;
  int8_t P10;
  int8_t P11;
} BMP390_Calib_t;

typedef struct {
  BMP390_Calib_t calib;
  float temperature; // Compensated temperature in Celsius
  float pressure;    // Compensated pressure in Pascals
  float altitude;    // Calculated altitude in meters
} BMP390_t;

HAL_StatusTypeDef BMP390_Init(BMP390_t *dev);
HAL_StatusTypeDef BMP390_Read(BMP390_t *dev);

#endif /* __SENSOR_BMP390_H */
