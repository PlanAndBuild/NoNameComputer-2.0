#ifndef __PCA9685_H
#define __PCA9685_H

#include "main.h"

// Initialize PCA9685 driver at 50Hz for Servos
HAL_StatusTypeDef PCA9685_Init(void);

// Set servo output in degrees (0 to 180). Maps to 1ms to 2ms pulses.
HAL_StatusTypeDef PCA9685_SetServo(uint8_t channel, float angle);

// Set duty cycle directly on any PCA9685 channel (0 to 15, value 0 to 4095)
HAL_StatusTypeDef PCA9685_SetPWM(uint8_t channel, uint16_t on_tick, uint16_t off_tick);

// Configure buzzer frequency (frequency_hz) and enable/disable it
HAL_StatusTypeDef PCA9685_BuzzerSetFrequency(float frequency_hz);
HAL_StatusTypeDef PCA9685_BuzzerOn(void);
HAL_StatusTypeDef PCA9685_BuzzerOff(void);

// ARGB (WS2812 pin) toggling
HAL_StatusTypeDef PCA9685_ARGB_High(void);
HAL_StatusTypeDef PCA9685_ARGB_Low(void);

#endif /* __PCA9685_H */
