#ifndef __SENSOR_LSM6DSO_H
#define __SENSOR_LSM6DSO_H

#include "main.h"

typedef struct {
  float ax, ay, az; // Acceleration in g
  float gx, gy, gz; // Angular rate in deg/s
} LSM6DSO_t;

HAL_StatusTypeDef LSM6DSO_Init(void);
HAL_StatusTypeDef LSM6DSO_Read(LSM6DSO_t *data);

#endif /* __SENSOR_LSM6DSO_H */
