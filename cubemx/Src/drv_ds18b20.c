/*
 * Copyright (c) 2006-2021, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2022-09-08     aocekeji5       the first version
 */
#include <rtthread.h>
#include "drv_common.h"
#include "board.h"
#include "drv_ds18b20.h"
#include <rtdevice.h>
#define DS18B20_DQ  GET_PIN(B,0)


void DS18B20_Rst(void)
{
     rt_pin_mode(DS18B20_DQ, PIN_MODE_OUTPUT);// 引脚为输出模式
     rt_pin_write(DS18B20_DQ, PIN_LOW);       //拉低DQ
     rt_hw_us_delay(750);                     //拉低750us（480 ~ 960us)
     rt_pin_mode(DS18B20_DQ, PIN_MODE_INPUT); //设置为输入，即可释放总线的占用
    //进入接收模式，等待应答信号。
}

uint8_t DS18B20_Check(void)
{
    uint8_t retry=0;
    //------------------等待时间----------------------------------
    rt_hw_us_delay(15);                          //15 ~60us 等待
    while (rt_pin_read(DS18B20_DQ)&&retry<100)  //最多还等待100us
    {
           retry++;
           rt_hw_us_delay(1);
     };
    if(retry>=100)  return 1;                 //100us未响应，则判断未检测到
    else retry=0;

     //----------------------从机拉低时间开始----------------------

    while (!rt_pin_read(DS18B20_DQ)&&retry<240)
    {
            retry++;
            rt_hw_us_delay(1);
    };
    if(retry>=240)  return 1;     //最长拉低240us
    return 0;
}
void DS18B20_Write_Byte(uint8_t dat)
{
    uint8_t j;
    uint8_t testb;
    rt_pin_mode(DS18B20_DQ, PIN_MODE_OUTPUT); // DQ引脚为输出模式
   for (j=1;j<=8;j++)
    {
        testb=dat&0x01;
        dat=dat>>1;
        if (testb) //输出高
        {
            rt_pin_write(DS18B20_DQ, PIN_LOW);       //拉低DQ，主机输出低电平
            rt_hw_us_delay(2);                       //延时2us
            rt_pin_write(DS18B20_DQ, PIN_HIGH);       //主机输出高电平
            rt_hw_us_delay(60);                      //延时60us
        }
        else       //输出低
        {
            rt_pin_write(DS18B20_DQ, PIN_LOW);       //拉低DQ，主机输出低电平
            rt_hw_us_delay(60);                      //延时60us

            rt_pin_write(DS18B20_DQ, PIN_HIGH);       //主机输出高电平
            rt_hw_us_delay(2);                        //延时2us
        }
    }
    rt_pin_mode(DS18B20_DQ, PIN_MODE_INPUT);         //设置为输入，释放总线
}


uint8_t  DS18B20_Read_Bit(void)              // read one bit
{
    uint8_t data;
    rt_pin_mode(DS18B20_DQ, PIN_MODE_OUTPUT); // DQ引脚为输出模式
    rt_pin_write(DS18B20_DQ, PIN_LOW);       //拉低DQ，主机输出低电平
    rt_hw_us_delay(2);                       //延时2us
    rt_pin_mode(DS18B20_DQ, PIN_MODE_INPUT); //设置为输入，释放总线
    rt_hw_us_delay(1);//延时1us

    if(rt_pin_read(DS18B20_DQ))
        data=1;//读取总线数据
    else
        data=0;
    rt_hw_us_delay(60);  //延时60us（读一位至少60us）
    return data;
}

//从DS18B20读取一个字节
//返回值：读到的数据
uint8_t DS18B20_Read_Byte(void)    // read one byte
{
    uint8_t i,j,dat;
    dat=0;

    for (i=1;i<=8;i++)
   {
      j=DS18B20_Read_Bit();
      dat=(j<<7)|(dat>>1);
   }
    return dat;
}


int16_t DS18B20_Get_Temp(void)
{
    uint8_t temp;         //用来判断符号
    uint8_t TL,TH;
    uint16_t tem;
    rt_base_t level;

    level = rt_hw_interrupt_disable();    //API：进入临界区，退出前系统不会发生任务调度
    // ds1820 start convert
    DS18B20_Rst();
    DS18B20_Check();
    DS18B20_Write_Byte(0xcc); // skip rom
    DS18B20_Write_Byte(0x44); // convert
    rt_hw_interrupt_enable(level);    //API：退出临界区

    rt_thread_mdelay(800);  //等待转换完成,至少750ms

    level = rt_hw_interrupt_disable();    //API：进入临界区，退出前系统不会发生任务调度
    DS18B20_Rst();        //复位
    DS18B20_Check();
    DS18B20_Write_Byte(0xcc);   // skip rom
    DS18B20_Write_Byte(0xbe);   // 读取命令
    TL=DS18B20_Read_Byte();
    TH=DS18B20_Read_Byte();
    rt_hw_interrupt_enable(level);    //API：退出临界区
    if(TH>7)
    {
        TH=~TH;
        TL=~TL;
        temp=0;//温度为负
    }else temp=1;//温度为正
    tem=TH; //获得高八位
    tem<<=8;
    tem+=TL;//获得底八位
    tem=(float)tem*0.625;//转换

    if(temp)
        return tem; //返回温度值
    else
        return -tem;
}




