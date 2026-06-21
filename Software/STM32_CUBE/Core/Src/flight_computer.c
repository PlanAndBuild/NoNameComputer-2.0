#include "flight_computer.h"
#include "sensor_ms5611.h"
#include "sensor_bmp390.h"
#include "sensor_lsm6dsv.h"
#include "sensor_lsm6dso.h"
#include "sensor_h3lis200.h"
#include "sensor_mmc5983.h"
#include "sensor_ina3221.h"
#include "gps_parser.h"
#include "lora_llcc68.h"
#include "flash_w25q.h"
#include "sd_card.h"
#include "pca9685.h"
#include "spi_common.h"
#include <stdio.h>
#include <string.h>

// Shared handles and states
extern UART_HandleTypeDef huart4;

static FlightState_t current_state = STATE_IDLE;
static float ground_altitude = 0.0f;
static uint32_t flight_start_time = 0;
static uint32_t flash_write_address = 0;
static uint32_t log_count = 0;

// Device handles
static MS5611_t ms5611;
static BMP390_t bmp390;
static LSM6DSV_t imu_main;
static LSM6DSO_t imu_sub;
static H3LIS200_t high_g;
static MMC5983_t mag;
static INA3221_t power_mon;
static GPS_Data_t *gps_ptr;

static LoRa_t lora_tx;
static LoRa_t lora_rx;

// 2-State Kalman Filter state: x = [alt, vel]^T
static float x_alt = 0.0f;
static float x_vel = 0.0f;
static float P_00 = 10.0f;
static float P_01 = 0.0f;
static float P_10 = 0.0f;
static float P_11 = 10.0f;

// Filter variances (tuning parameters)
static const float Q_alt = 0.1f;
static const float Q_vel = 0.5f;
static const float R_baro = 1.2f;

static void Kalman_Predict(float accel, float dt)
{
  // State extrapolation: x_k = F * x_{k-1} + B * u
  x_alt = x_alt + x_vel * dt + 0.5f * accel * dt * dt;
  x_vel = x_vel + accel * dt;

  // Covariance extrapolation: P = F * P * F^T + Q
  float P00_new = P_00 + dt * (P_10 + P_01 + dt * P_11) + Q_alt;
  float P01_new = P_01 + dt * P_11 + Q_alt;
  float P10_new = P_10 + dt * P_11 + Q_alt;
  float P11_new = P_11 + Q_vel;

  P_00 = P00_new;
  P_01 = P01_new;
  P_10 = P10_new;
  P_11 = P11_new;
}

static void Kalman_Update(float meas_alt)
{
  // Kalman Gain: K = P * H^T * (H * P * H^T + R)^-1
  float S = P_00 + R_baro;
  float K_0 = P_00 / S;
  float K_1 = P_10 / S;

  // State update: x = x + K * (z - H * x)
  float err = meas_alt - x_alt;
  x_alt = x_alt + K_0 * err;
  x_vel = x_vel + K_1 * err;

  // Covariance update: P = (I - K * H) * P
  float P_00_new = (1.0f - K_0) * P_00;
  float P_01_new = (1.0f - K_0) * P_01;
  float P_10_new = -K_1 * P_00 + P_10;
  float P_11_new = -K_1 * P_01 + P_11;

  P_00 = P_00_new;
  P_01 = P_01_new;
  P_10 = P_10_new;
  P_11 = P_11_new;
}

// Continuity checking helper
static uint8_t CheckPyroContinuity(uint8_t channel)
{
  GPIO_PinState state = GPIO_PIN_RESET;
  switch (channel)
  {
    case 1: state = HAL_GPIO_ReadPin(P_DET_1_GPIO_Port, P_DET_1_Pin); break;
    case 2: state = HAL_GPIO_ReadPin(P_DET_2_GPIO_Port, P_DET_2_Pin); break;
    case 3: state = HAL_GPIO_ReadPin(P_DET_3_GPIO_Port, P_DET_3_Pin); break;
    case 4: state = HAL_GPIO_ReadPin(P_DET_4_GPIO_Port, P_DET_4_Pin); break;
    case 5: state = HAL_GPIO_ReadPin(P_DET_5_GPIO_Port, P_DET_5_Pin); break;
    case 6: state = HAL_GPIO_ReadPin(P_DET_6_GPIO_Port, P_DET_6_Pin); break;
    default: break;
  }
  // Low logic level usually indicates igniter is connected (closing shunt to GND), 
  // High level means open circuit. Depending on schematic, let's return 1 for connected, 0 for open.
  return (state == GPIO_PIN_RESET) ? 1 : 0;
}

