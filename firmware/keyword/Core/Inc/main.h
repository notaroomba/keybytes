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
#include "stm32wbxx_hal.h"

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
int Keybit_Dock_Probe(uint32_t *cfg, uint32_t *status);
int Keybit_Dock_Reads(int n);
int Keybit_Dock_Score(int n, uint32_t *bw, uint32_t *br, int *bad, int *hr);
int Keybit_Dock_Regs(uint32_t *v, uint32_t delay_ms, int halt, int x, int *hr);
extern uint32_t keybit_dock_pause_until;
extern uint32_t keyword_reset_csr;   /* RCC->CSR at boot, before the reset flags were cleared */
void Keybit_Dock_Id(char *out, size_t len, int reset);
int Keybit_Dock_Read(uint32_t addr, uint32_t *out, int n);

/* USER CODE END EFP */

/* Private defines -----------------------------------------------------------*/
#define STATUS_B_Pin GPIO_PIN_12
#define STATUS_B_GPIO_Port GPIOC
#define STATUS_G_Pin GPIO_PIN_0
#define STATUS_G_GPIO_Port GPIOD
#define STATUS_R_Pin GPIO_PIN_1
#define STATUS_R_GPIO_Port GPIOD
#define BB_SWIO_Pin GPIO_PIN_5
#define BB_SWIO_GPIO_Port GPIOB

/* USER CODE BEGIN Private defines */

/* USER CODE END Private defines */

#ifdef __cplusplus
}
#endif

#endif /* __MAIN_H */
