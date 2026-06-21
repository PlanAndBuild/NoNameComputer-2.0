#include "lora_llcc68.h"
#include "spi_common.h"

// SX126x/LLCC68 Commands
#define LORA_CMD_SET_STANDBY         0x80
#define LORA_CMD_SET_RX              0x82
#define LORA_CMD_SET_TX              0x83
#define LORA_CMD_SET_RF_FREQUENCY    0x86
#define LORA_CMD_SET_TX_PARAMS       0x8E
#define LORA_CMD_SET_BUFFER_BASE_ADR 0x8F
#define LORA_CMD_SET_MODULATION_PARS 0x8B
#define LORA_CMD_SET_PACKET_PARS     0x8C
#define LORA_CMD_SET_DIO_IRQ_PARAMS  0x08
#define LORA_CMD_CLEAR_IRQ_STATUS    0x02
#define LORA_CMD_GET_IRQ_STATUS      0x12
#define LORA_CMD_GET_RX_BUFFER_STAT  0x13
#define LORA_CMD_GET_PACKET_STATUS   0x14
#define LORA_CMD_WRITE_BUFFER        0x0E
#define LORA_CMD_READ_BUFFER         0x1E
#define LORA_CMD_SET_PA_CONFIG       0x95
#define LORA_CMD_SET_REGULATOR_MODE  0x96
#define LORA_CMD_SET_DIO3_AS_TCXO_CTRL 0x97
#define LORA_CMD_SET_PACKET_TYPE     0x8A

static void LoRa_WaitBusy(LoRa_t *lora)
{
  uint32_t timeout = 50000; // Guard timeout
  while (HAL_GPIO_ReadPin(lora->busy_port, lora->busy_pin) == GPIO_PIN_SET && timeout > 0)
  {
    timeout--;
  }
}

static HAL_StatusTypeDef LoRa_SendCommand(LoRa_t *lora, uint8_t cmd, uint8_t *args, uint16_t arg_len)
{
  HAL_StatusTypeDef status;
  
  LoRa_WaitBusy(lora);
  SPI_CS_Select(lora->cs_port, lora->cs_pin);
  
  status = SPI_Transmit(&cmd, 1, 50);
  if (status == HAL_OK && arg_len > 0)
  {
    status = SPI_Transmit(args, arg_len, 100);
  }
  
  SPI_CS_Deselect(lora->cs_port, lora->cs_pin);
  LoRa_WaitBusy(lora);
  
  return status;
}

static HAL_StatusTypeDef LoRa_ReadData(LoRa_t *lora, uint8_t cmd, uint8_t *buffer, uint16_t len)
{
  HAL_StatusTypeDef status;
  uint8_t dummy = 0;
  
  LoRa_WaitBusy(lora);
  SPI_CS_Select(lora->cs_port, lora->cs_pin);
  
  status = SPI_Transmit(&cmd, 1, 50);
  if (status == HAL_OK)
  {
    // LLCC68 requires 1 dummy byte before reading out data
    status = SPI_Transmit(&dummy, 1, 50);
    if (status == HAL_OK)
    {
      status = SPI_Receive(buffer, len, 100);
    }
  }
  
  SPI_CS_Deselect(lora->cs_port, lora->cs_pin);
  LoRa_WaitBusy(lora);
  
  return status;
}

