#ifndef __SENSOR_H3LIS200_H
#define __SENSOR_H3LIS200_H

#include "main.h"

typedef struct {
  float ax, ay, az; // Acceleration in g (up to +/-200g)
} H3LIS200_t;

HAL_StatusTypeDef H3LIS200_Init(void);
HAL_StatusTypeDef H3LIS200_Read(H3LIS200_t *data);

#endif /* __SENSOR_H3LIS200_H */
