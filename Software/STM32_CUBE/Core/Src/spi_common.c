#include "spi_common.h"

extern SPI_HandleTypeDef hspi3;

void SPI_Common_Init(void)
{
  // 1. Pull all CS lines HIGH (deselect) to prevent bus contention
  HAL_GPIO_WritePin(CS_LoRa1_GPIO_Port, CS_LoRa1_Pin, GPIO_PIN_SET);
  HAL_GPIO_WritePin(CS_LoRa2_GPIO_Port, CS_LoRa2_Pin, GPIO_PIN_SET);
  HAL_GPIO_WritePin(CS_BMP390_GPIO_Port, CS_BMP390_Pin, GPIO_PIN_SET);
  HAL_GPIO_WritePin(CS_SD_GPIO_Port, CS_SD_Pin, GPIO_PIN_SET);
  HAL_GPIO_WritePin(CS_LSM2_GPIO_Port, CS_LSM2_Pin, GPIO_PIN_SET);
  HAL_GPIO_WritePin(CS_MS5611_GPIO_Port, CS_MS5611_Pin, GPIO_PIN_SET);
  HAL_GPIO_WritePin(CS_MAG_GPIO_Port, CS_MAG_Pin, GPIO_PIN_SET);
  HAL_GPIO_WritePin(CS_H3L_GPIO_Port, CS_H3L_Pin, GPIO_PIN_SET);
  HAL_GPIO_WritePin(CS_LSM1_GPIO_Port, CS_LSM1_Pin, GPIO_PIN_SET);

  // 2. Reconfigure hspi3 dynamically to 8-bit data size (CubeMX defaulted it to 4-bit)
  HAL_SPI_DeInit(&hspi3);
  hspi3.Init.DataSize = SPI_DATASIZE_8BIT;
  if (HAL_SPI_Init(&hspi3) != HAL_OK)
  {
    Error_Handler();
  }
}

HAL_StatusTypeDef SPI_Transmit(uint8_t *pData, uint16_t Size, uint32_t Timeout)
{
  return HAL_SPI_Transmit(&hspi3, pData, Size, Timeout);
}

HAL_StatusTypeDef SPI_Receive(uint8_t *pData, uint16_t Size, uint32_t Timeout)
{
  return HAL_SPI_Receive(&hspi3, pData, Size, Timeout);
}

HAL_StatusTypeDef SPI_TransmitReceive(uint8_t *pTxData, uint8_t *pRxData, uint16_t Size, uint32_t Timeout)
{
  return HAL_SPI_TransmitReceive(&hspi3, pTxData, pRxData, Size, Timeout);
}

void SPI_CS_Select(GPIO_TypeDef *GPIOx, uint16_t GPIO_Pin)
{
  HAL_GPIO_WritePin(GPIOx, GPIO_Pin, GPIO_PIN_RESET);
}

void SPI_CS_Deselect(GPIO_TypeDef *GPIOx, uint16_t GPIO_Pin)
{
  HAL_GPIO_WritePin(GPIOx, GPIO_Pin, GPIO_PIN_SET);
}

HAL_StatusTypeDef SPI_WriteRegister(GPIO_TypeDef *CS_Port, uint16_t CS_Pin, uint8_t reg, uint8_t val)
{
  HAL_StatusTypeDef status;
  uint8_t tx_buf[2];
  
  // SPI write standard: reg address has MSB set to 0 (write bit is 0)
  tx_buf[0] = reg & 0x7F;
  tx_buf[1] = val;

  SPI_CS_Select(CS_Port, CS_Pin);
  status = SPI_Transmit(tx_buf, 2, 100);
  SPI_CS_Deselect(CS_Port, CS_Pin);
  
  return status;
}

HAL_StatusTypeDef SPI_ReadRegister(GPIO_TypeDef *CS_Port, uint16_t CS_Pin, uint8_t reg, uint8_t *val)
{
  HAL_StatusTypeDef status;
  // SPI read standard: reg address has MSB set to 1 (read bit is 1)
  uint8_t tx_byte = reg | 0x80;

  SPI_CS_Select(CS_Port, CS_Pin);
  status = SPI_Transmit(&tx_byte, 1, 100);
  if (status == HAL_OK)
  {
    status = SPI_Receive(val, 1, 100);
  }
  SPI_CS_Deselect(CS_Port, CS_Pin);
  
  return status;
}

HAL_StatusTypeDef SPI_ReadBuffer(GPIO_TypeDef *CS_Port, uint16_t CS_Pin, uint8_t reg, uint8_t *buffer, uint16_t len)
{
  HAL_StatusTypeDef status;
  uint8_t tx_byte = reg | 0x80;

  SPI_CS_Select(CS_Port, CS_Pin);
  status = SPI_Transmit(&tx_byte, 1, 100);
  if (status == HAL_OK)
  {
    status = SPI_Receive(buffer, len, 100);
  }
  SPI_CS_Deselect(CS_Port, CS_Pin);
  
  return status;
}