HAL_StatusTypeDef LoRa_Init(LoRa_t *lora, LoRa_Module_t module)
{
  lora->module = module;
  
  if (module == LORA_1)
  {
    lora->cs_port = CS_LoRa1_GPIO_Port;
    lora->cs_pin = CS_LoRa1_Pin;
    lora->busy_port = BUSY_1_GPIO_Port;
    lora->busy_pin = BUSY_1_Pin;
    lora->dio1_port = DIO1_1_GPIO_Port;
    lora->dio1_pin = DIO1_1_Pin;
  }
  else
  {
    lora->cs_port = CS_LoRa2_GPIO_Port;
    lora->cs_pin = CS_LoRa2_Pin;
    lora->busy_port = BUSY_2_GPIO_Port;
    lora->busy_pin = BUSY_2_Pin;
    lora->dio1_port = DIO1_2_GPIO_Port;
    lora->dio1_pin = DIO1_2_Pin;
  }

  // Ensure CS is high at start
  SPI_CS_Deselect(lora->cs_port, lora->cs_pin);
  HAL_Delay(10);

  // Set standby mode
  uint8_t stdby_arg = 0x01; // Standby RC mode
  if (LoRa_SendCommand(lora, LORA_CMD_SET_STANDBY, &stdby_arg, 1) != HAL_OK)
  {
    return HAL_ERROR;
  }

  // Set regulator mode: LDO & DCDC (efficiency)
  uint8_t reg_arg = 0x01; // DCDC enabled
  LoRa_SendCommand(lora, LORA_CMD_SET_REGULATOR_MODE, &reg_arg, 1);

  // Set packet type to LoRa
  uint8_t pkt_type = 0x01; // LoRa mode
  LoRa_SendCommand(lora, LORA_CMD_SET_PACKET_TYPE, &pkt_type, 1);

  return HAL_OK;
}

HAL_StatusTypeDef LoRa_Configure(LoRa_t *lora, uint32_t frequency_hz, int8_t power_dbm, uint8_t sf, uint8_t bw)
{
  HAL_StatusTypeDef status;

  // 1. Calculate RF Frequency
  // F_RF = F_XTAL * F_RF_REG / 2^25. F_XTAL = 32MHz (32000000Hz).
  // F_RF_REG = F_RF * 2^25 / 32000000 = F_RF * 1.048576
  uint32_t freq_reg = (uint32_t)(((uint64_t)frequency_hz << 25) / 32000000ULL);
  uint8_t freq_args[4];
  freq_args[0] = (freq_reg >> 24) & 0xFF;
  freq_args[1] = (freq_reg >> 16) & 0xFF;
  freq_args[2] = (freq_reg >> 8) & 0xFF;
  freq_args[3] = freq_reg & 0xFF;
  status = LoRa_SendCommand(lora, LORA_CMD_SET_RF_FREQUENCY, freq_args, 4);
  if (status != HAL_OK) return status;

  // 2. Set PA Configuration (Output power matching for LLCC68)
  // LLCC68 HP PA configuration:
  // dutyCycle=0x04, hpMax=0x07 (power up to +22dBm), deviceSel=0x00, paLUT=0x01
  uint8_t pa_config[4] = {0x04, 0x07, 0x00, 0x01};
  LoRa_SendCommand(lora, LORA_CMD_SET_PA_CONFIG, pa_config, 4);

  // 3. Set TX parameters (Power and Ramp Time)
  // power_dbm: clamped between -9 and +22
  if (power_dbm > 22) power_dbm = 22;
  if (power_dbm < -9) power_dbm = -9;
  uint8_t tx_params[2];
  tx_params[0] = (uint8_t)power_dbm;
  tx_params[1] = 0x04; // Ramp time 200us
  LoRa_SendCommand(lora, LORA_CMD_SET_TX_PARAMS, tx_params, 2);

  // 4. Set Modulation parameters (LoRa configuration)
  // args: SF, BW, CR, LowDataRateOptimize
  // sf: 5-11
  // bw: 0=7.81kHz, 1=10.42kHz, 2=15.63kHz, 3=20.83kHz, 4=31.25kHz, 5=41.67kHz, 6=62.5kHz, 7=125kHz, 8=250kHz, 9=500kHz
  // CR: 1=4/5, 2=4/6, 3=4/7, 4=4/8
  // LowDataRateOptimize: 0=off, 1=on (required for very long symbols, e.g. SF10/SF11 at 125kHz)
  uint8_t mod_params[8] = {0};
  mod_params[0] = sf;
  mod_params[1] = bw;
  mod_params[2] = 0x01; // Coding rate 4/5
  mod_params[3] = (sf >= 10) ? 0x01 : 0x00; // Low data rate optimize
  LoRa_SendCommand(lora, LORA_CMD_SET_MODULATION_PARS, mod_params, 4);

  // 5. Set Packet parameters
  // args: PreambleLengthMSB, PreambleLengthLSB, HeaderType, PayloadLength, CRCType, InvertIQ
  uint8_t pkt_params[9] = {0};
  pkt_params[0] = 0x00; // Preamble MSB
  pkt_params[1] = 0x08; // Preamble LSB (8 symbols)
  pkt_params[2] = 0x00; // Variable length header (explicit header)
  pkt_params[3] = 0xFF; // Max payload length (255)
  pkt_params[4] = 0x01; // CRC on
  pkt_params[5] = 0x00; // Standard IQ (no invert)
  LoRa_SendCommand(lora, LORA_CMD_SET_PACKET_PARS, pkt_params, 6);

  // 6. Set Buffer base address (both TX and RX starting at offset 0)
  uint8_t base_addr[2] = {0x00, 0x00};
  LoRa_SendCommand(lora, LORA_CMD_SET_BUFFER_BASE_ADR, base_addr, 2);

  return HAL_OK;
}

