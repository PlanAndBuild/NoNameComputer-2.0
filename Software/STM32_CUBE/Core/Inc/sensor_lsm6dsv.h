#ifndef __SENSOR_LSM6DSV_H
#define __SENSOR_LSM6DSV_H

#include "main.h"

typedef struct {
  float ax, ay, az; // Acceleration in g
  float gx, gy, gz; // Angular rate in deg/s
} LSM6DSV_t;

HAL_StatusTypeDef LSM6DSV_Init(void);
HAL_StatusTypeDef LSM6DSV_Read(LSM6DSV_t *data);

#endif /* __SENSOR_LSM6DSV_H */
