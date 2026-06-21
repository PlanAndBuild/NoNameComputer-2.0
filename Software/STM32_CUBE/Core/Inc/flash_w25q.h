#ifndef __FLASH_W25Q_H
#define __FLASH_W25Q_H

#include "main.h"

#define W25Q_PAGE_SIZE           256
#define W25Q_SECTOR_SIZE         4096
#define W25Q_TOTAL_SECTORS       4096 // 4096 sectors * 4KB = 16MB

HAL_StatusTypeDef W25Q_Init(void);
HAL_StatusTypeDef W25Q_EraseSector(uint32_t sector_address);
HAL_StatusTypeDef W25Q_WritePage(uint32_t page_address, uint8_t *pBuffer, uint16_t size);
HAL_StatusTypeDef W25Q_Read(uint32_t address, uint8_t *pBuffer, uint32_t size);
HAL_StatusTypeDef W25Q_EraseChip(void);

#endif /* __FLASH_W25Q_H */
