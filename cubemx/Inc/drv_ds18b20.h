/*
 * Copyright (c) 2006-2021, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2022-09-08     aocekeji5       the first version
 */
#ifndef CUBEMX_INC_DRV_DS18B20_H_
#define CUBEMX_INC_DRV_DS18B20_H_

#define DS18B20_Dout_GPIO_CLK_ENABLE()              __HAL_RCC_GPIOB_CLK_ENABLE()
#define DS18B20_Dout_PORT                           GPIOB
#define DS18B20_Dout_PIN                            GPIO_PIN_0

#define DS18B20_DATA_0                              HAL_GPIO_WritePin(DS18B20_Dout_PORT,DS18B20_Dout_PIN,GPIO_PIN_RESET)
#define DS18B20_DATA_1                              HAL_GPIO_WritePin(DS18B20_Dout_PORT,DS18B20_Dout_PIN,GPIO_PIN_SET)
#define DS18B20_DATA_READ()                         HAL_GPIO_ReadPin(DS18B20_Dout_PORT,DS18B20_Dout_PIN)

int16_t DS18B20_Get_Temp(void);

#endif /* CUBEMX_INC_DRV_DS18B20_H_ */
