#include "sd_card.h"
#include "spi_common.h"
#include <string.h>

extern SPI_HandleTypeDef hspi3;

// SD Commands
#define SD_CMD_GO_IDLE_STATE      0x00
#define SD_CMD_SEND_IF_COND       0x08
#define SD_CMD_READ_SINGLE_BLOCK  0x11
#define SD_CMD_WRITE_SINGLE_BLOCK 0x18
#define SD_CMD_APP_CMD            0x37
#define SD_CMD_APP_SEND_OP_COND   0x29

static uint8_t SD_SendCommand(uint8_t cmd, uint32_t arg)
{
  uint8_t tx_buf[6];
  uint8_t rx_byte = 0xFF;
  uint16_t timeout = 500;

  tx_buf[0] = 0x40 | cmd;
  tx_buf[1] = (arg >> 24) & 0xFF;
  tx_buf[2] = (arg >> 16) & 0xFF;
  tx_buf[3] = (arg >> 8) & 0xFF;
  tx_buf[4] = arg & 0xFF;

  // CRC is only required for CMD0 and CMD8 during init
  if (cmd == SD_CMD_GO_IDLE_STATE) tx_buf[5] = 0x95;
  else if (cmd == SD_CMD_SEND_IF_COND) tx_buf[5] = 0x87;
  else tx_buf[5] = 0x01;

  // Transmit command
  SPI_CS_Select(CS_SD_GPIO_Port, CS_SD_Pin);
  SPI_Transmit(tx_buf, 6, 100);

  // Wait for response (up to 500 attempts)
  do {
    SPI_Receive(&rx_byte, 1, 100);
    timeout--;
  } while (rx_byte == 0xFF && timeout > 0);

  SPI_CS_Deselect(CS_SD_GPIO_Port, CS_SD_Pin);
  return rx_byte;
}

HAL_StatusTypeDef SD_Init(void)
{
  uint8_t res;
  uint8_t dummy = 0xFF;

  // 1. Temporarily configure SPI3 to a lower clock speed (<400kHz) for initialization
  // Standard PLL prescaler for SPI3 in CubeMX gives 24MBits/s (BaudRatePrescaler = 2)
  // Let's set BaudRatePrescaler to 128 (approx. 375kHz) for initialization
  HAL_SPI_DeInit(&hspi3);
  hspi3.Init.BaudRatePrescaler = SPI_BAUDRATEPRESCALER_128;
  if (HAL_SPI_Init(&hspi3) != HAL_OK) return HAL_ERROR;

  SPI_CS_Deselect(CS_SD_GPIO_Port, CS_SD_Pin);
  
  // 2. Send 80+ clock cycles with CS High to wake up card
  for (int i = 0; i < 10; i++)
  {
    SPI_Transmit(&dummy, 1, 100);
  }

  // 3. Send CMD0 (Reset)
  res = SD_SendCommand(SD_CMD_GO_IDLE_STATE, 0);
  if (res != 0x01)
  {
    return HAL_ERROR; // Card failed to enter Idle state
  }

  // 4. Send CMD8 (Check voltage)
  res = SD_SendCommand(SD_CMD_SEND_IF_COND, 0x1AA); // 3.3V and pattern 0xAA
  if (res == 0x01)
  {
    // Read trailing 4 bytes of response R7
    uint8_t trailing[4];
    SPI_CS_Select(CS_SD_GPIO_Port, CS_SD_Pin);
    SPI_Receive(trailing, 4, 100);
    SPI_CS_Deselect(CS_SD_GPIO_Port, CS_SD_Pin);
    
    if (trailing[3] != 0xAA) return HAL_ERROR; // Pattern match failed
  }
  else
  {
    return HAL_ERROR; // Non-SDHC or old card
  }

  // 5. Initialize card using ACMD41
  uint32_t timeout = 1000;
  do {
    SD_SendCommand(SD_CMD_APP_CMD, 0);
    res = SD_SendCommand(SD_CMD_APP_SEND_OP_COND, 0x40000000); // Support High Capacity (HCS)
    HAL_Delay(1);
    timeout--;
  } while (res != 0x00 && timeout > 0);

  if (timeout == 0) return HAL_ERROR; // Initialization timeout

  // 6. Restore SPI3 clock speed to high speed for in-flight operations
  HAL_SPI_DeInit(&hspi3);
  hspi3.Init.BaudRatePrescaler = SPI_BAUDRATEPRESCALER_2; // 24MBits/s
  if (HAL_SPI_Init(&hspi3) != HAL_OK) return HAL_ERROR;

  return HAL_OK;
}

