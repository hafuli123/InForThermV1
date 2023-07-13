/**
  ******************************************************************************
  * @file    i2c.c
  * @brief   This file provides code for the configuration
  *          of the I2C instances.
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

/* Includes ------------------------------------------------------------------*/
#include "i2c.h"

/* USER CODE BEGIN 0 */

/* USER CODE END 0 */

I2C_HandleTypeDef hi2c1;

/* I2C1 init function */
void MX_I2C1_Init(void)
{

  /* USER CODE BEGIN I2C1_Init 0 */

  /* USER CODE END I2C1_Init 0 */

  /* USER CODE BEGIN I2C1_Init 1 */

  /* USER CODE END I2C1_Init 1 */
  hi2c1.Instance = I2C1;
  hi2c1.Init.ClockSpeed = 100000;
  hi2c1.Init.DutyCycle = I2C_DUTYCYCLE_2;
  hi2c1.Init.OwnAddress1 = 0;
  hi2c1.Init.AddressingMode = I2C_ADDRESSINGMODE_7BIT;
  hi2c1.Init.DualAddressMode = I2C_DUALADDRESS_DISABLE;
  hi2c1.Init.OwnAddress2 = 0;
  hi2c1.Init.GeneralCallMode = I2C_GENERALCALL_DISABLE;
  hi2c1.Init.NoStretchMode = I2C_NOSTRETCH_DISABLE;
  if (HAL_I2C_Init(&hi2c1) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN I2C1_Init 2 */

  /* USER CODE END I2C1_Init 2 */

}

void HAL_I2C_MspInit(I2C_HandleTypeDef* i2cHandle)
{

  GPIO_InitTypeDef GPIO_InitStruct = {0};
  if(i2cHandle->Instance==I2C1)
  {
  /* USER CODE BEGIN I2C1_MspInit 0 */

  /* USER CODE END I2C1_MspInit 0 */

    __HAL_RCC_GPIOB_CLK_ENABLE();
    /**I2C1 GPIO Configuration
    PB6     ------> I2C1_SCL
    PB7     ------> I2C1_SDA
    */
    GPIO_InitStruct.Pin = GPIO_PIN_6|GPIO_PIN_7;
    GPIO_InitStruct.Mode = GPIO_MODE_AF_OD;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
    GPIO_InitStruct.Alternate = GPIO_AF4_I2C1;
    HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

    /* I2C1 clock enable */
    __HAL_RCC_I2C1_CLK_ENABLE();
  /* USER CODE BEGIN I2C1_MspInit 1 */

  /* USER CODE END I2C1_MspInit 1 */
  }
}

void HAL_I2C_MspDeInit(I2C_HandleTypeDef* i2cHandle)
{

  if(i2cHandle->Instance==I2C1)
  {
  /* USER CODE BEGIN I2C1_MspDeInit 0 */

  /* USER CODE END I2C1_MspDeInit 0 */
    /* Peripheral clock disable */
    __HAL_RCC_I2C1_CLK_DISABLE();

    /**I2C1 GPIO Configuration
    PB6     ------> I2C1_SCL
    PB7     ------> I2C1_SDA
    */
    HAL_GPIO_DeInit(GPIOB, GPIO_PIN_6);

    HAL_GPIO_DeInit(GPIOB, GPIO_PIN_7);

  /* USER CODE BEGIN I2C1_MspDeInit 1 */

  /* USER CODE END I2C1_MspDeInit 1 */
  }
}

/* USER CODE BEGIN 1 */
/**
  * 函数功能: I2C总线位延迟，�??�??400KHz
  * 输入参数: �??
  * �?? �?? �??: �??
  * �??    明：�??
  */
static void I2C_Delay(void)
{
    uint8_t i;

    for (i = 0; i < 30; i++);
}

/**
  * 函数功能: CPU发起I2C总线启动信号
  * 输入参数: �??
  * �?? �?? �??: �??
  * �??    明：�??
  */
void I2C_Start(void)
{
    /* 当SCL高电平时，SDA出现�??个下跳沿表示I2C总线启动信号 */
    I2C_SDA_HIGH();
    I2C_SCL_HIGH();
    I2C_Delay();
    I2C_SDA_LOW();
    I2C_Delay();
    I2C_SCL_LOW();
    I2C_Delay();
}

/**
  * 函数功能: CPU发起I2C总线停止信号
  * 输入参数: �??
  * �?? �?? �??: �??
  * �??    明：�??
  */
void I2C_Stop(void)
{
    /* 当SCL高电平时，SDA出现�??个上跳沿表示I2C总线停止信号 */
    I2C_SDA_LOW();
    I2C_SCL_HIGH();
    I2C_Delay();
    I2C_SDA_HIGH();
}

/**
  * 函数功能: CPU向I2C总线设备发�??8bit数据
  * 输入参数: Byte �?? 等待发�?�的字节
  * �?? �?? �??: �??
  * �??    明：�??
  */
void I2C_SendByte(uint8_t Byte)
{
    uint8_t i;

    /* 先发送字节的高位bit7 */
    for (i = 0; i < 8; i++)
    {
        if (Byte & 0x80)
        {
            I2C_SDA_HIGH();
        }
        else
        {
            I2C_SDA_LOW();
        }
        I2C_Delay();
        I2C_SCL_HIGH();
        I2C_Delay();
        I2C_SCL_LOW();
        if (i == 7)
        {
            I2C_SDA_HIGH(); // 释放总线
        }
        Byte <<= 1; /* 左移�??个bit */
        I2C_Delay();
    }
}


/**
  * 函数功能: CPU从I2C总线设备读取8bit数据
  * 输入参数: �??
  * �?? �?? �??: 读到的数�??
  * �??    明：�??
  */
uint8_t I2C_ReadByte(void)
{
    uint8_t i;
    uint8_t value;

    /* 读到�??1个bit为数据的bit7 */
    value = 0;
    for (i = 0; i < 8; i++)
    {
        value <<= 1;
        I2C_SCL_HIGH();
        I2C_Delay();
        if (I2C_SDA_READ())
        {
            value++;
        }
        I2C_SCL_LOW();
        I2C_Delay();
    }
    return value;
}

/**
  * 函数功能: CPU产生�??个时钟，并读取器件的ACK应答信号
  * 输入参数: �??
  * �?? �?? �??: 返回0表示正确应答�??1表示无器件响�??
  * �??    明：�??
  */
uint8_t I2C_WaitAck(void)
{
    uint8_t re;

    I2C_SDA_HIGH(); /* CPU释放SDA总线 */
    I2C_Delay();
    I2C_SCL_HIGH(); /* CPU驱动SCL = 1, 此时器件会返回ACK应答 */
    I2C_Delay();
    if (I2C_SDA_READ()) /* CPU读取SDA口线状�?? */
    {
        re = 1;
    }
    else
    {
        re = 0;
    }
    I2C_SCL_LOW();
    I2C_Delay();
    return re;
}

/**
  * 函数功能: CPU产生�??个ACK信号
  * 输入参数: �??
  * �?? �?? �??: �??
  * �??    明：�??
  */
void I2C_Ack(void)
{
    I2C_SDA_LOW();  /* CPU驱动SDA = 0 */
    I2C_Delay();
    I2C_SCL_HIGH(); /* CPU产生1个时�?? */
    I2C_Delay();
    I2C_SCL_LOW();
    I2C_Delay();
    I2C_SDA_HIGH(); /* CPU释放SDA总线 */
}

/**
  * 函数功能: CPU产生1个NACK信号
  * 输入参数: �??
  * �?? �?? �??: �??
  * �??    明：�??
  */
void I2C_NAck(void)
{
    I2C_SDA_HIGH(); /* CPU驱动SDA = 1 */
    I2C_Delay();
    I2C_SCL_HIGH(); /* CPU产生1个时�?? */
    I2C_Delay();
    I2C_SCL_LOW();
    I2C_Delay();
}

/**
  * 函数功能: 配置I2C总线的GPIO，采用模拟IO的方式实�??
  * 输入参数: �??
  * �?? �?? �??: �??
  * �??    明：�??
  */
void I2C_InitGPIO(void)
{
  GPIO_InitTypeDef GPIO_InitStruct;

  /* 打开GPIO时钟 */
    I2C_GPIO_CLK_ENABLE();

  GPIO_InitStruct.Pin = I2C_SCL_PIN|I2C_SDA_PIN;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_OD;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_MEDIUM;
  HAL_GPIO_Init(I2C_GPIO_PORT, &GPIO_InitStruct);

  /* 给一个停止信�??, 复位I2C总线上的�??有设备到待机模式 */
  I2C_Stop();
}

/**
  * 函数功能: 写入数据到i2c
  * 输入参数: �??
  * �?? �?? �??: �??
  * �??    明：�??
  */
void write_i2c(uint8_t device,uint8_t addr,uint8_t dat)
{
    I2C_Start();
    I2C_SendByte(device);
    I2C_WaitAck();
    I2C_SendByte(addr);
    I2C_WaitAck();
    I2C_SendByte(dat);
    I2C_WaitAck();
    I2C_Stop();
}

/**
  * 函数功能: 从i2c读取数据
  * 输入参数: �??
  * �?? �?? �??: �??
  * �??    明：�??
  */
uint8_t read_i2c(uint8_t device,uint8_t addr)
{
    uint8_t dat;
    I2C_Start();
    I2C_SendByte(device);
    I2C_WaitAck();
    I2C_SendByte(addr);
    I2C_WaitAck();
    I2C_Start();
    I2C_SendByte(device+0x01);
    I2C_WaitAck();
    dat=I2C_ReadByte();
    I2C_NAck();
    I2C_Stop();
    return dat;
}
/* USER CODE END 1 */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
