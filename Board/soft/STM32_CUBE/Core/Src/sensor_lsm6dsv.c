#include "sensor_lsm6dsv.h"
#include "spi_common.h"

// LSM6DSV Registers
#define LSM6DSV_REG_WHO_AM_I     0x0F
#define LSM6DSV_REG_CTRL1        0x10 // Accelerometer ODR & scale
#define LSM6DSV_REG_CTRL2        0x11 // Gyroscope ODR & scale
#define LSM6DSV_REG_CTRL3        0x12 // Control Register 3 (BDU, Software Reset)
#define LSM6DSV_REG_OUTX_L_G     0x22 // Gyro X LSB
#define LSM6DSV_REG_OUTX_L_A     0x28 // Accel X LSB

#define LSM6DSV_WHO_AM_I_VAL     0x70

// Scale sensitivities
#define LSM6DSV_ACCEL_16G_SENS   0.488f  // mg/LSB for 16g range
#define LSM6DSV_GYRO_2000_SENS   70.0f   // mdps/LSB for 2000dps range

HAL_StatusTypeDef LSM6DSV_Init(void)
{
  uint8_t who_am_i = 0;
  
  // 1. Verify connection
  if (SPI_ReadRegister(CS_LSM1_GPIO_Port, CS_LSM1_Pin, LSM6DSV_REG_WHO_AM_I, &who_am_i) != HAL_OK)
  {
    return HAL_ERROR;
  }
  if (who_am_i != LSM6DSV_WHO_AM_I_VAL)
  {
    return HAL_ERROR;
  }

  // 2. Software reset
  SPI_WriteRegister(CS_LSM1_GPIO_Port, CS_LSM1_Pin, LSM6DSV_REG_CTRL3, 0x01); // SW_RESET
  HAL_Delay(10);

  // 3. Enable Block Data Update (BDU)
  SPI_WriteRegister(CS_LSM1_GPIO_Port, CS_LSM1_Pin, LSM6DSV_REG_CTRL3, 0x40); // BDU=1

  // 4. Configure Accelerometer: 120Hz ODR, +/-16g scale
  // CTRL1 format: ODR_XL[3:0] | FS_XL[1:0] | LPF2_XL_EN | HP_PATH_XL_EN
  // ODR = 120 Hz (0x04)
  // FS = +/-16g (0x02)
  SPI_WriteRegister(CS_LSM1_GPIO_Port, CS_LSM1_Pin, LSM6DSV_REG_CTRL1, (0x04 << 4) | (0x02 << 2));

  // 5. Configure Gyroscope: 120Hz ODR, +/-2000dps scale
  // CTRL2 format: ODR_G[3:0] | FS_G[1:0] | 0 | 0
  // ODR = 120 Hz (0x04)
  // FS = +/-2000dps (0x03)
  SPI_WriteRegister(CS_LSM1_GPIO_Port, CS_LSM1_Pin, LSM6DSV_REG_CTRL2, (0x04 << 4) | (0x03 << 2));

  return HAL_OK;
}

HAL_StatusTypeDef LSM6DSV_Read(LSM6DSV_t *data)
{
  uint8_t buffer[12] = {0};
  int16_t raw_gx, raw_gy, raw_gz;
  int16_t raw_ax, raw_ay, raw_az;

  // Read 12 bytes of data starting from OUTX_L_G (6 bytes gyro, 6 bytes accel)
  if (SPI_ReadBuffer(CS_LSM1_GPIO_Port, CS_LSM1_Pin, LSM6DSV_REG_OUTX_L_G, buffer, 12) != HAL_OK)
  {
    return HAL_ERROR;
  }

  // Decode Gyroscope raw values
  raw_gx = (int16_t)((buffer[1] << 8) | buffer[0]);
  raw_gy = (int16_t)((buffer[3] << 8) | buffer[2]);
  raw_gz = (int16_t)((buffer[5] << 8) | buffer[4]);

  // Decode Accelerometer raw values
  raw_ax = (int16_t)((buffer[7] << 8) | buffer[6]);
  raw_ay = (int16_t)((buffer[9] << 8) | buffer[8]);
  raw_az = (int16_t)((buffer[11] << 8) | buffer[10]);

  // Convert to physical units (g and dps)
  data->ax = (float)raw_ax * LSM6DSV_ACCEL_16G_SENS / 1000.0f;
  data->ay = (float)raw_ay * LSM6DSV_ACCEL_16G_SENS / 1000.0f;
  data->az = (float)raw_az * LSM6DSV_ACCEL_16G_SENS / 1000.0f;

  data->gx = (float)raw_gx * LSM6DSV_GYRO_2000_SENS / 1000.0f;
  data->gy = (float)raw_gy * LSM6DSV_GYRO_2000_SENS / 1000.0f;
  data->gz = (float)raw_gz * LSM6DSV_GYRO_2000_SENS / 1000.0f;

  return HAL_OK;
}