HAL_StatusTypeDef SD_ReadBlock(uint32_t block_addr, uint8_t *pBuffer)
{
  uint8_t res;
  uint8_t token;
  uint32_t timeout = 10000;

  // Send CMD17 (Read block)
  res = SD_SendCommand(SD_CMD_READ_SINGLE_BLOCK, block_addr);
  if (res != 0x00) return HAL_ERROR;

  SPI_CS_Select(CS_SD_GPIO_Port, CS_SD_Pin);
  
  // Wait for data start token (0xFE)
  do {
    SPI_Receive(&token, 1, 100);
    timeout--;
  } while (token != 0xFE && timeout > 0);

  if (timeout == 0)
  {
    SPI_CS_Deselect(CS_SD_GPIO_Port, CS_SD_Pin);
    return HAL_ERROR;
  }

  // Read 512 bytes payload
  SPI_Receive(pBuffer, 512, 100);

  // Read 2 bytes CRC (ignored in SPI mode)
  uint8_t crc[2];
  SPI_Receive(crc, 2, 100);

  SPI_CS_Deselect(CS_SD_GPIO_Port, CS_SD_Pin);
  return HAL_OK;
}

HAL_StatusTypeDef SD_WriteBlock(uint32_t block_addr, uint8_t *pBuffer)
{
  uint8_t res;
  uint8_t status_token;
  uint8_t busy_token;
  uint32_t timeout = 50000;

  // Send CMD24 (Write block)
  res = SD_SendCommand(SD_CMD_WRITE_SINGLE_BLOCK, block_addr);
  if (res != 0x00) return HAL_ERROR;

  SPI_CS_Select(CS_SD_GPIO_Port, CS_SD_Pin);

  // Send data start token (0xFE)
  uint8_t token = 0xFE;
  SPI_Transmit(&token, 1, 50);

  // Send 512 bytes payload
  SPI_Transmit(pBuffer, 512, 200);

  // Send 2 dummy CRC bytes
  uint8_t crc[2] = {0xFF, 0xFF};
  SPI_Transmit(crc, 2, 50);

  // Read status response
  SPI_Receive(&status_token, 1, 50);
  if ((status_token & 0x1F) != 0x05) // Data accepted
  {
    SPI_CS_Deselect(CS_SD_GPIO_Port, CS_SD_Pin);
    return HAL_ERROR;
  }

  // Wait until write is complete (card releases MISO line from low)
  do {
    SPI_Receive(&busy_token, 1, 50);
    timeout--;
  } while (busy_token == 0x00 && timeout > 0);

  SPI_CS_Deselect(CS_SD_GPIO_Port, CS_SD_Pin);

  if (timeout == 0) return HAL_ERROR;
  return HAL_OK;
}

// === Lightweight FAT32 Formatting and Allocator ===
// To avoid including the full FatFs library which requires extensive configuration,
// we set up a minimal valid FAT32 volume with a single file FLIGHT.CSV starting at 
// a fixed sector (e.g. Sector 2048).
// When the card is inserted in a PC, it sees a perfectly valid FAT32 drive 
// with a single file containing all our data!

#define FAT32_START_SECTOR       2048 // 1MB Offset for alignment
#define FAT32_SECTORS_PER_CLUS   8    // 4KB clusters
#define FAT32_RESERVED_SECTORS   32

