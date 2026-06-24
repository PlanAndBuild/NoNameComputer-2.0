#ifndef __LORA_LLCC68_H
#define __LORA_LLCC68_H

#include "main.h"

// Define the two LoRa module instances
typedef enum {
  LORA_1 = 0,
  LORA_2
} LoRa_Module_t;

typedef struct {
  LoRa_Module_t module;
  GPIO_TypeDef *cs_port;
  uint16_t cs_pin;
  GPIO_TypeDef *busy_port;
  uint16_t busy_pin;
  GPIO_TypeDef *dio1_port;
  uint16_t dio1_pin;
  uint8_t packet_len;
} LoRa_t;

HAL_StatusTypeDef LoRa_Init(LoRa_t *lora, LoRa_Module_t module);
HAL_StatusTypeDef LoRa_Configure(LoRa_t *lora, uint32_t frequency_hz, int8_t power_dbm, uint8_t sf, uint8_t bw);
HAL_StatusTypeDef LoRa_Transmit(LoRa_t *lora, uint8_t *payload, uint8_t len);
HAL_StatusTypeDef LoRa_ReceiveStart(LoRa_t *lora);
HAL_StatusTypeDef LoRa_ReceivePoll(LoRa_t *lora, uint8_t *payload, uint8_t *len, int8_t *rssi, int8_t *snr);

#endif /* __LORA_LLCC68_H */
