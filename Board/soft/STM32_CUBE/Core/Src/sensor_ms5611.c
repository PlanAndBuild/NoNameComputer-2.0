#include "sensor_ms5611.h"
#include "spi_common.h"
#include <math.h>

#define MS5611_CMD_RESET       0x1E
#define MS5611_CMD_ADC_READ    0x00
#define MS5611_CMD_CONV_D1_4096 0x48 // Pressure conversion OSR 4096
#define MS5611_CMD_CONV_D2_4096 0x58 // Temperature conversion OSR 4096
#define MS5611_CMD_PROM_RD     0xA0

static HAL_StatusTypeDef MS5611_Reset(void)
{
  uint8_t cmd = MS5611_CMD_RESET;
  SPI_CS_Select(CS_MS5611_GPIO_Port, CS_MS5611_Pin);
  HAL_StatusTypeDef status = SPI_Transmit(&cmd, 1, 50);
  SPI_CS_Deselect(CS_MS5611_GPIO_Port, CS_MS5611_Pin);
  HAL_Delay(10); // Wait for reset reload sequence
  return status;
}

static uint16_t MS5611_ReadPROM(uint8_t address)
{
  uint8_t cmd = MS5611_CMD_PROM_RD + (address << 1);
  uint8_t rx_buf[2] = {0};
  
  SPI_CS_Select(CS_MS5611_GPIO_Port, CS_MS5611_Pin);
  SPI_Transmit(&cmd, 1, 50);
  SPI_Receive(rx_buf, 2, 50);
  SPI_CS_Deselect(CS_MS5611_GPIO_Port, CS_MS5611_Pin);
  
  return (uint16_t)((rx_buf[0] << 8) | rx_buf[1]);
}

HAL_StatusTypeDef MS5611_Init(MS5611_t *dev)
{
  HAL_StatusTypeDef status = MS5611_Reset();
  if (status != HAL_OK) return status;

  // Read PROM values (C1-C6)
  for (uint8_t i = 1; i <= 6; i++)
  {
    dev->c[i] = MS5611_ReadPROM(i);
  }
  
  // Basic sanity check: C1-C6 should not be 0 or 0xFFFF
  if (dev->c[1] == 0 || dev->c[1] == 0xFFFF)
  {
    return HAL_ERROR;
  }
  
  return HAL_OK;
}

static uint32_t MS5611_GetADCValue(uint8_t conv_cmd)
{
  uint8_t cmd = conv_cmd;
  uint8_t rx_buf[3] = {0};

  // 1. Trigger conversion
  SPI_CS_Select(CS_MS5611_GPIO_Port, CS_MS5611_Pin);
  SPI_Transmit(&cmd, 1, 50);
  SPI_CS_Deselect(CS_MS5611_GPIO_Port, CS_MS5611_Pin);

  // 2. Wait for conversion to finish (approx. 9.04ms for OSR 4096)
  HAL_Delay(10);

  // 3. Read ADC
  cmd = MS5611_CMD_ADC_READ;
  SPI_CS_Select(CS_MS5611_GPIO_Port, CS_MS5611_Pin);
  SPI_Transmit(&cmd, 1, 50);
  SPI_Receive(rx_buf, 3, 50);
  SPI_CS_Deselect(CS_MS5611_GPIO_Port, CS_MS5611_Pin);

  return (uint32_t)((rx_buf[0] << 16) | (rx_buf[1] << 8) | rx_buf[2]);
}

HAL_StatusTypeDef MS5611_Read(MS5611_t *dev)
{
  uint32_t d1 = MS5611_GetADCValue(MS5611_CMD_CONV_D1_4096); // Pressure
  uint32_t d2 = MS5611_GetADCValue(MS5611_CMD_CONV_D2_4096); // Temperature

  if (d1 == 0 || d2 == 0) return HAL_ERROR;

  // Compensation algorithm from MS5611 datasheet
  int32_t dt = d2 - ((int32_t)dev->c[5] << 8);
  int32_t temp = 2000 + (((int64_t)dt * dev->c[6]) >> 23);

  int64_t off = ((int64_t)dev->c[2] << 16) + (((int64_t)dev->c[4] * dt) >> 7);
  int64_t sens = ((int64_t)dev->c[1] << 15) + (((int64_t)dev->c[3] * dt) >> 8);

  // Second-order temperature compensation
  if (temp < 2000)
  {
    int64_t t2 = ((int64_t)dt * dt) >> 31;
    int64_t off2 = 5 * ((int64_t)(temp - 2000) * (temp - 2000)) >> 1;
    int64_t sens2 = 5 * ((int64_t)(temp - 2000) * (temp - 2000)) >> 2;
    
    if (temp < -1500)
    {
      off2 = off2 + 7 * ((int64_t)(temp + 1500) * (temp + 1500));
      sens2 = sens2 + ((11 * ((int64_t)(temp + 1500) * (temp + 1500))) >> 1);
    }
    
    temp -= t2;
    off -= off2;
    sens -= sens2;
  }

  int32_t press = (int32_t)((((d1 * sens) >> 21) - off) >> 15);

  dev->temp = temp;
  dev->press = press;

  // Calculate altitude in meters based on the standard atmosphere formula:
  // H = 44330 * (1 - (P/P0)^(1/5.255))
  // P0 = 101325 Pa (Standard sea level pressure)
  dev->altitude = 44330.0f * (1.0f - powf((float)press / 101325.0f, 0.1902949f));

  return HAL_OK;
}
