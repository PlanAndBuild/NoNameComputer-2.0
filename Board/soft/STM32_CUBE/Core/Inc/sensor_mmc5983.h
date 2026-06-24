#ifndef __SENSOR_MMC5983_H
#define __SENSOR_MMC5983_H

#include "main.h"

typedef struct {
  float mx, my, mz; // Magnetic field strength in Gauss
} MMC5983_t;

HAL_StatusTypeDef MMC5983_Init(void);
HAL_StatusTypeDef MMC5983_Read(MMC5983_t *data);

#endif /* __SENSOR_MMC5983_H */
