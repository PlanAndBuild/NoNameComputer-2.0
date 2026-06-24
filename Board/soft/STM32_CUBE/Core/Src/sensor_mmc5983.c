#include "sensor_mmc5983.h"
#include "spi_common.h"

// MMC5983MA Registers
#define MMC5983_REG_XOUT_0        0x00 // Xout [17:10]
#define MMC5983_REG_XOUT_1        0x01 // Xout [9:2]
#define MMC5983_REG_YOUT_0        0x02
#define MMC5983_REG_YOUT_1        0x03
#define MMC5983_REG_ZOUT_0        0x04
#define MMC5983_REG_ZOUT_1        0x05
#define MMC5983_REG_XYZOUT_2      0x06 // LSBS [1:0] for X, Y, Z
#define MMC5983_REG_STATUS        0x08
#define MMC5983_REG_CTRL0         0x09
#define MMC5983_REG_CTRL1         0x0A
#define MMC5983_REG_CTRL2         0x0B
#define MMC5983_REG_WHO_AM_I      0x2F

#define MMC5983_WHO_AM_I_VAL      0x30

// Conversion Factor: MMC5983MA 16-bit output has sensitivity 4096 LSB/Gauss
#define MMC5983_SENSITIVITY       4096.0f

HAL_StatusTypeDef MMC5983_Init(void)
{
  uint8_t who_am_i = 0;
  
  // 1. Verify WHO_AM_I
  if (SPI_ReadRegister(CS_MAG_GPIO_Port, CS_MAG_Pin, MMC5983_REG_WHO_AM_I, &who_am_i) != HAL_OK)
  {
    return HAL_ERROR;
  }
  if (who_am_i != MMC5983_WHO_AM_I_VAL)
  {
    return HAL_ERROR;
  }

  // 2. Perform software reset (CTRL1: SW_RST bit)
  SPI_WriteRegister(CS_MAG_GPIO_Port, CS_MAG_Pin, MMC5983_REG_CTRL1, 0x80);
  HAL_Delay(15);

  // 3. Clear reset and setup CTRL0 for auto-set/reset (best performance)
  // CTRL0 set/reset bits: Set=1 (0x08), Reset=1 (0x10) -> 0x18
  // Let's perform a SET operation to align magnetic domains initially
  SPI_WriteRegister(CS_MAG_GPIO_Port, CS_MAG_Pin, MMC5983_REG_CTRL0, 0x08); // SET
  HAL_Delay(1);

  // 4. Configure CTRL2 for continuous mode (ODR = 50 Hz)
  // CM_EN = 1 (Continuous Mode Enable - 0x80)
  // ODR = 50Hz (0x04) -> 0x84
  SPI_WriteRegister(CS_MAG_GPIO_Port, CS_MAG_Pin, MMC5983_REG_CTRL2, 0x84);

  return HAL_OK;
}

HAL_StatusTypeDef MMC5983_Read(MMC5983_t *data)
{
  uint8_t buffer[7] = {0};
  uint32_t raw_x, raw_y, raw_z;

  // In continuous mode, check the Status register (0x08) to see if Meas_M_Done (bit 0) is set.
  uint8_t status = 0;
  uint8_t timeout = 50;
  do {
    if (SPI_ReadRegister(CS_MAG_GPIO_Port, CS_MAG_Pin, MMC5983_REG_STATUS, &status) != HAL_OK)
    {
      return HAL_ERROR;
    }
    HAL_Delay(1);
    timeout--;
  } while (!(status & 0x01) && timeout > 0);

  if (timeout == 0) return HAL_TIMEOUT;

  // Read X, Y, Z raw values (7 bytes: 0x00 to 0x06)
  if (SPI_ReadBuffer(CS_MAG_GPIO_Port, CS_MAG_Pin, MMC5983_REG_XOUT_0, buffer, 7) != HAL_OK)
  {
    return HAL_ERROR;
  }

  // Parse 18-bit values:
  // X = [XOUT0][XOUT1][XYZOUT2 bits 7:6]
  raw_x = (uint32_t)((buffer[0] << 10) | (buffer[1] << 2) | (buffer[6] >> 6));
  raw_y = (uint32_t)((buffer[2] << 10) | (buffer[3] << 2) | ((buffer[6] >> 4) & 0x03));
  raw_z = (uint32_t)((buffer[4] << 10) | (buffer[5] << 2) | ((buffer[6] >> 2) & 0x03));

  // Shift to 18-bit signed centering:
  // Center range is 2^17 (131072)
  int32_t centered_x = (int32_t)raw_x - 131072;
  int32_t centered_y = (int32_t)raw_y - 131072;
  int32_t centered_z = (int32_t)raw_z - 131072;

  // Convert to Gauss:
  // For 18-bit mode, sensitivity is 16384 LSB/G.
  data->mx = (float)centered_x / 16384.0f;
  data->my = (float)centered_y / 16384.0f;
  data->mz = (float)centered_z / 16384.0f;

  return HAL_OK;
}
