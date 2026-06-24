#include "sensor_h3lis200.h"
#include "spi_common.h"

// H3LIS200 Registers
#define H3LIS200_REG_WHO_AM_I     0x0F
#define H3LIS200_REG_CTRL_REG1    0x20
#define H3LIS200_REG_CTRL_REG4    0x23
#define H3LIS200_REG_OUT_X_L      0x28

#define H3LIS200_WHO_AM_I_VAL     0x32

// Sensitivity: 12-bit output, left-justified. 
// For +/- 200g scale, sensitivity is approx 98 mg/digit (or 0.098 g/digit).
#define H3LIS200_SENSITIVITY      0.098f 

// H3LIS200 SPI Read helper: Multi-byte read requires setting the MSB (bit 7) and 
// standard SPI reads set bit 7 (read) and bit 6 (address increment). 
// So, for reading multiple registers starting at 0x28, we must transmit: address | 0x80 | 0x40 = address | 0xC0.
static HAL_StatusTypeDef H3LIS200_SPI_ReadBuffer(uint8_t reg, uint8_t *buffer, uint16_t len)
{
  HAL_StatusTypeDef status;
  // Multi-byte read address byte format: READ (0x80) | MS (0x40) | Register Address
  uint8_t tx_byte = reg | 0xC0;

  SPI_CS_Select(CS_H3L_GPIO_Port, CS_H3L_Pin);
  status = SPI_Transmit(&tx_byte, 1, 50);
  if (status == HAL_OK)
  {
    status = SPI_Receive(buffer, len, 100);
  }
  SPI_CS_Deselect(CS_H3L_GPIO_Port, CS_H3L_Pin);
  
  return status;
}

HAL_StatusTypeDef H3LIS200_Init(void)
{
  uint8_t who_am_i = 0;
  
  // 1. Verify WHO_AM_I
  if (SPI_ReadRegister(CS_H3L_GPIO_Port, CS_H3L_Pin, H3LIS200_REG_WHO_AM_I, &who_am_i) != HAL_OK)
  {
    return HAL_ERROR;
  }
  if (who_am_i != H3LIS200_WHO_AM_I_VAL)
  {
    return HAL_ERROR;
  }

  // 2. Configure CTRL_REG1: Normal power mode, 100Hz ODR, all axes enabled
  // PM[2:0] = 001 (Normal Mode), DR[1:0] = 00 (50Hz) or 01 (100Hz), Zen=1, Yen=1, Xen=1
  // PM=001 (Normal), DR=01 (100Hz), Axes=111 -> 0b00101111 = 0x2F
  SPI_WriteRegister(CS_H3L_GPIO_Port, CS_H3L_Pin, H3LIS200_REG_CTRL_REG1, 0x2F);

  // 3. Configure CTRL_REG4: Block Data Update (BDU) enabled, scale +/-200g
  // BDU=1 (0x80), BLE=0, FS[1:0]=00 (+/-100g) or 01 (+/-200g), SIM=0
  // FS = +/-200g is 0x10 or 0x30 depending on submodel (H3LIS200 is fixed at +/-200g, FS bits set to 0b01 or 0b11)
  // Let's set FS = 200g (FS[1:0] = 0b01, which is 0x10) and BDU=1 (0x80) -> 0x90
  SPI_WriteRegister(CS_H3L_GPIO_Port, CS_H3L_Pin, H3LIS200_REG_CTRL_REG4, 0x90);

  return HAL_OK;
}

HAL_StatusTypeDef H3LIS200_Read(H3LIS200_t *data)
{
  uint8_t buffer[6] = {0};
  int16_t raw_x, raw_y, raw_z;

  // Read 6 bytes starting from OUT_X_L (X, Y, Z raw values, each 2 bytes, left-justified)
  if (H3LIS200_SPI_ReadBuffer(H3LIS200_REG_OUT_X_L, buffer, 6) != HAL_OK)
  {
    return HAL_ERROR;
  }

  // Values are 12-bit left-justified in 16-bit registers (lower 4 bits are 0).
  // Shift right by 4 to get the raw 12-bit signed value.
  raw_x = (int16_t)((buffer[1] << 8) | buffer[0]) >> 4;
  raw_y = (int16_t)((buffer[3] << 8) | buffer[2]) >> 4;
  raw_z = (int16_t)((buffer[5] << 8) | buffer[4]) >> 4;

  // Convert to g
  data->ax = (float)raw_x * H3LIS200_SENSITIVITY;
  data->ay = (float)raw_y * H3LIS200_SENSITIVITY;
  data->az = (float)raw_z * H3LIS200_SENSITIVITY;

  return HAL_OK;
}
