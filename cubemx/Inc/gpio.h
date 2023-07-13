/**
  ******************************************************************************
  * @file    gpio.h
  * @brief   This file contains all the function prototypes for
  *          the gpio.c file
  ******************************************************************************
  * @attention
  *
  * <h2><center>&copy; Copyright (c) 2022 STMicroelectronics.
  * All rights reserved.</center></h2>
  *
  * This software component is licensed by ST under BSD 3-Clause license,
  * the "License"; You may not use this file except in compliance with the
  * License. You may obtain a copy of the License at:
  *                        opensource.org/licenses/BSD-3-Clause
  *
  ******************************************************************************
  */
/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __GPIO_H__
#define __GPIO_H__

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "main.h"

/* USER CODE BEGIN Includes */

/* USER CODE END Includes */

/* USER CODE BEGIN Private defines */
#define KEY_ON_GPIO         GPIOB
#define KEY_ON_PIN          GPIO_PIN_3
#define KEY_ON_DOWN_LEVEL   GPIO_PIN_SET
#define KEY_ON_IRQ          EXTI3_IRQn

#define PWR_EN_GPIO         GPIOB
#define PWR_EN_PIN          GPIO_PIN_4
#define SYSTEM_PWR_EN_ON()  HAL_GPIO_WritePin(PWR_EN_GPIO, PWR_EN_PIN, GPIO_PIN_SET);     //系统上电
#define SYSTEM_PWR_EN_OFF() HAL_GPIO_WritePin(PWR_EN_GPIO, PWR_EN_PIN, GPIO_PIN_RESET);     //系统断电

#define EC600_PWK_GPIO      GPIOB
#define EC600_PWK_PIN       GPIO_PIN_10
#define EC600_PWK_HIGH()    HAL_GPIO_WritePin(EC600_PWK_GPIO, EC600_PWK_PIN, GPIO_PIN_SET);     //EC600�?�?
#define EC600_PWK_LOW()     HAL_GPIO_WritePin(EC600_PWK_GPIO, EC600_PWK_PIN, GPIO_PIN_RESET);

#define KEY_N_GPIO          GPIOB
#define KEY_1_PIN           GPIO_PIN_12
#define KEY_2_PIN           GPIO_PIN_13
#define KEY_3_PIN           GPIO_PIN_14
#define KEY_4_PIN           GPIO_PIN_15
#define KEY_1_DOWN_LEVEL    GPIO_PIN_RESET
#define KEY_2_DOWN_LEVEL    GPIO_PIN_RESET
#define KEY_3_DOWN_LEVEL    GPIO_PIN_RESET
#define KEY_4_DOWN_LEVEL    GPIO_PIN_RESET

/* USER CODE END Private defines */

void MX_GPIO_Init(void);
void MX_KEYON_GPIO_Init(void);
/* USER CODE BEGIN Prototypes */

/* USER CODE END Prototypes */

#ifdef __cplusplus
}
#endif
#endif /*__ GPIO_H__ */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