static void FirePyro(uint8_t channel)
{
  switch (channel)
  {
    case 1:
      HAL_GPIO_WritePin(PYRO_1_GPIO_Port, PYRO_1_Pin, GPIO_PIN_SET);
      HAL_Delay(1000); // 1-second pulse
      HAL_GPIO_WritePin(PYRO_1_GPIO_Port, PYRO_1_Pin, GPIO_PIN_RESET);
      break;
    case 2:
      HAL_GPIO_WritePin(PYRO_2_GPIO_Port, PYRO_2_Pin, GPIO_PIN_SET);
      HAL_Delay(1000);
      HAL_GPIO_WritePin(PYRO_2_GPIO_Port, PYRO_2_Pin, GPIO_PIN_RESET);
      break;
    case 3:
      HAL_GPIO_WritePin(PYRO_3_GPIO_Port, PYRO_3_Pin, GPIO_PIN_SET);
      HAL_Delay(1000);
      HAL_GPIO_WritePin(PYRO_3_GPIO_Port, PYRO_3_Pin, GPIO_PIN_RESET);
      break;
    case 4:
      HAL_GPIO_WritePin(PYRO_4_GPIO_Port, PYRO_4_Pin, GPIO_PIN_SET);
      HAL_Delay(1000);
      HAL_GPIO_WritePin(PYRO_4_GPIO_Port, PYRO_4_Pin, GPIO_PIN_RESET);
      break;
    case 5:
      HAL_GPIO_WritePin(PYRO_5_GPIO_Port, PYRO_5_Pin, GPIO_PIN_SET);
      HAL_Delay(1000);
      HAL_GPIO_WritePin(PYRO_5_GPIO_Port, PYRO_5_Pin, GPIO_PIN_RESET);
      break;
    case 6:
      HAL_GPIO_WritePin(PYRO_6_GPIO_Port, PYRO_6_Pin, GPIO_PIN_SET);
      HAL_Delay(1000);
      HAL_GPIO_WritePin(PYRO_6_GPIO_Port, PYRO_6_Pin, GPIO_PIN_RESET);
      break;
    default:
      break;
  }
}

// Dump records from W25Q128 Flash to SD card in CSV format
static void DumpLogsToSDCard(void)
{
  uint8_t block_buf[512];
  uint32_t flash_address = 0;
  FlightLog_t log_record;
  uint32_t csv_sector_idx = 0;
  uint32_t current_block_offset = 0;

  // Let's create an SD card file pre-allocated size of 15000 sectors (approx 7.5MB)
  if (FAT32_FormatAndCreateFile(15000) != HAL_OK)
  {
    return; // SD format failed
  }

  // Pre-fill block buffer with CSV header
  memset(block_buf, 0, 512);
  int header_len = sprintf((char*)block_buf, 
    "Time(ms),State,KalmanAlt(m),KalmanVel(m/s),MS5611Alt(m),BMP390Alt(m),AccX(g),AccY(g),AccZ(g),IMUAX(g),IMUAY(g),IMUAZ(g),IMUGX(deg/s),IMUGY,IMUGZ,MagX(G),MagY,MagZ,V_Batt(V),I_Sys(A),Lat,Lon,GpsAlt\n");
  
  current_block_offset = header_len;

  for (uint32_t i = 0; i < log_count; i++)
  {
    // Read record from Flash
    if (W25Q_Read(flash_address, (uint8_t*)&log_record, sizeof(FlightLog_t)) != HAL_OK) break;
    flash_address += sizeof(FlightLog_t);

    // Format CSV row
    char row_buf[300];
    int row_len = sprintf(row_buf, 
      "%lu,%d,%.2f,%.2f,%.2f,%.2f,%.3f,%.3f,%.3f,%.3f,%.3f,%.3f,%.2f,%.2f,%.2f,%.3f,%.3f,%.3f,%.2f,%.3f,%.6f,%.6f,%.1f\n",
      log_record.timestamp_ms, log_record.state, log_record.kalman_alt, log_record.kalman_vel,
      log_record.ms5611_alt, log_record.bmp390_alt, log_record.ax, log_record.ay, log_record.az,
      log_record.imu_ax, log_record.imu_ay, log_record.imu_az, log_record.imu_gx, log_record.imu_gy, log_record.imu_gz,
      log_record.mx, log_record.my, log_record.mz, log_record.battery_v, log_record.system_current,
      log_record.gps_lat, log_record.gps_lon, log_record.gps_alt);

    if (current_block_offset + row_len >= 512)
    {
      // Write full sector buffer to SD
      FAT32_WriteFileSectors(csv_sector_idx++, block_buf, 1);
      
      // Setup next buffer
      memset(block_buf, 0, 512);
      memcpy(block_buf, row_buf, row_len);
      current_block_offset = row_len;
    }
    else
    {
      memcpy(block_buf + current_block_offset, row_buf, row_len);
      current_block_offset += row_len;
    }
  }

  // Write remaining sector buffer
  if (current_block_offset > 0)
  {
    FAT32_WriteFileSectors(csv_sector_idx++, block_buf, 1);
  }
}

