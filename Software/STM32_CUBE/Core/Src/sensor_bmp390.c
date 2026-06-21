#include "sensor_bmp390.h"
#include "spi_common.h"
#include <math.h>

// BMP390 Registers
#define BMP390_REG_CHIP_ID      0x00
#define BMP390_REG_ERR_REG      0x02
#define BMP390_REG_STATUS       0x03
#define BMP390_REG_DATA_0       0x04 // Pressure data XLSB
#define BMP390_REG_DATA_1       0x05 // Pressure data LSB
#define BMP390_REG_DATA_2       0x06 // Pressure data MSB
#define BMP390_REG_DATA_3       0x07 // Temperature data XLSB
#define BMP390_REG_DATA_4       0x08 // Temperature data LSB
#define BMP390_REG_DATA_5       0x09 // Temperature data MSB
#define BMP390_REG_INT_STATUS   0x11
#define BMP390_REG_OSR          0x1C
#define BMP390_REG_ODR          0x1D
#define BMP390_REG_CONFIG       0x1F
#define BMP390_REG_PWR_CTRL     0x1B
#define BMP390_REG_CMD          0x7E
#define BMP390_REG_CALIB_FIRST  0x31

#define BMP390_CHIP_ID_VAL      0x60
#define BMP390_CMD_SOFTRESET    0xB6

// BMP390 SPI read requires 1 dummy byte after transmitting the register address.
static HAL_StatusTypeDef BMP390_SPI_Read(uint8_t reg, uint8_t *buffer, uint16_t len)
{
  HAL_StatusTypeDef status;
  uint8_t tx_byte = reg | 0x80;
  uint8_t dummy = 0;

  SPI_CS_Select(CS_BMP390_GPIO_Port, CS_BMP390_Pin);
  status = SPI_Transmit(&tx_byte, 1, 50);
  if (status == HAL_OK)
  {
    // Transmit dummy byte as required by BMP390 SPI interface
    status = SPI_Transmit(&dummy, 1, 50);
    if (status == HAL_OK)
    {
      status = SPI_Receive(buffer, len, 100);
    }
  }
  SPI_CS_Deselect(CS_BMP390_GPIO_Port, CS_BMP390_Pin);
  
  return status;
}

static HAL_StatusTypeDef BMP390_SPI_Write(uint8_t reg, uint8_t val)
{
  return SPI_WriteRegister(CS_BMP390_GPIO_Port, CS_BMP390_Pin, reg, val);
}

static void BMP390_ReadCalibration(BMP390_t *dev)
{
  uint8_t calib_buf[21] = {0};
  
  // Read 21 bytes of calibration data starting at register 0x31
  BMP390_SPI_Read(BMP390_REG_CALIB_FIRST, calib_buf, 21);

  dev->calib.T1 = (uint16_t)((calib_buf[1] << 8) | calib_buf[0]);
  dev->calib.T2 = (uint16_t)((calib_buf[3] << 8) | calib_buf[2]);
  dev->calib.T3 = (int8_t)calib_buf[4];
  dev->calib.P1 = (int16_t)((calib_buf[6] << 8) | calib_buf[5]);
  dev->calib.P2 = (int16_t)((calib_buf[8] << 8) | calib_buf[7]);
  dev->calib.P3 = (int8_t)calib_buf[9];
  dev->calib.P4 = (int8_t)calib_buf[10];
  dev->calib.P5 = (uint16_t)((calib_buf[12] << 8) | calib_buf[11]);
  dev->calib.P6 = (uint16_t)((calib_buf[14] << 8) | calib_buf[13]);
  dev->calib.P7 = (int8_t)calib_buf[15];
  dev->calib.P8 = (int8_t)calib_buf[16];
  dev->calib.P9 = (int16_t)((calib_buf[18] << 8) | calib_buf[17]);
  dev->calib.P10 = (int8_t)calib_buf[19];
  dev->calib.P11 = (int8_t)calib_buf[20];
}

