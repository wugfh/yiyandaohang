/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.h
  * @brief          : Header for main.c file.
  *                   This file contains the common defines of the application.
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2024 STMicroelectronics.
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
#include "stm32f4xx_hal.h"

#include "stm32f4xx_ll_i2c.h"
#include "stm32f4xx_ll_rcc.h"
#include "stm32f4xx_ll_bus.h"
#include "stm32f4xx_ll_system.h"
#include "stm32f4xx_ll_exti.h"
#include "stm32f4xx_ll_cortex.h"
#include "stm32f4xx_ll_utils.h"
#include "stm32f4xx_ll_pwr.h"
#include "stm32f4xx_ll_dma.h"
#include "stm32f4xx_ll_usart.h"
#include "stm32f4xx_ll_gpio.h"

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
#define HI3861_TX_Pin LL_GPIO_PIN_2
#define HI3861_TX_GPIO_Port GPIOA
#define HI3861_RX_Pin LL_GPIO_PIN_3
#define HI3861_RX_GPIO_Port GPIOA
#define ATGM_TX_Pin LL_GPIO_PIN_10
#define ATGM_TX_GPIO_Port GPIOB
#define ATGM_RX_Pin LL_GPIO_PIN_11
#define ATGM_RX_GPIO_Port GPIOB
#define SYNPORT_TX_Pin LL_GPIO_PIN_6
#define SYNPORT_TX_GPIO_Port GPIOC
#define SYNPORT_RX_Pin LL_GPIO_PIN_7
#define SYNPORT_RX_GPIO_Port GPIOC
#define COMPUTER_TX_Pin LL_GPIO_PIN_9
#define COMPUTER_TX_GPIO_Port GPIOA
#define COMPUTER_RX_Pin LL_GPIO_PIN_10
#define COMPUTER_RX_GPIO_Port GPIOA
#define SU03_TX_Pin LL_GPIO_PIN_10
#define SU03_TX_GPIO_Port GPIOC
#define SU03_RX_Pin LL_GPIO_PIN_11
#define SU03_RX_GPIO_Port GPIOC
#define COMPASS_SCL_Pin LL_GPIO_PIN_6
#define COMPASS_SCL_GPIO_Port GPIOB
#define COMPASS_SDA_Pin LL_GPIO_PIN_7
#define COMPASS_SDA_GPIO_Port GPIOB

/* USER CODE BEGIN Private defines */
#define SYN6288_USART   USART6
/* USER CODE END Private defines */

#ifdef __cplusplus
}
#endif

#endif /* __MAIN_H */