void FlightComputer_Init(void)
{
  // 1. Initialize peripherals
  SPI_Common_Init();
  PCA9685_Init();
  
  // Turn on 5V supply to power servos and external logic
  HAL_GPIO_WritePin(EN_5V_GPIO_Port, EN_5V_Pin, GPIO_PIN_SET);

  // 2. Beep twice for startup signaling
  PCA9685_BuzzerSetFrequency(880.0f); // 880Hz (A5 note)
  PCA9685_BuzzerOn();
  HAL_Delay(100);
  PCA9685_BuzzerOff();
  HAL_Delay(100);
  PCA9685_BuzzerOn();
  HAL_Delay(100);
  PCA9685_BuzzerOff();

  // 3. Initialize Flash and SD card
  W25Q_Init();
  SD_Init();

  // 4. Initialize Sensors
  MS5611_Init(&ms5611);
  BMP390_Init(&bmp390);
  LSM6DSV_Init();
  LSM6DSO_Init();
  H3LIS200_Init();
  MMC5983_Init();
  INA3221_Init();
  GPS_Parser_Init();
  gps_ptr = GPS_Parser_GetData();

  // 5. Initialize LoRa transceivers
  // LoRa1 = Telemetry TX (continuous transmit beacon)
  // LoRa2 = Command RX (continuous listen)
  LoRa_Init(&lora_tx, LORA_1);
  LoRa_Configure(&lora_tx, 433000000UL, 22, 7, 7); // 433MHz, +22dBm, SF7, 125kHz

  LoRa_Init(&lora_rx, LORA_2);
  LoRa_Configure(&lora_rx, 433500000UL, 10, 7, 7); // 433.5MHz, SF7, 125kHz
  LoRa_ReceiveStart(&lora_rx);

  // Start status LED blinking or set ARGB state
  PCA9685_ARGB_High(); // LED9 active on startup
  HAL_GPIO_WritePin(LED_1_GPIO_Port, LED_1_Pin, GPIO_PIN_SET);

  current_state = STATE_IDLE;
}

void FlightComputer_ProcessCommand(uint8_t *cmd, uint8_t len)
{
  if (len < 3) return;
  
  // Arming handshake: "ARM"
  if (strncmp((char*)cmd, "ARM", 3) == 0 && current_state == STATE_IDLE)
  {
    // Calibrate baseline ground altitude
    float alt_sum = 0.0f;
    for (int i = 0; i < 20; i++)
    {
      MS5611_Read(&ms5611);
      alt_sum += ms5611.altitude;
      HAL_Delay(20);
    }
    ground_altitude = alt_sum / 20.0f;

    // Reset Kalman filter states relative to ground
    x_alt = 0.0f;
    x_vel = 0.0f;

    // Erase Flash sector area for logging
    W25Q_EraseChip();
    flash_write_address = 0;
    log_count = 0;

    current_state = STATE_ARMED;

    // Three beeps for ARMED signaling
    for (int i = 0; i < 3; i++)
    {
      PCA9685_BuzzerSetFrequency(1000.0f);
      PCA9685_BuzzerOn();
      HAL_Delay(80);
      PCA9685_BuzzerOff();
      HAL_Delay(80);
    }
  }
}

