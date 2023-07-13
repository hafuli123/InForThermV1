/**
  ******************************************************************************
  * @file    rtc.c
  * @brief   This file provides code for the configuration
  *          of the RTC instances.
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
#include "bsp_rtc.h"
#include "time.h"
/* USER CODE BEGIN 0 */
RTC_TimeTypeDef sTime = {0};
RTC_DateTypeDef sDate = {0};
/* USER CODE END 0 */

RTC_HandleTypeDef hrtc;

void RTC_CalendarConfig(uint8_t yr,uint8_t mon,uint8_t day,
                        uint8_t hr,uint8_t min,uint8_t sec)
{
    sDate.Year = yr;sDate.Month = mon;sDate.Date =day;
    sTime.Hours = hr;sTime.Minutes =min;sTime.Seconds = sec;
}

/* RTC init function */
void MX_RTC_Init(void)
{

  /* USER CODE BEGIN RTC_Init 0 */

  /* USER CODE END RTC_Init 0 */



  /* USER CODE BEGIN RTC_Init 1 */

  /* USER CODE END RTC_Init 1 */
  /** Initialize RTC Only
  */
  hrtc.Instance = RTC;
  hrtc.Init.HourFormat = RTC_HOURFORMAT_24;
  hrtc.Init.AsynchPrediv = 127;
  hrtc.Init.SynchPrediv = 255;
  hrtc.Init.OutPut = RTC_OUTPUT_DISABLE;
  hrtc.Init.OutPutPolarity = RTC_OUTPUT_POLARITY_HIGH;
  hrtc.Init.OutPutType = RTC_OUTPUT_TYPE_OPENDRAIN;
  if (HAL_RTC_Init(&hrtc) != HAL_OK)
  {
    Error_Handler();
  }

  /* USER CODE BEGIN Check_RTC_BKUP */

  /* USER CODE END Check_RTC_BKUP */

  /** Initialize RTC and set the Time and Date
  */
//  sTime.Hours = 16;
//  sTime.Minutes = 30;
//  sTime.Seconds = 51;
  sTime.TimeFormat = RTC_HOURFORMAT12_AM;
  sTime.DayLightSaving = RTC_DAYLIGHTSAVING_NONE;
  sTime.StoreOperation = RTC_STOREOPERATION_RESET;
  if (HAL_RTC_SetTime(&hrtc, &sTime, RTC_FORMAT_BIN) != HAL_OK)
  {
    Error_Handler();
  }

//  sDate.Month = RTC_MONTH_JANUARY;
//  sDate.Date = 21;
//  sDate.Year = 22;

  if (HAL_RTC_SetDate(&hrtc, &sDate, RTC_FORMAT_BIN) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN RTC_Init 2 */

  /* USER CODE END RTC_Init 2 */

}

void HAL_RTC_MspInit(RTC_HandleTypeDef* rtcHandle)
{

  RCC_PeriphCLKInitTypeDef PeriphClkInitStruct = {0};
  if(rtcHandle->Instance==RTC)
  {
  /* USER CODE BEGIN RTC_MspInit 0 */

  /* USER CODE END RTC_MspInit 0 */
  /** Initializes the peripherals clock
  */
    PeriphClkInitStruct.PeriphClockSelection = RCC_PERIPHCLK_RTC;
    PeriphClkInitStruct.RTCClockSelection = RCC_RTCCLKSOURCE_LSE;
    if (HAL_RCCEx_PeriphCLKConfig(&PeriphClkInitStruct) != HAL_OK)
    {
      Error_Handler();
    }

    /* RTC clock enable */
    __HAL_RCC_RTC_ENABLE();
  /* USER CODE BEGIN RTC_MspInit 1 */

  /* USER CODE END RTC_MspInit 1 */
  }
}

void HAL_RTC_MspDeInit(RTC_HandleTypeDef* rtcHandle)
{

  if(rtcHandle->Instance==RTC)
  {
  /* USER CODE BEGIN RTC_MspDeInit 0 */

  /* USER CODE END RTC_MspDeInit 0 */
    /* Peripheral clock disable */
    __HAL_RCC_RTC_DISABLE();
  /* USER CODE BEGIN RTC_MspDeInit 1 */

  /* USER CODE END RTC_MspDeInit 1 */
  }
}

/* USER CODE BEGIN 1 */
/*
 * 从NTP获取时间的ASC码信息，通过时间戳转化为最终时区正确的时间，送入RTC时钟并完成RTC时钟配置
 */
//void ntp_Get_RTCLocalTime(uint8_t *bcdval)
//{
//    struct tm ttime;
//    time_t ttimestamp;
//    ttime.tm_year = (2000+(0x0f&bcdval[0])*10+(0x0f&bcdval[1]))-1900;
//    ttime.tm_mon = (0x0f&bcdval[2])*10+(0x0f&bcdval[3])-1;
//    ttime.tm_mday = (0x0f&bcdval[4])*10+(0x0f&bcdval[5]);
//    ttime.tm_hour = (0x0f&bcdval[6])*10+(0x0f&bcdval[7]);
//    ttime.tm_min = (0x0f&bcdval[8])*10+(0x0f&bcdval[9]);
//    ttime.tm_sec = (0x0f&bcdval[10])*10+(0x0f&bcdval[11]);
//
//    ttimestamp = mktime(&ttime)+3600*8; //消除时差
//    ttime = *localtime(&ttimestamp);
//
//    /* 给RTC设入时间 */
//    sDate.Year = (uint8_t)(((ttime.tm_year+1900)/10%10)*10 + (ttime.tm_year+1900)/1%10);
//    sDate.Month = ttime.tm_mon+1;
//    sDate.Date = ttime.tm_mday;
//    sTime.Hours = ttime.tm_hour;
//    sTime.Minutes = ttime.tm_min;
//    sTime.Seconds = ttime.tm_sec;
//
//    MX_RTC_Init();
//    return;
//}

/* USER CODE END 1 */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
