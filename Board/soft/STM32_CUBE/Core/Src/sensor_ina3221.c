#include "sensor_ina3221.h"

extern I2C_HandleTypeDef hi2c2;

// INA3221 I2C Address (7-bit = 0x40. 8-bit write = 0x80, 8-bit read = 0x81)
#define INA3221_I2C_ADDR         0x80

// Registers
#define INA3221_REG_CONFIG       0x00
#define INA3221_REG_SHUNTV1      0x01
#define INA3221_REG_BUSV1        0x02
#define INA3221_REG_SHUNTV2      0x03
#define INA3221_REG_BUSV2        0x04
#define INA3221_REG_SHUNTV3      0x05
#define INA3221_REG_BUSV3        0x06
#define INA3221_REG_WHO_AM_I     0xFE // Manufacturer ID

#define INA3221_WHO_AM_I_VAL     0x5449 // "TI" in ASCII

// Default Shunt Resistors for current calculations (in Ohms)
#define SHUNT_RESISTOR_CH1       0.010f // 10 mOhm
#define SHUNT_RESISTOR_CH2       0.010f // 10 mOhm
#define SHUNT_RESISTOR_CH3       0.010f // 10 mOhm

static HAL_StatusTypeDef INA3221_WriteRegister(uint8_t reg, uint16_t val)
{
  uint8_t data[3];
  data[0] = reg;
  data[1] = (val >> 8) & 0xFF;
  data[2] = val & 0xFF;
  return HAL_I2C_Master_Transmit(&hi2c2, INA3221_I2C_ADDR, data, 3, 100);
}

static HAL_StatusTypeDef INA3221_ReadRegister(uint8_t reg, uint16_t *val)
{
  uint8_t rx_data[2] = {0};
  HAL_StatusTypeDef status = HAL_I2C_Master_Transmit(&hi2c2, INA3221_I2C_ADDR, &reg, 1, 100);
  if (status == HAL_OK)
  {
    status = HAL_I2C_Master_Receive(&hi2c2, INA3221_I2C_ADDR, rx_data, 2, 100);
    if (status == HAL_OK)
    {
      *val = (uint16_t)((rx_data[0] << 8) | rx_data[1]);
    }
  }
  return status;
}

HAL_StatusTypeDef INA3221_Init(void)
{
  uint16_t id = 0;
  
  // 1. Verify connection
  if (INA3221_ReadRegister(INA3221_REG_WHO_AM_I, &id) != HAL_OK)
  {
    return HAL_ERROR;
  }
  if (id != INA3221_WHO_AM_I_VAL)
  {
    return HAL_ERROR;
  }

  // 2. Configure chip: enable all 3 channels, average 4 samples, 
  // bus conversion time 1.1ms, shunt conversion time 1.1ms, continuous mode.
  // Value = 0x7127 (roughly matches default + 4 samples averaging)
  uint16_t config = (1 << 15) | (7 << 12) | (1 << 9) | (4 << 6) | (4 << 3) | 7; // Reset then write:
  INA_WriteRegister_Attempt:
  if (INA3221_WriteRegister(INA3221_REG_CONFIG, 0x7127) != HAL_OK)
  {
    return HAL_ERROR;
  }

  return HAL_OK;
}

HAL_StatusTypeDef INA3221_Read(INA3221_t *data)
{
  uint16_t raw_val = 0;
  float shunts[3] = { SHUNT_RESISTOR_CH1, SHUNT_RESISTOR_CH2, SHUNT_RESISTOR_CH3 };

  // Read Channel 1-3 Shunt and Bus Voltages
  for (int ch = 0; ch < 3; ch++)
  {
    // Shunt Voltage register addresses: 0x01, 0x03, 0x05
    uint8_t shunt_reg = INA3221_REG_SHUNTV1 + (ch * 2);
    if (INA3221_ReadRegister(shunt_reg, &raw_val) != HAL_OK)
    {
      return HAL_ERROR;
    }
    
    // Shunt voltage LSB is 40 uV. Value is signed.
    int16_t signed_shunt = (int16_t)raw_val;
    data->shunt_voltage[ch] = (float)signed_shunt * 0.040f; // in mV

    // Bus Voltage register addresses: 0x02, 0x04, 0x06
    uint8_t bus_reg = INA3221_REG_BUSV1 + (ch * 2);
    if (INA3221_ReadRegister(bus_reg, &raw_val) != HAL_OK)
    {
      return HAL_ERROR;
    }
    
    // Bus voltage LSB is 8 mV. Upper 3 bits are 0, lowest 3 bits are 0 (it is shifted by 3 bits).
    // The value is in bits [15:3].
    int16_t signed_bus = (int16_t)raw_val;
    data->bus_voltage[ch] = (float)(signed_bus >> 3) * 0.008f; // in Volts

    // Calculate current: I = V_shunt / R_shunt
    data->current[ch] = (data->shunt_voltage[ch] / 1000.0f) / shunts[ch]; // in Amperes
  }

  return HAL_OK;
}