void FlightComputer_Loop(void)
{
  uint32_t last_time = HAL_GetTick();
  uint8_t uart_rx_char;

  while (1)
  {
    // Ensure strict 100Hz frequency loop (dt = 0.01 seconds)
    uint32_t current_time = HAL_GetTick();
    if (current_time - last_time < 10)
    {
      // Check for incoming UART GPS bytes while waiting
      if (HAL_UART_Receive(&huart4, &uart_rx_char, 1, 1) == HAL_OK)
      {
        GPS_Parser_FeedChar(uart_rx_char);
      }
      
      // Poll LoRa command packet
      uint8_t rx_packet[64];
      uint8_t rx_len = 0;
      int8_t rssi = 0, snr = 0;
      if (LoRa_ReceivePoll(&lora_rx, rx_packet, &rx_len, &rssi, &snr) == HAL_OK)
      {
        FlightComputer_ProcessCommand(rx_packet, rx_len);
      }
      continue;
    }
    float dt = (float)(current_time - last_time) / 1000.0f;
    last_time = current_time;

    // 1. Read all sensors
    MS5611_Read(&ms5611);
    BMP390_Read(&bmp390);
    LSM6DSV_Read(&imu_main);
    LSM6DSO_Read(&imu_sub);
    H3LIS200_Read(&high_g);
    MMC5983_Read(&mag);
    INA3221_Read(&power_mon);

    float relative_alt = ms5611.altitude - ground_altitude;

    // Use High-G Z axis as control acceleration input, fall back to Main IMU if clipping
    float vertical_accel = imu_main.az - 1.0f; // in Gs (offset gravity)
    if (fabs(high_g.az) > 15.0f) // clips main IMU
    {
      vertical_accel = high_g.az - 1.0f;
    }
    // Convert vertical acceleration from g to m/s^2
    vertical_accel *= 9.80665f;

    // 2. Run Kalman Filter prediction and update
    Kalman_Predict(vertical_accel, dt);
    Kalman_Update(relative_alt);

    // 3. Flight State Machine Logic
    switch (current_state)
    {
      case STATE_IDLE:
        // Slow status beacon toggling
        if ((current_time / 1000) % 2 == 0)
        {
          PCA9685_ARGB_High();
          HAL_GPIO_WritePin(LED_1_GPIO_Port, LED_1_Pin, GPIO_PIN_SET);
        }
        else
        {
          PCA9685_ARGB_Low();
          HAL_GPIO_WritePin(LED_1_GPIO_Port, LED_1_Pin, GPIO_PIN_RESET);
        }
        break;

      case STATE_ARMED:
        // Rapid blink signaling ARMED
        if ((current_time / 200) % 2 == 0)
        {
          PCA9685_ARGB_High();
          HAL_GPIO_WritePin(LED_1_GPIO_Port, LED_1_Pin, GPIO_PIN_SET);
        }
        else
        {
          PCA9685_ARGB_Low();
          HAL_GPIO_WritePin(LED_1_GPIO_Port, LED_1_Pin, GPIO_PIN_RESET);
        }

        // Launch detection: Vertical acceleration > 3.0g for 3 continuous cycles
        static uint8_t launch_cnt = 0;
        if (vertical_accel > 3.0f * 9.80665f)
        {
          launch_cnt++;
          if (launch_cnt >= 3)
          {
            current_state = STATE_POWERED_BOOST;
            flight_start_time = current_time;
            PCA9685_ARGB_High(); // Constant ON in boost
          }
        }
        else
        {
          launch_cnt = 0;
        }
        break;

      case STATE_POWERED_BOOST:
        // Burnout detection: Vertical acceleration drops below 0.5g after boost phase
        if (current_time - flight_start_time > 1500) // Minimum 1.5 seconds boost guard
        {
          if (vertical_accel < 0.2f * 9.80665f)
          {
            current_state = STATE_COASTING;
          }
        }
        break;

      case STATE_COASTING:
        // Apogee detection: Velocity drops below 0 m/s (Kalman velocity estimate)
        if (x_vel < 0.0f && x_alt > 10.0f) // Require at least 10m altitude
        {
          // Fire Drogue Parachute (Pyro 1)
          FirePyro(1);
          current_state = STATE_DESCENT_DROGUE;
        }
        break;

      case STATE_DESCENT_DROGUE:
        // Main parachute deployment: altitude drops below 250m above ground
        if (x_alt < 250.0f && x_vel < 0.0f)
        {
          // Fire Main Parachute (Pyro 2)
          FirePyro(2);
          current_state = STATE_DESCENT_MAIN;
        }
        break;

      case STATE_DESCENT_MAIN:
        // Landed detection: altitude variance is near zero over 5 seconds
        static uint32_t stable_start = 0;
        if (fabs(x_vel) < 0.3f && fabs(x_alt) < 10.0f)
        {
          if (stable_start == 0) stable_start = current_time;
          else if (current_time - stable_start > 5000) // 5 seconds stable
          {
            current_state = STATE_LANDED;
            
            // Stop logging, write logs to SD Card
            DumpLogsToSDCard();
          }
        }
        else
        {
          stable_start = 0;
        }
        break;

      case STATE_LANDED:
        // Toggle buzzer beacon to help locate rocket on the ground
        if ((current_time / 1000) % 3 == 0)
        {
          PCA9685_BuzzerSetFrequency(2000.0f); // Loud 2kHz tone
          PCA9685_BuzzerOn();
        }
        else
        {
          PCA9685_BuzzerOff();
        }

        // Transmit recovery GPS coordinates over LoRa
        static uint32_t last_tx = 0;
        if (current_time - last_tx > 2000) // Every 2 seconds
        {
          last_tx = current_time;
          char packet[64];
          int p_len = sprintf(packet, "GPS:%f,%f,A:%.1f", 
            gps_ptr->latitude, gps_ptr->longitude, gps_ptr->altitude);
          LoRa_Transmit(&lora_tx, (uint8_t*)packet, p_len);
        }
        break;
    }

    // 4. Logging: record flight logs to Flash (W25Q) during active flight phases
    if (current_state >= STATE_POWERED_BOOST && current_state < STATE_LANDED)
    {
      if (flash_write_address + sizeof(FlightLog_t) < W25Q_TOTAL_SECTORS * W25Q_SECTOR_SIZE)
      {
        FlightLog_t log_rec;
        log_rec.timestamp_ms = current_time;
        log_rec.state = (uint8_t)current_state;
        log_rec.kalman_alt = x_alt;
        log_rec.kalman_vel = x_vel;
        log_rec.ms5611_alt = ms5611.altitude;
        log_rec.bmp390_alt = bmp390.altitude;
        log_rec.ax = high_g.ax;
        log_rec.ay = high_g.ay;
        log_rec.az = high_g.az;
        log_rec.imu_ax = imu_main.ax;
        log_rec.imu_ay = imu_main.ay;
        log_rec.imu_az = imu_main.az;
        log_rec.imu_gx = imu_main.gx;
        log_rec.imu_gy = imu_main.gy;
        log_rec.imu_gz = imu_main.gz;
        log_rec.mx = mag.mx;
        log_rec.my = mag.my;
        log_rec.mz = mag.mz;
        log_rec.battery_v = power_mon.bus_voltage[0]; // Channel 1 system battery
        log_rec.system_current = power_mon.current[0];
        log_rec.gps_lat = gps_ptr->latitude;
        log_rec.gps_lon = gps_ptr->longitude;
        log_rec.gps_alt = gps_ptr->altitude;

        // Perform flash logging
        W25Q_WritePage(flash_write_address, (uint8_t*)&log_rec, sizeof(FlightLog_t));
        flash_write_address += sizeof(FlightLog_t);
        log_count++;
      }
    }

    // 5. TX In-flight packet (LoRa1): send basic altitude/velocity telemetry
    static uint32_t telemetry_time = 0;
    if (current_time - telemetry_time > 200) // 5Hz Telemetry Rate
    {
      telemetry_time = current_time;
      char tele_pkt[40];
      int t_len = sprintf(tele_pkt, "S:%d,A:%.1f,V:%.1f", 
        (int)current_state, x_alt, x_vel);
      LoRa_Transmit(&lora_tx, (uint8_t*)tele_pkt, t_len);
    }
  }
}
