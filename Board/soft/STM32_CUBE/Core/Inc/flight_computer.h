#ifndef __FLIGHT_COMPUTER_H
#define __FLIGHT_COMPUTER_H

#include "main.h"

// Flight States
typedef enum {
  STATE_IDLE = 0,
  STATE_ARMED,
  STATE_POWERED_BOOST,
  STATE_COASTING,
  STATE_DESCENT_DROGUE,
  STATE_DESCENT_MAIN,
  STATE_LANDED
} FlightState_t;

// Log Record to write to Flash/SD
typedef struct {
  uint32_t timestamp_ms;
  uint8_t state;
  float kalman_alt;
  float kalman_vel;
  float ms5611_alt;
  float bmp390_alt;
  float ax, ay, az; // High-G accel
  float imu_ax, imu_ay, imu_az; // Main IMU
  float imu_gx, imu_gy, imu_gz; // Main IMU
  float mx, my, mz; // Mag
  float battery_v;
  float system_current;
  float gps_lat, gps_lon, gps_alt;
} FlightLog_t;

void FlightComputer_Init(void);
void FlightComputer_Loop(void);

// LoRa Command Handlers
void FlightComputer_ProcessCommand(uint8_t *cmd, uint8_t len);

#endif /* __FLIGHT_COMPUTER_H */
