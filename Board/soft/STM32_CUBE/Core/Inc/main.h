/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.h
  * @brief          : Header for main.c file.
  *                   This file contains the common defines of the application.
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2026 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */
/* USER CODE END Header */

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __MAIN_H
#define __MAIN_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "stm32h5xx_hal.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */

/* USER CODE END Includes */

/* Exported types ------------------------------------------------------------*/
/* USER CODE BEGIN ET */

/* USER CODE END ET */

/* Exported constants --------------------------------------------------------*/
/* USER CODE BEGIN EC */

/* USER CODE END EC */

/* Exported macro ------------------------------------------------------------*/
/* USER CODE BEGIN EM */

/* USER CODE END EM */

/* Exported functions prototypes ---------------------------------------------*/
void Error_Handler(void);

/* USER CODE BEGIN EFP */

/* USER CODE END EFP */

/* Private defines -----------------------------------------------------------*/
#define CS_LoRa1_Pin GPIO_PIN_13
#define CS_LoRa1_GPIO_Port GPIOC
#define DIO1_1_Pin GPIO_PIN_14
#define DIO1_1_GPIO_Port GPIOC
#define CS_BMP390_Pin GPIO_PIN_15
#define CS_BMP390_GPIO_Port GPIOC
#define SepDet1_Pin GPIO_PIN_0
#define SepDet1_GPIO_Port GPIOC
#define SepDet2_Pin GPIO_PIN_1
#define SepDet2_GPIO_Port GPIOC
#define GPS_RST_Pin GPIO_PIN_1
#define GPS_RST_GPIO_Port GPIOA
#define EN_5V_Pin GPIO_PIN_4
#define EN_5V_GPIO_Port GPIOA
#define LED_1_Pin GPIO_PIN_5
#define LED_1_GPIO_Port GPIOA
#define PYRO_1_Pin GPIO_PIN_7
#define PYRO_1_GPIO_Port GPIOA
#define P_DET_1_Pin GPIO_PIN_4
#define P_DET_1_GPIO_Port GPIOC
#define PYRO_2_Pin GPIO_PIN_5
#define PYRO_2_GPIO_Port GPIOC
#define P_DET_2_Pin GPIO_PIN_1
#define P_DET_2_GPIO_Port GPIOB
#define PYRO_3_Pin GPIO_PIN_2
#define PYRO_3_GPIO_Port GPIOB
#define P_DET_3_Pin GPIO_PIN_12
#define P_DET_3_GPIO_Port GPIOB
#define P_DET_6_Pin GPIO_PIN_13
#define P_DET_6_GPIO_Port GPIOB
#define PYRO_6_Pin GPIO_PIN_14
#define PYRO_6_GPIO_Port GPIOB
#define P_DET_5_Pin GPIO_PIN_15
#define P_DET_5_GPIO_Port GPIOB
#define PYRO_5_Pin GPIO_PIN_11
#define PYRO_5_GPIO_Port GPIOD
#define P_DET_4_Pin GPIO_PIN_12
#define P_DET_4_GPIO_Port GPIOD
#define PYRO_4_Pin GPIO_PIN_6
#define PYRO_4_GPIO_Port GPIOC
#define CS_SD_Pin GPIO_PIN_7
#define CS_SD_GPIO_Port GPIOC
#define CS_LSM2_Pin GPIO_PIN_8
#define CS_LSM2_GPIO_Port GPIOC
#define CS_MS5611_Pin GPIO_PIN_9
#define CS_MS5611_GPIO_Port GPIOA
#define CS_MAG_Pin GPIO_PIN_10
#define CS_MAG_GPIO_Port GPIOA
#define CS_H3L_Pin GPIO_PIN_13
#define CS_H3L_GPIO_Port GPIOA
#define CS_LSM1_Pin GPIO_PIN_14
#define CS_LSM1_GPIO_Port GPIOA
#define BUSY_2_Pin GPIO_PIN_15
#define BUSY_2_GPIO_Port GPIOA
#define CS_LoRa2_Pin GPIO_PIN_7
#define CS_LoRa2_GPIO_Port GPIOB
#define DIO1_2_Pin GPIO_PIN_9
#define DIO1_2_GPIO_Port GPIOB
#define BUSY_1_Pin GPIO_PIN_0
#define BUSY_1_GPIO_Port GPIOE

/* USER CODE BEGIN Private defines */

/* USER CODE END Private defines */

#ifdef __cplusplus
}
#endif

#endif /* __MAIN_H */
