#ifndef __SPI_COMMON_H
#define __SPI_COMMON_H

#include "main.h"

// Reconfigures hspi3 to 8-bit data size dynamically.
void SPI_Common_Init(void);

// Safe wrappers to transact bytes over SPI3
HAL_StatusTypeDef SPI_Transmit(uint8_t *pData, uint16_t Size, uint32_t Timeout);
HAL_StatusTypeDef SPI_Receive(uint8_t *pData, uint16_t Size, uint32_t Timeout);
HAL_StatusTypeDef SPI_TransmitReceive(uint8_t *pTxData, uint8_t *pRxData, uint16_t Size, uint32_t Timeout);

// CS helper operations
void SPI_CS_Select(GPIO_TypeDef *GPIOx, uint16_t GPIO_Pin);
void SPI_CS_Deselect(GPIO_TypeDef *GPIOx, uint16_t GPIO_Pin);

// Read/write helpers for registers
HAL_StatusTypeDef SPI_WriteRegister(GPIO_TypeDef *CS_Port, uint16_t CS_Pin, uint8_t reg, uint8_t val);
HAL_StatusTypeDef SPI_ReadRegister(GPIO_TypeDef *CS_Port, uint16_t CS_Pin, uint8_t reg, uint8_t *val);
HAL_StatusTypeDef SPI_ReadBuffer(GPIO_TypeDef *CS_Port, uint16_t CS_Pin, uint8_t reg, uint8_t *buffer, uint16_t len);

#endif /* __SPI_COMMON_H */
