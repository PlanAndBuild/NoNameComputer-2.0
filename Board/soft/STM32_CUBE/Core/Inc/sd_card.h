#ifndef __SD_CARD_H
#define __SD_CARD_H

#include "main.h"

// SD Card block operations
HAL_StatusTypeDef SD_Init(void);
HAL_StatusTypeDef SD_WriteBlock(uint32_t block_addr, uint8_t *pBuffer);
HAL_StatusTypeDef SD_ReadBlock(uint32_t block_addr, uint8_t *pBuffer);

// Lightweight FAT32 direct writer.
// Creates a FAT32 filesystem containing a single file: FLIGHT.CSV
// This pre-allocates sectors so the flight computer can write directly,
// and the file remains perfectly readable on any PC!
HAL_StatusTypeDef FAT32_FormatAndCreateFile(uint32_t file_size_sectors);
HAL_StatusTypeDef FAT32_WriteFileSectors(uint32_t sector_offset, uint8_t *pBuffer, uint32_t num_sectors);

#endif /* __SD_CARD_H */
