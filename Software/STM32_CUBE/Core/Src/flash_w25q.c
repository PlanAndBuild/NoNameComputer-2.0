#include "flash_w25q.h"

extern XSPI_HandleTypeDef hxspi1;

// W25Q128 Instruction Codes
#define W25Q_CMD_WRITE_ENABLE    0x06
#define W25Q_CMD_READ_STATUS1    0x05
#define W25Q_CMD_PAGE_PROGRAM    0x02
#define W25Q_CMD_SECTOR_ERASE    0x20
#define W25Q_CMD_READ_DATA       0x03
#define W25Q_CMD_CHIP_ERASE      0xC7
#define W25Q_CMD_DEVICE_ID       0x9F

// Helper to configure a basic XSPI command
static void W25Q_BuildCommand(XSPI_RegularCmdTypeDef *sCommand, uint8_t instruction, uint32_t address, uint32_t addressMode, uint32_t dataMode, uint32_t nbData)
{
  sCommand->OperationType = HAL_XSPI_OPTYPE_COMMON_CFG;
  sCommand->InstructionMode = HAL_XSPI_INSTRUCTION_1_LINE;
  sCommand->InstructionWidth = HAL_XSPI_INSTRUCTION_8_BITS;
  sCommand->Instruction = instruction;
  
  sCommand->AddressMode = addressMode;
  sCommand->AddressWidth = HAL_XSPI_ADDRESS_24_BITS;
  sCommand->Address = address;
  
  sCommand->AlternateBytesMode = HAL_XSPI_ALT_BYTES_NONE;
  sCommand->DataMode = dataMode;
  sCommand->DataLength = nbData;
  sCommand->DummyCycles = 0;
  
  sCommand->DQSMode = HAL_XSPI_DQS_DISABLE;
  sCommand->SIOOMode = HAL_XSPI_SIOO_INST_EVERY_CMD;
}

static HAL_StatusTypeDef W25Q_WriteEnable(void)
{
  XSPI_RegularCmdTypeDef sCommand = {0};
  W25Q_BuildCommand(&sCommand, W25Q_CMD_WRITE_ENABLE, 0, HAL_XSPI_ADDRESS_NONE, HAL_XSPI_DATA_NONE, 0);
  return HAL_XSPI_Command(&hxspi1, &sCommand, HAL_XSPI_TIMEOUT_DEFAULT_VALUE);
}

static uint8_t W25Q_ReadStatusReg1(void)
{
  XSPI_RegularCmdTypeDef sCommand = {0};
  uint8_t status = 0;
  
  W25Q_BuildCommand(&sCommand, W25Q_CMD_READ_STATUS1, 0, HAL_XSPI_ADDRESS_NONE, HAL_XSPI_DATA_1_LINE, 1);
  if (HAL_XSPI_Command(&hxspi1, &sCommand, HAL_XSPI_TIMEOUT_DEFAULT_VALUE) == HAL_OK)
  {
    HAL_XSPI_Receive(&hxspi1, &status, HAL_XSPI_TIMEOUT_DEFAULT_VALUE);
  }
  return status;
}

static void W25Q_WaitBusy(void)
{
  while (W25Q_ReadStatusReg1() & 0x01)
  {
    HAL_Delay(1);
  }
}

HAL_StatusTypeDef W25Q_Init(void)
{
  XSPI_RegularCmdTypeDef sCommand = {0};
  uint8_t id_buf[3] = {0};
  
  // 1. Verify device connection by reading JEDEC ID (0x9F)
  W25Q_BuildCommand(&sCommand, W25Q_CMD_DEVICE_ID, 0, HAL_XSPI_ADDRESS_NONE, HAL_XSPI_DATA_1_LINE, 3);
  if (HAL_XSPI_Command(&hxspi1, &sCommand, HAL_XSPI_TIMEOUT_DEFAULT_VALUE) != HAL_OK)
  {
    return HAL_ERROR;
  }
  if (HAL_XSPI_Receive(&hxspi1, id_buf, HAL_XSPI_TIMEOUT_DEFAULT_VALUE) != HAL_OK)
  {
    return HAL_ERROR;
  }

  // Manufacturer ID (0xEF) and Device ID (0x4018 for W25Q128JV or 0x4019 for W25Q256JV)
  if (id_buf[0] != 0xEF)
  {
    return HAL_ERROR;
  }

  W25Q_WaitBusy();
  return HAL_OK;
}

