#ifndef __SENSOR_INA3221_H
#define __SENSOR_INA3221_H

#include "main.h"

typedef struct {
  float bus_voltage[3];   // Bus voltages in Volts (e.g. system 5V, 3.3V, Battery)
  float shunt_voltage[3]; // Shunt voltages in milliVolts
  float current[3];       // Currents in Amperes
} INA3221_t;

HAL_StatusTypeDef INA3221_Init(void);
HAL_StatusTypeDef INA3221_Read(INA3221_t *data);

#endif /* __SENSOR_INA3221_H */