HAL_StatusTypeDef FAT32_FormatAndCreateFile(uint32_t file_size_sectors)
{
  uint8_t sector[512] = {0};

  // 1. Write Master Boot Record (MBR) to Sector 0
  memset(sector, 0, 512);
  // Partition Entry 1: Boot indicator = 0, Type = FAT32 LBA (0x0C)
  // Starting sector = 2048 (0x00000800), size = 0x00F00000 (roughly 7.5GB)
  sector[446] = 0x00; // Bootable: no
  sector[450] = 0x0C; // System ID: FAT32 LBA
  
  // Starting sector LBA = 2048
  sector[454] = 0x00;
  sector[455] = 0x08;
  sector[456] = 0x00;
  sector[457] = 0x00;
  
  // Size in sectors (e.g., 7.5GB = 15728640 sectors)
  sector[458] = 0x00;
  sector[459] = 0x00;
  sector[460] = 0xF0;
  sector[461] = 0x00;
  
  sector[510] = 0x55; // MBR Signature
  sector[511] = 0xAA;
  if (SD_WriteBlock(0, sector) != HAL_OK) return HAL_ERROR;

  // 2. Write Partition Boot Record (PBR) to Sector 2048 (Start of Partition)
  memset(sector, 0, 512);
  // Jump code
  sector[0] = 0xEB; sector[1] = 0x58; sector[2] = 0x90;
  // OEM Name
  memcpy(&sector[3], "MSWIN4.1", 8);
  // Bytes per sector
  sector[11] = 0x00; sector[12] = 0x02; // 512 bytes
  // Sectors per cluster
  sector[13] = FAT32_SECTORS_PER_CLUS; // 8 sectors (4KB)
  // Reserved sectors
  sector[14] = FAT32_RESERVED_SECTORS; sector[15] = 0x00; // 32
  // Number of FATs
  sector[16] = 0x02; // 2 FAT tables
  // Media descriptor
  sector[21] = 0xF8; // Hard drive
  // Sectors per track
  sector[24] = 0x3F; sector[25] = 0x00;
  // Number of heads
  sector[26] = 0xFF; sector[27] = 0x00;
  // Hidden sectors (2048)
  sector[28] = 0x00; sector[29] = 0x08; sector[30] = 0x00; sector[31] = 0x00;
  // Large sector count (e.g. 15728640)
  sector[32] = 0x00; sector[33] = 0x00; sector[34] = 0xF0; sector[35] = 0x00;
  
  // FAT32 Specific fields
  // Sectors per FAT (e.g. 15360 sectors)
  uint32_t fat_size = 15360;
  sector[36] = fat_size & 0xFF;
  sector[37] = (fat_size >> 8) & 0xFF;
  sector[38] = (fat_size >> 16) & 0xFF;
  sector[39] = (fat_size >> 24) & 0xFF;
  
  // Root directory cluster (Cluster 2)
  sector[44] = 0x02; sector[45] = 0x00; sector[46] = 0x00; sector[47] = 0x00;
  // File system info sector
  sector[48] = 0x01; sector[49] = 0x00;
  // Signature
  sector[510] = 0x55; sector[511] = 0xAA;
  if (SD_WriteBlock(FAT32_START_SECTOR, sector) != HAL_OK) return HAL_ERROR;

  // 3. Clear FAT tables (initially set Cluster 2 = End of Chain, Cluster 3 = File Start)
  // FAT1 starts at FAT32_START_SECTOR + ReservedSectors = 2048 + 32 = 2080
  // Write FAT entries. Entry 0: Media descriptor, Entry 1: End of Chain marker.
  // Entry 2 (Root Dir Cluster): End of Chain.
  // Entry 3 to N (FLIGHT.CSV pre-allocated clusters).
  uint32_t fat1_sector = FAT32_START_SECTOR + FAT32_RESERVED_SECTORS;
  
  memset(sector, 0, 512);
  // FAT header entries
  sector[0] = 0xF8; sector[1] = 0xFF; sector[2] = 0xFF; sector[3] = 0x0F; // Media
  sector[4] = 0xFF; sector[5] = 0xFF; sector[6] = 0xFF; sector[7] = 0x0F; // Partition
  // Root Dir Cluster 2
  sector[8] = 0xFF; sector[9] = 0xFF; sector[10] = 0xFF; sector[11] = 0x0F; // EOF Root dir
  
  // Link clusters for pre-allocated file (FLIGHT.CSV starting at Cluster 3)
  // Write a sequential cluster chain: Cluster 3 -> 4 -> 5 ... -> EOF
  uint32_t file_clusters = (file_size_sectors + FAT32_SECTORS_PER_CLUS - 1) / FAT32_SECTORS_PER_CLUS;
  uint32_t current_cluster = 3;
  
  // Fill the first FAT sector
  for (uint16_t i = 3; i < 128; i++)
  {
    if (i - 3 < file_clusters - 1)
    {
      uint32_t next = i + 1;
      sector[i*4] = next & 0xFF;
      sector[i*4+1] = (next >> 8) & 0xFF;
      sector[i*4+2] = (next >> 16) & 0xFF;
      sector[i*4+3] = (next >> 24) & 0xFF;
    }
    else
    {
      sector[i*4] = 0xFF; sector[i*4+1] = 0xFF; sector[i*4+2] = 0xFF; sector[i*4+3] = 0x0F; // EOF
      break;
    }
  }
  // Write FAT sector to FAT1 & FAT2
  if (SD_WriteBlock(fat1_sector, sector) != HAL_OK) return HAL_ERROR;
  if (SD_WriteBlock(fat1_sector + fat_size, sector) != HAL_OK) return HAL_ERROR;

  // Clear rest of the FAT tables
  memset(sector, 0, 512);
  for (uint32_t s = 1; s < fat_size; s++)
  {
    // Write sequential blocks if needed for file size (each block handles 128 clusters = 512KB)
    if (s * 128 < file_clusters)
    {
      for (uint16_t i = 0; i < 128; i++)
      {
        uint32_t clus = s * 128 + i + 2; // offset
        if (clus - 3 < file_clusters - 1)
        {
          uint32_t next = clus + 1;
          sector[i*4] = next & 0xFF;
          sector[i*4+1] = (next >> 8) & 0xFF;
          sector[i*4+2] = (next >> 16) & 0xFF;
          sector[i*4+3] = (next >> 24) & 0xFF;
        }
        else
        {
          sector[i*4] = 0xFF; sector[i*4+1] = 0xFF; sector[i*4+2] = 0xFF; sector[i*4+3] = 0x0F; // EOF
          break;
        }
      }
    }
    else
    {
      memset(sector, 0, 512); // Rest are empty/free clusters
    }
    SD_WriteBlock(fat1_sector + s, sector);
    SD_WriteBlock(fat1_sector + fat_size + s, sector);
  }

  // 4. Create Root Directory Entry (Cluster 2)
  // Cluster 2 is located at:
  // fat1_sector + 2*fat_size = 2080 + 30720 = 32800 (Data Start)
  uint32_t root_dir_sector = fat1_sector + 2 * fat_size;
  memset(sector, 0, 512);
  
  // Set up file entry for FLIGHT.CSV
  // Directory entry structure (32 bytes):
  // 0-7: Name ("FLIGHT  ")
  // 8-10: Extension ("CSV")
  // 11: Attribute (0x20 = Archive)
  // 20-21: High cluster offset (Cluster 3 high word = 0x0000)
  // 26-27: Low cluster offset (Cluster 3 low word = 0x0003)
  // 28-31: File size in bytes = file_size_sectors * 512
  uint8_t *entry = &sector[0];
  memcpy(&entry[0], "FLIGHT  ", 8);
  memcpy(&entry[8], "CSV", 3);
  entry[11] = 0x20; // Archive attribute
  
  // Cluster 3
  entry[20] = 0x00; entry[21] = 0x00; // High word
  entry[26] = 0x03; entry[27] = 0x00; // Low word
  
  uint32_t byte_size = file_size_sectors * 512;
  entry[28] = byte_size & 0xFF;
  entry[29] = (byte_size >> 8) & 0xFF;
  entry[30] = (byte_size >> 16) & 0xFF;
  entry[31] = (byte_size >> 24) & 0xFF;

  if (SD_WriteBlock(root_dir_sector, sector) != HAL_OK) return HAL_ERROR;

  return HAL_OK;
}

HAL_StatusTypeDef FAT32_WriteFileSectors(uint32_t sector_offset, uint8_t *pBuffer, uint32_t num_sectors)
{
  // File starts at Cluster 3.
  // Cluster 2 starts at sector `root_dir_sector` = Data Start
  // Cluster 3 starts at `root_dir_sector` + FAT32_SECTORS_PER_CLUS = Data Start + 8
  uint32_t fat1_sector = FAT32_START_SECTOR + FAT32_RESERVED_SECTORS;
  uint32_t fat_size = 15360;
  uint32_t file_start_sector = fat1_sector + (2 * fat_size) + FAT32_SECTORS_PER_CLUS;

  for (uint32_t i = 0; i < num_sectors; i++)
  {
    if (SD_WriteBlock(file_start_sector + sector_offset + i, pBuffer + (i * 512)) != HAL_OK)
    {
      return HAL_ERROR;
    }
  }
  return HAL_OK;
}