HAL_StatusTypeDef W25Q_EraseSector(uint32_t sector_address)
{
  XSPI_RegularCmdTypeDef sCommand = {0};
  
  if (W25Q_WriteEnable() != HAL_OK) return HAL_ERROR;

  W25Q_BuildCommand(&sCommand, W25Q_CMD_SECTOR_ERASE, sector_address, HAL_XSPI_ADDRESS_1_LINE, HAL_XSPI_DATA_NONE, 0);
  if (HAL_XSPI_Command(&hxspi1, &sCommand, HAL_XSPI_TIMEOUT_DEFAULT_VALUE) != HAL_OK)
  {
    return HAL_ERROR;
  }
  
  W25Q_WaitBusy();
  return HAL_OK;
}

HAL_StatusTypeDef W25Q_WritePage(uint32_t page_address, uint8_t *pBuffer, uint16_t size)
{
  XSPI_RegularCmdTypeDef sCommand = {0};
  
  if (size > W25Q_PAGE_SIZE) size = W25Q_PAGE_SIZE;

  if (W25Q_WriteEnable() != HAL_OK) return HAL_ERROR;

  W25Q_BuildCommand(&sCommand, W25Q_CMD_PAGE_PROGRAM, page_address, HAL_XSPI_ADDRESS_1_LINE, HAL_XSPI_DATA_1_LINE, size);
  if (HAL_XSPI_Command(&hxspi1, &sCommand, HAL_XSPI_TIMEOUT_DEFAULT_VALUE) != HAL_OK)
  {
    return HAL_ERROR;
  }
  
  if (HAL_XSPI_Transmit(&hxspi1, pBuffer, HAL_XSPI_TIMEOUT_DEFAULT_VALUE) != HAL_OK)
  {
    return HAL_ERROR;
  }
  
  W25Q_WaitBusy();
  return HAL_OK;
}

HAL_StatusTypeDef W25Q_Read(uint32_t address, uint8_t *pBuffer, uint32_t size)
{
  XSPI_RegularCmdTypeDef sCommand = {0};
  
  W25Q_BuildCommand(&sCommand, W25Q_CMD_READ_DATA, address, HAL_XSPI_ADDRESS_1_LINE, HAL_XSPI_DATA_1_LINE, size);
  if (HAL_XSPI_Command(&hxspi1, &sCommand, HAL_XSPI_TIMEOUT_DEFAULT_VALUE) != HAL_OK)
  {
    return HAL_ERROR;
  }
  
  return HAL_XSPI_Receive(&hxspi1, pBuffer, HAL_XSPI_TIMEOUT_DEFAULT_VALUE);
}

HAL_StatusTypeDef W25Q_EraseChip(void)
{
  XSPI_RegularCmdTypeDef sCommand = {0};
  
  if (W25Q_WriteEnable() != HAL_OK) return HAL_ERROR;

  W25Q_BuildCommand(&sCommand, W25Q_CMD_CHIP_ERASE, 0, HAL_XSPI_ADDRESS_NONE, HAL_XSPI_DATA_NONE, 0);
  if (HAL_XSPI_Command(&hxspi1, &sCommand, HAL_XSPI_TIMEOUT_DEFAULT_VALUE) != HAL_OK)
  {
    return HAL_ERROR;
  }
  
  W25Q_WaitBusy();
  return HAL_OK;
}