HAL_StatusTypeDef LoRa_Transmit(LoRa_t *lora, uint8_t *payload, uint8_t len)
{
  HAL_StatusTypeDef status;

  // 1. Enter standby
  uint8_t stdby_arg = 0x01;
  LoRa_SendCommand(lora, LORA_CMD_SET_STANDBY, &stdby_arg, 1);

  // 2. Set packet parameters with exact payload length
  uint8_t pkt_params[6];
  pkt_params[0] = 0x00; // Preamble MSB
  pkt_params[1] = 0x08; // Preamble LSB
  pkt_params[2] = 0x00; // Variable length header
  pkt_params[3] = len;  // Payload length
  pkt_params[4] = 0x01; // CRC on
  pkt_params[5] = 0x00; // Standard IQ
  LoRa_SendCommand(lora, LORA_CMD_SET_PACKET_PARS, pkt_params, 6);

  // 3. Write payload into TX buffer (offset 0)
  uint8_t tx_args[2] = {0x00, 0x00}; // Offset 0
  
  LoRa_WaitBusy(lora);
  SPI_CS_Select(lora->cs_port, lora->cs_pin);
  uint8_t cmd = LORA_CMD_WRITE_BUFFER;
  SPI_Transmit(&cmd, 1, 50);
  SPI_Transmit(tx_args, 2, 50);
  SPI_Transmit(payload, len, 100);
  SPI_CS_Deselect(lora->cs_port, lora->cs_pin);
  LoRa_WaitBusy(lora);

  // 4. Configure interrupts (IRQ): TX Done interrupt on DIO1
  // IRQ Mask: TX_DONE (0x0001)
  // DIO1 Mask: TX_DONE (0x0001)
  // DIO2 Mask: None (0x0000)
  // DIO3 Mask: None (0x0000)
  uint8_t irq_params[8] = {0x00, 0x01, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00};
  LoRa_SendCommand(lora, LORA_CMD_SET_DIO_IRQ_PARAMS, irq_params, 8);

  // 5. Clear past interrupt flags
  uint8_t clear_irq[2] = {0xFF, 0xFF}; // Clear all interrupts
  LoRa_SendCommand(lora, LORA_CMD_CLEAR_IRQ_STATUS, clear_irq, 2);

  // 6. Start transmission (TX mode)
  // timeout = 0 means single transmission, no timeout
  uint8_t tx_timeout[3] = {0x00, 0x00, 0x00};
  status = LoRa_SendCommand(lora, LORA_CMD_SET_TX, tx_timeout, 3);
  
  return status;
}

