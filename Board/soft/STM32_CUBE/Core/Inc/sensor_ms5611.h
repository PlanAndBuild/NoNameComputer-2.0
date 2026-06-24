#ifndef __SENSOR_MS5611_H
#define __SENSOR_MS5611_H

#include "main.h"

typedef struct {
  uint16_t c[8]; // Calibration coefficients C1-C6 (index 1 to 6)
  int32_t temp;  // Compensated temperature * 100
  int32_t press; // Compensated pressure (Pa)
  float altitude; // Calculated altitude in meters
} MS5611_t;

HAL_StatusTypeDef MS5611_Init(MS5611_t *dev);
HAL_StatusTypeDef MS5611_Read(MS5611_t *dev);

#endif /* __SENSOR_MS5611_H */