HAL_StatusTypeDef BMP390_Init(BMP390_t *dev)
{
  uint8_t chip_id = 0;
  
  // 1. Check chip ID
  BMP390_SPI_Read(BMP390_REG_CHIP_ID, &chip_id, 1);
  if (chip_id != BMP390_CHIP_ID_VAL)
  {
    return HAL_ERROR;
  }

  // 2. Perform soft reset
  BMP390_SPI_Write(BMP390_REG_CMD, BMP390_CMD_SOFTRESET);
  HAL_Delay(10);

  // 3. Read calibration trim parameters
  BMP390_ReadCalibration(dev);

  // 4. Configure Oversampling (ultra high resolution: Press x8, Temp x1)
  BMP390_SPI_Write(BMP390_REG_OSR, 0x03); // osr_p = x8, osr_t = x1

  // 5. Configure ODR (Output Data Rate = 50 Hz)
  BMP390_SPI_Write(BMP390_REG_ODR, 0x02); // 50 Hz

  // 6. Configure IIR Filter (coefficient = 3)
  BMP390_SPI_Write(BMP390_REG_CONFIG, 0x02 << 1);

  // 7. Enable Temp & Press in Normal Mode
  BMP390_SPI_Write(BMP390_REG_PWR_CTRL, 0x33); // press_en=1, temp_en=1, mode=normal (0x33)
  HAL_Delay(20);

  return HAL_OK;
}

// Compensation formulas provided in BMP390 Datasheet section 8.5
static float BMP390_Compensate_Temp(BMP390_t *dev, uint32_t uncomp_temp)
{
  float partial_data1 = (float)(uncomp_temp - dev->calib.T1);
  float partial_data2 = (float)(partial_data1 * dev->calib.T2);
  
  // Store compensated temperature value for pressure compensation
  float comp_temp = partial_data2 + (partial_data1 * partial_data1) * dev->calib.T3;
  return comp_temp;
}

static float BMP390_Compensate_Press(BMP390_t *dev, uint32_t uncomp_press, float comp_temp)
{
  float partial_data1;
  float partial_data2;
  float partial_data3;
  float partial_data4;
  float out1;
  float out2;

  partial_data1 = dev->calib.P6 * comp_temp;
  partial_data2 = dev->calib.P7 * (comp_temp * comp_temp);
  partial_data3 = dev->calib.P8 * (comp_temp * comp_temp * comp_temp);
  partial_data4 = dev->calib.P5 + partial_data1 + partial_data2 + partial_data3;

  partial_data1 = dev->calib.P2 * comp_temp;
  partial_data2 = dev->calib.P3 * (comp_temp * comp_temp);
  partial_data3 = dev->calib.P4 * (comp_temp * comp_temp * comp_temp);
  out1 = (float)dev->calib.P1 + partial_data1 + partial_data2 + partial_data3;

  partial_data1 = dev->calib.P9 * (comp_temp * comp_temp);
  partial_data2 = dev->calib.P10 * (comp_temp * comp_temp * comp_temp);
  partial_data3 = (float)uncomp_press;
  partial_data4 = partial_data4 + (partial_data1 * partial_data3) + (partial_data2 * partial_data3 * partial_data3);

  out2 = partial_data4 + (out1 * partial_data3);
  return out2;
}

HAL_StatusTypeDef BMP390_Read(BMP390_t *dev)
{
  uint8_t raw_data[6] = {0};
  
  // Read pressure (3 bytes) and temperature (3 bytes)
  if (BMP390_SPI_Read(BMP390_REG_DATA_0, raw_data, 6) != HAL_OK)
  {
    return HAL_ERROR;
  }

  uint32_t uncomp_press = (uint32_t)((raw_data[2] << 16) | (raw_data[1] << 8) | raw_data[0]);
  uint32_t uncomp_temp = (uint32_t)((raw_data[5] << 16) | (raw_data[4] << 8) | raw_data[3]);

  if (uncomp_press == 0 || uncomp_temp == 0)
  {
    return HAL_ERROR;
  }

  float comp_temp = BMP390_Compensate_Temp(dev, uncomp_temp);
  // comp_temp here is the raw compensated term, actual temperature is comp_temp / 2^16 (roughly)
  dev->temperature = comp_temp; // actual compensated temperature
  dev->pressure = BMP390_Compensate_Press(dev, uncomp_press, comp_temp);

  // Calculate altitude in meters based on the standard atmosphere formula:
  dev->altitude = 44330.0f * (1.0f - powf(dev->pressure / 101325.0f, 0.1902949f));

  return HAL_OK;
}