HAL_StatusTypeDef LoRa_ReceiveStart(LoRa_t *lora)
{
  // 1. Enter standby
  uint8_t stdby_arg = 0x01;
  LoRa_SendCommand(lora, LORA_CMD_SET_STANDBY, &stdby_arg, 1);

  // 2. Configure interrupts (IRQ): RX Done (0x0002) and CRC Error (0x0040) on DIO1
  // IRQ Mask: 0x0002 | 0x0040 = 0x0042
  uint8_t irq_params[8] = {0x00, 0x42, 0x00, 0x42, 0x00, 0x00, 0x00, 0x00};
  LoRa_SendCommand(lora, LORA_CMD_SET_DIO_IRQ_PARAMS, irq_params, 8);

  // 3. Clear interrupts
  uint8_t clear_irq[2] = {0xFF, 0xFF};
  LoRa_SendCommand(lora, LORA_CMD_CLEAR_IRQ_STATUS, clear_irq, 2);

  // 4. Start receiving (continuous receive mode = 0xFFFFFF)
  uint8_t rx_timeout[3] = {0xFF, 0xFF, 0xFF};
  return LoRa_SendCommand(lora, LORA_CMD_SET_RX, rx_timeout, 3);
}

HAL_StatusTypeDef LoRa_ReceivePoll(LoRa_t *lora, uint8_t *payload, uint8_t *len, int8_t *rssi, int8_t *snr)
{
  uint8_t irq_status[2] = {0};
  
  // Read IRQ Status
  if (LoRa_ReadData(lora, LORA_CMD_GET_IRQ_STATUS, irq_status, 2) != HAL_OK)
  {
    return HAL_ERROR;
  }
  
  uint16_t irq = (uint16_t)((irq_status[0] << 8) | irq_status[1]);
  
  // Check RX_DONE
  if (irq & 0x0002)
  {
    // Clear IRQ status
    uint8_t clear_irq[2] = {0xFF, 0xFF};
    LoRa_SendCommand(lora, LORA_CMD_CLEAR_IRQ_STATUS, clear_irq, 2);
    
    // Check for CRC Error
    if (irq & 0x0040)
    {
      // CRC Error, discard packet
      return HAL_ERROR;
    }
    
    // Get RX buffer status: PayloadLength (byte 0), RxStartBufferPointer (byte 1)
    uint8_t rx_stat[2] = {0};
    LoRa_ReadData(lora, LORA_CMD_GET_RX_BUFFER_STAT, rx_stat, 2);
    *len = rx_stat[0];
    uint8_t start_addr = rx_stat[1];
    
    // Read payload from RX buffer
    uint8_t read_args[2];
    read_args[0] = start_addr;
    read_args[1] = 0x00; // Offset dummy
    
    LoRa_WaitBusy(lora);
    SPI_CS_Select(lora->cs_port, lora->cs_pin);
    uint8_t cmd = LORA_CMD_READ_BUFFER;
    SPI_Transmit(&cmd, 1, 50);
    SPI_Transmit(read_args, 2, 50);
    SPI_Receive(payload, *len, 100);
    SPI_CS_Deselect(lora->cs_port, lora->cs_pin);
    LoRa_WaitBusy(lora);

    // Read Packet Status: RssiPkt (byte 0), SnrPkt (byte 1), SignalRssiPkt (byte 2)
    uint8_t pkt_stat[3] = {0};
    LoRa_ReadData(lora, LORA_CMD_GET_PACKET_STATUS, pkt_stat, 3);
    
    // RSSI [dBm] = -RssiPkt / 2
    *rssi = -(int8_t)(pkt_stat[0] / 2);
    // SNR [dB] = SnrPkt / 4 (signed 8-bit integer)
    *snr = (int8_t)pkt_stat[1] / 4;
    
    // Restart RX continuous
    LoRa_ReceiveStart(lora);
    
    return HAL_OK;
  }
  
  return HAL_BUSY;
}
