#include "sensor_lsm6dso.h"
#include "spi_common.h"

// LSM6DSO Registers
#define LSM6DSO_REG_WHO_AM_I     0x0F
#define LSM6DSO_REG_CTRL1_XL     0x10 // Accelerometer control
#define LSM6DSO_REG_CTRL2_G      0x11 // Gyroscope control
#define LSM6DSO_REG_CTRL3_C      0x12 // Control Register 3 (BDU, SW reset)
#define LSM6DSO_REG_OUTX_L_G     0x22 // Gyro X output
#define LSM6DSO_REG_OUTX_L_A     0x28 // Accel X output

#define LSM6DSO_WHO_AM_I_VAL     0x6C // WHO_AM_I value for LSM6DSO

// Sensitivities
#define LSM6DSO_ACCEL_16G_SENS   0.488f  // mg/LSB for 16g range
#define LSM6DSO_GYRO_2000_SENS   70.0f   // mdps/LSB for 2000dps range

HAL_StatusTypeDef LSM6DSO_Init(void)
{
  uint8_t who_am_i = 0;
  
  // 1. Check chip ID
  if (SPI_ReadRegister(CS_LSM2_GPIO_Port, CS_LSM2_Pin, LSM6DSO_REG_WHO_AM_I, &who_am_i) != HAL_OK)
  {
    return HAL_ERROR;
  }
  if (who_am_i != LSM6DSO_WHO_AM_I_VAL)
  {
    return HAL_ERROR;
  }

  // 2. Soft reset
  SPI_WriteRegister(CS_LSM2_GPIO_Port, CS_LSM2_Pin, LSM6DSO_REG_CTRL3_C, 0x01); // SW_RESET
  HAL_Delay(10);

  // 3. Enable Block Data Update (BDU)
  SPI_WriteRegister(CS_LSM2_GPIO_Port, CS_LSM2_Pin, LSM6DSO_REG_CTRL3_C, 0x40); // BDU=1

  // 4. Configure Accel: 104Hz ODR, +/-16g scale
  // ODR_XL = 104Hz (0x04)
  // FS_XL = +/-16g (0x02) (0x04 << 4 | 0x02 << 2 = 0x48)
  SPI_WriteRegister(CS_LSM2_GPIO_Port, CS_LSM2_Pin, LSM6DSO_REG_CTRL1_XL, 0x48);

  // 5. Configure Gyro: 104Hz ODR, +/-2000dps scale
  // ODR_G = 104Hz (0x04)
  // FS_G = 2000dps (0x03) (0x04 << 4 | 0x03 << 2 = 0x4C)
  SPI_WriteRegister(CS_LSM2_GPIO_Port, CS_LSM2_Pin, LSM6DSO_REG_CTRL2_G, 0x4C);

  return HAL_OK;
}

HAL_StatusTypeDef LSM6DSO_Read(LSM6DSO_t *data)
{
  uint8_t buffer[12] = {0};
  int16_t raw_gx, raw_gy, raw_gz;
  int16_t raw_ax, raw_ay, raw_az;

  // Read 12 bytes starting from OUTX_L_G (6 bytes gyro, 6 bytes accel)
  if (SPI_ReadBuffer(CS_LSM2_GPIO_Port, CS_LSM2_Pin, LSM6DSO_REG_OUTX_L_G, buffer, 12) != HAL_OK)
  {
    return HAL_ERROR;
  }

  // Gyro raw values
  raw_gx = (int16_t)((buffer[1] << 8) | buffer[0]);
  raw_gy = (int16_t)((buffer[3] << 8) | buffer[2]);
  raw_gz = (int16_t)((buffer[5] << 8) | buffer[4]);

  // Accel raw values
  raw_ax = (int16_t)((buffer[7] << 8) | buffer[6]);
  raw_ay = (int16_t)((buffer[9] << 8) | buffer[8]);
  raw_az = (int16_t)((buffer[11] << 8) | buffer[10]);

  // Sensitivities conversion
  data->ax = (float)raw_ax * LSM6DSO_ACCEL_16G_SENS / 1000.0f;
  data->ay = (float)raw_ay * LSM6DSO_ACCEL_16G_SENS / 1000.0f;
  data->az = (float)raw_az * LSM6DSO_ACCEL_16G_SENS / 1000.0f;

  data->gx = (float)raw_gx * LSM6DSO_GYRO_2000_SENS / 1000.0f;
  data->gy = (float)raw_gy * LSM6DSO_GYRO_2000_SENS / 1000.0f;
  data->gz = (float)raw_gz * LSM6DSO_GYRO_2000_SENS / 1000.0f;

  return HAL_OK;
}
