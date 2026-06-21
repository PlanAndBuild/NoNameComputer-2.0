#include "pca9685.h"

extern I2C_HandleTypeDef hi2c2;

// PCA9685 I2C Address (7-bit = 0x40. 8-bit write = 0x80)
#define PCA9685_I2C_ADDR         0x80

// PCA9685 registers
#define PCA9685_REG_MODE1        0x00
#define PCA9685_REG_MODE2        0x01
#define PCA9685_REG_PRESCALE     0xFE
#define PCA9685_REG_LED0_ON_L    0x06

// Channels
#define PCA9685_BUZZ_CHANNEL     8
#define PCA9685_ARGB_CHANNEL     9

static HAL_StatusTypeDef PCA9685_WriteReg(uint8_t reg, uint8_t val)
{
  uint8_t buf[2] = {reg, val};
  return HAL_I2C_Master_Transmit(&hi2c2, PCA9685_I2C_ADDR, buf, 2, 100);
}

static HAL_StatusTypeDef PCA9685_ReadReg(uint8_t reg, uint8_t *val)
{
  return HAL_I2C_Mem_Read(&hi2c2, PCA9685_I2C_ADDR, reg, I2C_MEMADD_SIZE_8BIT, val, 1, 100);
}

HAL_StatusTypeDef PCA9685_SetPWM(uint8_t channel, uint16_t on_tick, uint16_t off_tick)
{
  uint8_t buf[5];
  buf[0] = PCA9685_REG_LED0_ON_L + (channel * 4);
  buf[1] = on_tick & 0xFF;
  buf[2] = (on_tick >> 8) & 0xFF;
  buf[3] = off_tick & 0xFF;
  buf[4] = (off_tick >> 8) & 0xFF;
  return HAL_I2C_Master_Transmit(&hi2c2, PCA9685_I2C_ADDR, buf, 5, 100);
}

static HAL_StatusTypeDef PCA9685_SetPrescaler(uint8_t prescale)
{
  uint8_t mode1 = 0;
  
  // 1. Read Mode 1
  if (PCA9685_ReadReg(PCA9685_REG_MODE1, &mode1) != HAL_OK) return HAL_ERROR;
  
  // 2. Put oscillator to sleep to write prescale
  uint8_t sleep_mode = (mode1 & 0x7F) | 0x10; // set SLEEP bit
  if (PCA9685_WriteReg(PCA9685_REG_MODE1, sleep_mode) != HAL_OK) return HAL_ERROR;
  
  // 3. Write prescaler
  if (PCA9685_WriteReg(PCA9685_REG_PRESCALE, prescale) != HAL_OK) return HAL_ERROR;
  
  // 4. Wake up oscillator
  if (PCA9685_WriteReg(PCA9685_REG_MODE1, mode1 & 0xEF) != HAL_OK) return HAL_ERROR;
  HAL_Delay(1); // Wait for oscillator stabilization
  
  // 5. Re-enable auto-increment
  return PCA9685_WriteReg(PCA9685_REG_MODE1, mode1 | 0xA0);
}

HAL_StatusTypeDef PCA9685_Init(void)
{
  // 1. Wake up chip and enable auto-increment
  if (PCA9685_WriteReg(PCA9685_REG_MODE1, 0x20) != HAL_OK) return HAL_ERROR; // Auto-increment=1, Sleep=0
  if (PCA9685_WriteReg(PCA9685_REG_MODE2, 0x04) != HAL_OK) return HAL_ERROR; // Totem pole structure (outdrv=1)

  // 2. Set default PWM frequency to 50Hz (standard for RC Servos)
  // prescale = round(25000000 / (4096 * 50)) - 1 = 121
  if (PCA9685_SetPrescaler(121) != HAL_OK) return HAL_ERROR;

  // Initialize all servos to 90 degrees (middle position)
  for (uint8_t i = 0; i < 8; i++)
  {
    PCA9685_SetServo(i, 90.0f);
  }
  
  // Turn off buzzer and ARGB
  PCA9685_BuzzerOff();
  PCA9685_ARGB_Low();

  return HAL_OK;
}

HAL_StatusTypeDef PCA9685_SetServo(uint8_t channel, float angle)
{
  if (channel > 7) return HAL_ERROR;
  if (angle < 0.0f) angle = 0.0f;
  if (angle > 180.0f) angle = 180.0f;

  // Servos operate at 50Hz (20ms cycle). 
  // Pulse width is 1.0ms (0 deg) to 2.0ms (180 deg).
  // 1.0ms is 1.0/20 * 4096 = 205 ticks.
  // 2.0ms is 2.0/20 * 4096 = 410 ticks.
  // We map 0-180 deg to 205-410 ticks.
  uint16_t ticks = (uint16_t)(205.0f + (angle / 180.0f) * 205.0f);
  
  return PCA9685_SetPWM(channel, 0, ticks);
}

HAL_StatusTypeDef PCA9685_BuzzerSetFrequency(float frequency_hz)
{
  if (frequency_hz < 24.0f) frequency_hz = 24.0f;
  if (frequency_hz > 1526.0f) frequency_hz = 1526.0f;

  // Calculate prescaler for desired buzzer tone
  uint8_t prescale = (uint8_t)(roundf(25000000.0f / (4096.0f * frequency_hz)) - 1);
  return PCA9685_SetPrescaler(prescale);
}

HAL_StatusTypeDef PCA9685_BuzzerOn(void)
{
  // 50% duty cycle (4096 / 2 = 2048 ticks) for square wave sound
  return PCA9685_SetPWM(PCA9685_BUZZ_CHANNEL, 0, 2048);
}

HAL_StatusTypeDef PCA9685_BuzzerOff(void)
{
  // 0% duty cycle (off)
  return PCA9685_SetPWM(PCA9685_BUZZ_CHANNEL, 0, 0);
}

HAL_StatusTypeDef PCA9685_ARGB_High(void)
{
  // 100% duty cycle (on_tick=4096, off_tick=0 is fully ON)
  // In PCA9685, setting full ON is done by setting bit 4 of LEDn_ON_H
  return PCA9685_SetPWM(PCA9685_ARGB_CHANNEL, 0x1000, 0);
}

HAL_StatusTypeDef PCA9685_ARGB_Low(void)
{
  // 0% duty cycle (fully OFF)
  return PCA9685_SetPWM(PCA9685_ARGB_CHANNEL, 0, 0x1000);
}
