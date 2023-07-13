/*
 * Copyright (c) 2006-2022, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2022-09-02     RT-Thread    first version
 */

#include <rtthread.h>

#define DBG_TAG "main"
#define DBG_LVL DBG_LOG
#include <rtdbg.h>
#include "serial.h"
#include "math.h"
#include "stdlib.h"
#include "time.h"
#include <string.h>
#include "rtc.h"
#include "main.h"
//#include "adc.h"
#include "bsp_i2c.h"
#include "bsp_i2c_OLED.h"

#include "bsp_spi.h"
#include "usart.h"
#include "gpio.h"
#include "drv_ds18b20.h"
#include "bsp_rtc.h"
#include "other_func.h"
#include "drv_w25q128.h"
//#include "bsp_dma.h"
#include "bsp_rtc.h"

#include "dfs_posix.h"

#include "bsp_fatfs_usb.h"
//#include "drv_spi.h"    //USB和FATFS挂载都在这里
#include "app_spi.h"
/**********************************************************************/
/* 使用方式  */
#define USING_BSP_RTC      //使用HAL库的RTC计数

/**********************************************************************/

/**********************************************************************/
/* 工作状态 */
struct WORK_STRUCT{
    uint8_t work_stat;
#define WORK_STAT_MENU          0
#define WORK_STAT_GET_NTP_TIME  1
#define WORK_STAT_OFFLINE_WORK  2
#define WORK_STAT_ONLINE_WORK   3

#define WORK_STAT_USB           10
}work_struct;
/**********************************************************************/

/**********************************************************************/
struct MENU_STRUCT{
    struct OLED_P{
        uint8_t screen;
#define SCREEN_MENU_START       0
#define SCREEN_FIRSTMENU_USB    1
#define SCREEN_FIRSTMENU_THERM  2

#define SCREEN_MENU_CYBERWORK   3
#define SCREEN_MENU_OFFLINEWORK 4
#define SCREEN_MENU_SERIALNUM   5
#define SCREEN_MENU_IPSET       6
#define SCREEN_MENU_NTPSET      7
#define SCREEN_MENU_POWEROFF    8

#define SCREEN_NTPMENU_ONLINE   10
#define SCREEN_NTPMENU_HAND     11
#define SCREEN_NTPMENU_FAILED   12

#define SCREEN_SERIALNUMMENU_HORI   13
#define SCREEN_SERIALNUMMENU_VERT   14

#define SCREEN_IPSETMENU_HORI   15
#define SCREEN_IPSETMENU_VERT   16

#define SCREEN_USB_A            50
#define SCREEN_USB_POWEROFF     51
    }oled_p;

    struct SETTING_P{
        uint8_t serialnum[5];
        uint8_t serialnum_bit;

        uint8_t address_bit;
        uint8_t ip_add[15];
        uint8_t ip_add_bit;
        uint8_t ip_port[4];
        uint8_t ntp_add[15];
    }setting_p;

    uint8_t menu_stat;
#define MENU_STAT_INIT_PAGE         0
#define MENU_STAT_FIRST_PAGE        1
#define MENU_STAT_NTP_PAGE          2
#define MENU_STAT_NTP_SETTIME_PAGE  3
#define MENU_STAT_SERIALNUM_PAGE    4
#define MENU_STAT_USB_PAGE          5
#define MENU_STAT_IP_SET_PAGE       6

#define MENU_STAT_END           0xff

}menu_struct;
/**********************************************************************/

/**********************************************************************/
/* 温度计 */
struct TEMP_STRUCT{
    float f_temp;
    uint8_t temp_sign;
#define TEMP_SIGN_POSITIVE  0
#define TEMP_SIGN_NEGATIVE  1
    int32_t i32_temp;
    uint32_t u32_temp;
    uint8_t DS18B20ID[8];   //温度计编号
}temp_struct;
/**********************************************************************/

/**********************************************************************/
/* DEVICE */
struct DEVICES_STRUCT{
    /* uart */
    rt_device_t uart1_dev;
    rt_err_t uart1_ret;

    /* adc */
    struct ADC_DEV{
        rt_adc_device_t adc_dev;
        rt_uint32_t value,vol;
    }adc_dev;
}dev_struct;
struct serial_configure uart1_configs = RT_SERIAL_CONFIG_DEFAULT;
/* U1RX */
struct RxBuf_Struct{
#define RXBUF_MAX   100
   uint8_t rxbuf[RXBUF_MAX];
   uint8_t rbit;
}rxbuf_struct;
/**********************************************************************/
/**********************************************************************/
/* SEM */
struct SEM_STRUCT{
    struct rt_semaphore uart1rx_sem;
    struct rt_semaphore adc1_sem;
    struct rt_semaphore ec600_sendm_sem , ec600_pwk_sem;
}sem_struct;
struct rt_semaphore fatfs_sem;
/**********************************************************************/
/* THREAD */
struct TH_STRUCT{
    /* devices */
    struct DEV_TP{
        rt_thread_t uart1recv_th;
        void(*uart1recv_th_entry)(void*parameter);

        rt_thread_t adc1_th;
        void(*adc1_th_entry)(void*parameter);
    }dev_tp;

    /* ec600 send message */
    struct EC600_TP{
        rt_thread_t sendm_th , pwk_th;
        void(*sendm_th_entry)(void*parameter);
        void(*pwk_th_entry)(void*parameter);
    }ec600_tp;

    /* ds18b20 */
    struct DS18B20_TP{
        rt_thread_t get_temp_th;
        void(*get_temp_th_entry)(void*parameter);
    }ds18b20_tp;

    /* fatfs */
    struct FATFS_TP{
        rt_thread_t fatfs_th;
        void(*fatfs_th_entry)(void*parameter);
    }fatfs_tp;
}th_struct;
/**********************************************************************/
/* TIMER */
struct TIM_STRUCT{
    struct EC600_TIM{
        rt_timer_t pwk_on_tm , sendm_ot_tm , sendm_tm , ntpm_ot_tm;
#define TIMER_PWK_ON_INTERVAL   60000*3 //发送温度信息间隔
#define TIMER_SENDM_OT_INTERVAL 30000
#define TIMER_SENDM_INTERVAL    1000    //每条发送命令的间隔
#define TIMER_NTPM_INTERVAL     45000   //NTP获取时间时等待的时间
    }ec600_tim;

    struct TEMP_TIM{
        rt_timer_t temp_tm;
#define TIMER_TEMP_INTERVAL     3000
    }temp_tim;

    struct OLED_TIM{
        rt_timer_t time_reflash_tm; //屏幕显示的时间每一秒刷新一次
#define TIMER_OLED_REFLASH_INTERVAL 1000
    }oled_tim;

    struct ADC_TIM{
        rt_timer_t adc_on_tm;   //获取电池电量的时间间隔
#define TIMER_ADC_ON_INTERVAL   1000*5
    }adc_tim;
}tim_struct;
/**********************************************************************/

/**********************************************************************/
/* EC600 */
struct EC600_Struct{
    uint8_t step;
#define EC600_STEP_ATE0     0
#define EC600_STEP_ATQICSGP 1
#define EC600_STEP_QIACT    2
#define EC600_STEP_QIOPEN   3
#define EC600_STEP_QISEND   4
#define EC600_STEP_0X1A     5
#define EC600_STEP_QICLOSE  6
#define EC600_STEP_QIDEACT  7

#define EC600_STEP_NTP_ATE0     10
#define EC600_STEP_NTP_QIACT    11
#define EC600_STEP_NTP_QNTP     12
#define EC600_STEP_NTP_QIDEACT  13
#define EC600_STEP_NTP_DONE     14

    uint8_t ntptime[12]; //ntp获取的时间存放于此

#define EC600_MESSAGE_LENGTH    30
    uint8_t message[EC600_MESSAGE_LENGTH];//ec600发送的温度信息
    uint8_t failed_message[30][EC600_MESSAGE_LENGTH];


    uint32_t send_count;
    uint32_t send_error_count;    //发送信息失败总次数
    uint32_t send_error_fatfs_lsk;

    uint8_t send_message_stat;  //信息发送成功还是失败
#define SEND_MESSAGE_STAT_INIT      0
#define SEND_MESSAGE_STAT_SUCCESS   1
#define SEND_MESSAGE_STAT_FAILED    2

}ec600_struct;
/**********************************************************************/
/* 时间  */
struct TIME_STRUCT{
    struct tm ntp_tmtime , localtime;
    time_t ntp_timestamp;   //从NTP获取到的时间戳
    time_t now_timestamp;     //传送信息时需要获取的当前时间戳
#define TIME_YEAR    0
#define TIME_MONTH   1
#define TIME_DAY     2
#define TIME_HOUR    3
#define TIME_MINUTE  4
#define TIME_SECOND  5
}time_struct;

/**********************************************************************/
/* fatfs */
struct FATFS_STRUCT{
#define FATFS_PATH_LENGTH   27
    uint8_t path[FATFS_PATH_LENGTH];    //FATFS记录温度信息的txt文件的文件名
    uint8_t errpath[FATFS_PATH_LENGTH+1];
    uint32_t lsk;   //文件内容索引针
    uint32_t errlsk;

    struct SETTING_FAT{
        uint8_t set_type;
#define FATFS_SET_NONE          0
#define FATFS_SET_SERIALNUM     1
#define FATFS_SET_IP            2
#define FATFS_SET_NTP           3

#define FATFS_CLEAR_DOC         10

#define FATFS_SET_READ          0xf0
#define FATFS_SET_UNINSTALL     0xf1
#define FATFS_FAILERTXT_READ    0xf2

        /* 记录设置txt的索引 */
#define FATFS_SET_LSK_SERIALNUM 3
#define FATFS_SET_LSK_IP        12
#define FATFS_SET_LSK_IPPORT    34
    }setting_fat;
}fatfs_struct;
/**********************************************************************/

/**********************************************************************/
/* Cyber Work */
struct CYBERWORK_STRUCT{
    uint8_t end_count;  //结束标志
}cyberwork_struct;
/**********************************************************************/

/**********************************************************************/
/* 其他变量 */
//系统状态
uint8_t system_stat;
#define SYSTEM_STAT_THERM   1
#define SYSTEM_STAT_USB     2
//uart1 device 的接收char
char u1rbuff;
//HAL库的RTC
RTC_TimeTypeDef ssTime;
RTC_DateTypeDef ssDate;
//KEY_ON按键状态
uint8_t key_on_stat;
#define KEY_ON_STAT_DISABLE     0
#define KEY_ON_STAT_ENABLE      1   //允许关机
//正式联网工作时，是否已经开始了发送的状态，用于READY后，按一下1键正式开始发送信息
uint8_t online_send_stat;
#define ONLINE_SEND_READY       0
#define ONLINE_SEND_WORKING     1
//正式联网工作时，需要按三次4键才会正式进入结束状态
uint8_t online_end_count;

//第一次FATFS记录文件，要先删除eror文件夹里的failed.txt文件
uint8_t delete_failed_txt  = 0;

/**********************************************************************/

void Message_Write(void);
void delay_secs(uint8_t sec);
void Battery_Display(uint32_t vol);

/**********************************************************************/
/* uart1 rx callback */
rt_err_t uart1rx_callback(rt_device_t dev,rt_size_t size)
{
    rt_sem_release(&sem_struct.uart1rx_sem);//释放信号量 让他下面线程可以跑
    return RT_EOK;
}
void UART1RECV_th_entry(void *parameter)
{
    while(1){
        while(rt_device_read(dev_struct.uart1_dev, 0, &u1rbuff, 1)!=1){
            rt_sem_take(&sem_struct.uart1rx_sem, RT_WAITING_FOREVER);
        }

        //一段接收完成，开始分析
        rxbuf_struct.rxbuf[rxbuf_struct.rbit]=u1rbuff;
//        rt_device_write(dev_struct.uart1_dev, 0, (const void*)&u1rbuff, 1);
//        rt_kprintf("%c",u1rbuff);
        switch(ec600_struct.step){
        /* EC600 - 发送信息 */
        case EC600_STEP_ATE0:{
            if((rxbuf_struct.rbit>=1)&&(rxbuf_struct.rxbuf[rxbuf_struct.rbit]=='K')&&(rxbuf_struct.rxbuf[rxbuf_struct.rbit-1]=='O')){
                LOG_D("ate0-FINE");
                rxbuf_struct.rbit=0;
                memset(rxbuf_struct.rxbuf,0,sizeof(rxbuf_struct.rxbuf));
                ec600_struct.step++;
                rt_sem_release(&sem_struct.ec600_sendm_sem);
                break;
            }
            else{
                rxbuf_struct.rbit++;
            }
            break;
        }
        case EC600_STEP_ATQICSGP:{
            if((rxbuf_struct.rbit>=1)&&(rxbuf_struct.rxbuf[rxbuf_struct.rbit]=='K')&&(rxbuf_struct.rxbuf[rxbuf_struct.rbit-1]=='O')){
                LOG_D("atqicsgp-FINE");
                rxbuf_struct.rbit=0;
                memset(rxbuf_struct.rxbuf,0,sizeof(rxbuf_struct.rxbuf));
                ec600_struct.step++;
                rt_sem_release(&sem_struct.ec600_sendm_sem);
                break;
            }
            else{
                rxbuf_struct.rbit++;
            }
            break;
        }
        case EC600_STEP_QIACT:{
            if((rxbuf_struct.rbit>=1)&&(rxbuf_struct.rxbuf[rxbuf_struct.rbit]=='K')&&(rxbuf_struct.rxbuf[rxbuf_struct.rbit-1]=='O')){
                LOG_D("atqiact-FINE");
                rxbuf_struct.rbit=0;
                memset(rxbuf_struct.rxbuf,0,sizeof(rxbuf_struct.rxbuf));
                ec600_struct.step++;
                rt_sem_release(&sem_struct.ec600_sendm_sem);
                break;
            }
            else{
                rxbuf_struct.rbit++;
            }
            break;
        }
        case EC600_STEP_QIOPEN:{
            if((rxbuf_struct.rbit>=2)&&(rxbuf_struct.rxbuf[rxbuf_struct.rbit]=='0')&&(rxbuf_struct.rxbuf[rxbuf_struct.rbit-1]==',')
                    &&(rxbuf_struct.rxbuf[rxbuf_struct.rbit-2]=='0')){
                LOG_D("atqiopen-FINE");
                rxbuf_struct.rbit=0;
                memset(rxbuf_struct.rxbuf,0,sizeof(rxbuf_struct.rxbuf));
                ec600_struct.step++;
                rt_sem_release(&sem_struct.ec600_sendm_sem);
                break;
            }
            else{
                rxbuf_struct.rbit++;
            }
            break;
        }
        case EC600_STEP_QISEND:{
            if((rxbuf_struct.rbit>=0)&&(rxbuf_struct.rxbuf[rxbuf_struct.rbit]=='>')){
                LOG_D("atqisend-FINE");
                rxbuf_struct.rbit=0;
                memset(rxbuf_struct.rxbuf,0,sizeof(rxbuf_struct.rxbuf));
                ec600_struct.step++;
                rt_sem_release(&sem_struct.ec600_sendm_sem);
                break;
            }
            else{
                rxbuf_struct.rbit++;
            }
            break;
        }

        case EC600_STEP_0X1A:{ //成功发完
            if((rxbuf_struct.rbit>=3)&&(rxbuf_struct.rxbuf[rxbuf_struct.rbit]=='D')&&(rxbuf_struct.rxbuf[rxbuf_struct.rbit-1]=='N')
                    &&(rxbuf_struct.rxbuf[rxbuf_struct.rbit-2]=='E')&&(rxbuf_struct.rxbuf[rxbuf_struct.rbit-3]=='S')){
                LOG_D("at0x1a-FINE");
                rxbuf_struct.rbit=0;
                memset(rxbuf_struct.rxbuf,0,sizeof(rxbuf_struct.rxbuf));
                ec600_struct.step++;
                rt_sem_release(&sem_struct.ec600_sendm_sem);
                break;
            }
            else{
                rxbuf_struct.rbit++;
            }
            break;
        }


        /* EC600 - NTP获取时间 */
        case EC600_STEP_NTP_ATE0:{
            if((rxbuf_struct.rbit>=1)&&(rxbuf_struct.rxbuf[rxbuf_struct.rbit]=='K')&&(rxbuf_struct.rxbuf[rxbuf_struct.rbit-1]=='O')){
                LOG_D("ntp-ate0-FINE");
                OLED_ShowStr(35, 7,(uint8_t*)&"[.     ]", 1);
                rxbuf_struct.rbit=0;
                memset(rxbuf_struct.rxbuf,0,sizeof(rxbuf_struct.rxbuf));
                ec600_struct.step++;
                rt_sem_release(&sem_struct.ec600_sendm_sem);
                break;
            }
            else{
                rxbuf_struct.rbit++;
            }
            break;
        }
        case EC600_STEP_NTP_QIACT:{
            if((rxbuf_struct.rbit>=1)&&(rxbuf_struct.rxbuf[rxbuf_struct.rbit]=='K')&&(rxbuf_struct.rxbuf[rxbuf_struct.rbit-1]=='O')){
                LOG_D("ntp-atqiact-FINE");
                OLED_ShowStr(35, 7,(uint8_t*)&"[...   ]", 1);
                rxbuf_struct.rbit=0;
                memset(rxbuf_struct.rxbuf,0,sizeof(rxbuf_struct.rxbuf));
                ec600_struct.step++;
                rt_sem_release(&sem_struct.ec600_sendm_sem);
                break;
            }
            else{
                rxbuf_struct.rbit++;
            }
            break;
        }
        case EC600_STEP_NTP_QNTP:{
            if((rxbuf_struct.rbit>=2)&&(rxbuf_struct.rxbuf[rxbuf_struct.rbit]=='2')&&(rxbuf_struct.rxbuf[rxbuf_struct.rbit-1]=='3')
                    &&(rxbuf_struct.rxbuf[rxbuf_struct.rbit-2]=='+')){
                LOG_D("ntp-qntp-FINE");
                OLED_ShowStr(35, 7,(uint8_t*)&"[..... ]", 1);
                /* 提取时间 */
                uint8_t *p,*d;
                p=&rxbuf_struct.rxbuf[rxbuf_struct.rbit-3];
                d=&ec600_struct.ntptime[11];
                for(int i=0;i<12;i++){
                    *d=*p;
                    d--;
                    p--;
                    if((i==1)||(i==3)||(i==5)||(i==7)||(i==9)){
                        p--;
                    }
                }

                for(int i=0;i<12;i++){
                    rt_kprintf("%c",ec600_struct.ntptime[i]);
                }


                /* 获取正确时间并配置RTC时钟  */
                ntp_Get_RTCLocalTime(ec600_struct.ntptime);

                rxbuf_struct.rbit=0;
                memset(rxbuf_struct.rxbuf,0,sizeof(rxbuf_struct.rxbuf));
                ec600_struct.step++;
                rt_sem_release(&sem_struct.ec600_sendm_sem);
                break;
            }
            else{
                rxbuf_struct.rbit++;
            }
            break;
        }
        case EC600_STEP_NTP_QIDEACT:{
            if((rxbuf_struct.rbit>=1)&&(rxbuf_struct.rxbuf[rxbuf_struct.rbit]=='K')&&(rxbuf_struct.rxbuf[rxbuf_struct.rbit-1]=='O')){
                LOG_D("ntp-atqideact-FINE");
                OLED_ShowStr(35, 7,(uint8_t*)&"[......]", 1);
                rxbuf_struct.rbit=0;
                memset(rxbuf_struct.rxbuf,0,sizeof(rxbuf_struct.rxbuf));
                ec600_struct.step++;
                rt_sem_release(&sem_struct.ec600_sendm_sem);
                break;
            }
            else{
                rxbuf_struct.rbit++;
            }
            break;
        }
        }

    }
}
/**********************************************************************/

/**********************************************************************/
/* ec600 send message entry */
void EC600_SENDM_th_entry(void*parameter)
{
    while(1){
        rt_sem_take(&sem_struct.ec600_sendm_sem, RT_WAITING_FOREVER);
        switch(ec600_struct.step){
        /* 发送温度信息 */
        case EC600_STEP_ATE0:{
            /* 发送总数+1 */
            ec600_struct.send_count++;
            /* 填写需要发送的信息 */
            Message_Write();
//            /* FATFS填写txt文件 */
//            rt_sem_release(&fatfs_sem);
            /* EC600发送流程开始 */
            rt_device_write(dev_struct.uart1_dev, 0, "ATE0\r\n", rt_strlen("ATE0\r\n"));LOG_D("send-ATE0\n");
            OLED_ShowStr(60, 0,(uint8_t*)&"SENDING.", 1);
            break;
        }

        case EC600_STEP_ATQICSGP:{rt_device_write(dev_struct.uart1_dev, 0,
                "AT+QICSGP=1,1,\"CTWAP\",\"\",\"\",1\r\n", rt_strlen("AT+QICSGP=1,1,\"CTWAP\",\"\",\"\",1\r\n"));LOG_D("send-QICSGP\n");break;}
        case EC600_STEP_QIACT:{rt_device_write(dev_struct.uart1_dev, 0,
                "AT+QIACT=1\r\n", rt_strlen("AT+QIACT=1\r\n"));LOG_D("send-QIACT\n");break;}
        case EC600_STEP_QIOPEN:{
            /* 测试 */
            uint8_t sbuf[60],s1[]="AT+QIOPEN=1,0,\"TCP\",\"",s2[]=",0,1\r\n";
            uint8_t *p,*d;
            uint8_t bt;
            p=sbuf;
            d=s1;
            for(int i=0;i<21;i++){
                *p=*d;p++;d++;
            }
            bt=21;
            d=menu_struct.setting_p.ip_add;
            for(int i=0;i<15;i++){
                if(*d==' '){
                    d++;
                    continue;
                }
                else{
                    *p=*d;p++;d++;
                    bt++;
                }
            }
            *p='\"';p++;
            *p=',';p++;
            bt=bt+2;
            d=menu_struct.setting_p.ip_port;
            for(int i=0;i<4;i++){
                if(*d==' '){
                    d++;
                    continue;
                }
                else{
                    *p=*d;p++;d++;
                    bt++;
                }
            }
            d=s2;
            for(int i=0;i<6;i++){
                *p=*d;p++;d++;
                bt++;
            }
            rt_kprintf("data is %s, bit is %d",sbuf,bt);
            rt_device_write(dev_struct.uart1_dev, 0,sbuf, bt);

//                rt_device_write(dev_struct.uart1_dev, 0,
//                        "AT+QIOPEN=1,0,\"TCP\",\"120.25.207.40\",9190,0,1\r\n", rt_strlen("AT+QIOPEN=1,0,\"TCP\",\"120.25.207.40\",9190,0,1\r\n"));

                LOG_D("send-QIOPEN\n");break;}
        case EC600_STEP_QISEND:{rt_device_write(dev_struct.uart1_dev, 0,
                "AT+QISEND=0\r\n", rt_strlen("AT+QISEND=0\r\n"));LOG_D("send-QISEND\n");break;}
        case EC600_STEP_0X1A:{
            uint8_t endcode=0x1A;

            if(ec600_struct.send_error_count==0){   //不需要补发
                rt_device_write(dev_struct.uart1_dev, 0,ec600_struct.message, rt_strlen((const char*)ec600_struct.message));
                rt_thread_mdelay(100);
                rt_device_write(dev_struct.uart1_dev, 0,(const char*)&endcode, 1);
                LOG_D("send-QISEND\n");
                break;
            }


            /* 补发，最多只能发48条30字节，就发30条吧 */
            fatfs_struct.setting_fat.set_type = FATFS_FAILERTXT_READ;
            rt_sem_release(&fatfs_sem);
            break;
        }
        case EC600_STEP_QICLOSE:{
            LOG_D("success!\n");
            /* 发送成功 */
            ec600_struct.send_message_stat=SEND_MESSAGE_STAT_SUCCESS;
            //有补发
            if((ec600_struct.send_error_count<=30)&&(ec600_struct.send_error_count!=0)){
                ec600_struct.send_error_fatfs_lsk=ec600_struct.send_error_fatfs_lsk+ec600_struct.send_error_count*30;
                ec600_struct.send_error_count=0;
            }
            if(ec600_struct.send_error_count>30){
                ec600_struct.send_error_fatfs_lsk=ec600_struct.send_error_fatfs_lsk+30*30;
                ec600_struct.send_error_count=ec600_struct.send_error_count-30;
            }

            OLED_ShowStr(60, 0,(uint8_t*)&"WORKING.", 1);
            rt_sem_release(&sem_struct.ec600_pwk_sem);
            break;
        }


        /* NTP获取时间 */
        case EC600_STEP_NTP_ATE0:{
            rt_device_write(dev_struct.uart1_dev, 0, "ATE0\r\n", rt_strlen("ATE0\r\n"));
            LOG_D("send-ATE0\n");
            break;
        }
        case EC600_STEP_NTP_QIACT:{
            rt_device_write(dev_struct.uart1_dev, 0,"AT+QIACT=1\r\n", rt_strlen("AT+QIACT=1\r\n"));
            LOG_D("send-QIACT\n");
            break;
        }
        case EC600_STEP_NTP_QNTP:{
            rt_device_write(dev_struct.uart1_dev, 0,"AT+QNTP=1,\"ntp1.aliyun.com\",123\r\n", rt_strlen("AT+QNTP=1,\"ntp1.aliyun.com\",123\r\n"));
            LOG_D("send-QNTP\n");
            break;
        }
        case EC600_STEP_NTP_QIDEACT:{
            rt_device_write(dev_struct.uart1_dev, 0,"AT+QIDEACT=1\r\n", rt_strlen("AT+QIDEACT=1\r\n"));
            LOG_D("send-QIDEACT\n");
            break;
        }
        case EC600_STEP_NTP_DONE:{
            /* NTP获取时间成功 */
            LOG_D("ntp get time success!\n");
            rt_timer_stop(tim_struct.ec600_tim.ntpm_ot_tm);
            /* 重置 */
            rxbuf_struct.rbit=0;
            memset(rxbuf_struct.rxbuf,0,sizeof(rxbuf_struct.rxbuf));
            ec600_struct.step=0;

            OLED_ShowCNX24(0,3,X24_Line_Clear,5);
            OLED_ShowStr(60, 0,(uint8_t*)&"READY!  ", 1);
            online_send_stat = ONLINE_SEND_READY;

            work_struct.work_stat = WORK_STAT_ONLINE_WORK;
            /* 开启采集温度 */
            rt_timer_start(tim_struct.temp_tim.temp_tm);

            /* 开启显示时间 */
            rt_timer_start(tim_struct.oled_tim.time_reflash_tm);
//            /* 开始发送 */
//            rt_sem_release(&sem_struct.ec600_sendm_sem);
//            rt_timer_start(tim_struct.ec600_tim.sendm_ot_tm);
            break;
        }
        }
    }
}

/* ec600 turn off */
void PWK_th_entry(void*parameter)
{
    while(1){
        rt_sem_take(&sem_struct.ec600_pwk_sem, RT_WAITING_FOREVER);

        /* 关闭30秒计时器 */
        rt_timer_stop(tim_struct.ec600_tim.sendm_ot_tm);

        /* FATFS填写txt文件 */
        rt_sem_release(&fatfs_sem);

        /* 重置 */
        rxbuf_struct.rbit=0;
        memset(rxbuf_struct.rxbuf,0,sizeof(rxbuf_struct.rxbuf));
        ec600_struct.step=0;

        OLED_ShowStr(60, 0,(uint8_t*)&"WORKING.", 1);

        /* 关闭EC600 */
        EC600_PWK_LOW();
        rt_thread_mdelay(2);
        EC600_PWK_HIGH();
        rt_thread_mdelay(700);
        EC600_PWK_LOW();

        /* 开启5分钟计时器 */
        rt_timer_start(tim_struct.ec600_tim.pwk_on_tm);

    }
}
/**********************************************************************/

/**********************************************************************/
void GET_TEMP_th_entry(void*parameter)
{
    int16_t dat;
    while(1){
        rt_thread_mdelay(1000); //1s读取一次
        dat =  DS18B20_Get_Temp();
        temp_struct.i32_temp = (int32_t)dat;
        if(temp_struct.i32_temp>=0){
            temp_struct.temp_sign=0;
        }
        else{
            temp_struct.temp_sign=1;
        }
        temp_struct.u32_temp = (uint32_t)abs(temp_struct.i32_temp);
        OLED_ShowTempX32(temp_struct.u32_temp , temp_struct.temp_sign);
    }
}
/**********************************************************************/

void ADC1_th_entry(void*parameter)
{
    while(1){
        rt_sem_take(&sem_struct.adc1_sem, RT_WAITING_FOREVER);
        rt_adc_enable(dev_struct.adc_dev.adc_dev, 1);
        dev_struct.adc_dev.value=rt_adc_read(dev_struct.adc_dev.adc_dev, 1);
        dev_struct.adc_dev.vol = dev_struct.adc_dev.value*330/4096; //放大了100倍
//        rt_kprintf("the voltage is :%d.%02d \n", dev_struct.adc_dev.vol / 100, dev_struct.adc_dev.vol % 100);
//        rt_kprintf("the voltage is :%d\n",dev_struct.adc_dev.vol);
        rt_adc_disable(dev_struct.adc_dev.adc_dev, 1);
        Battery_Display(dev_struct.adc_dev.vol);
    }
}
void adc_on_tm_callback(void*parameter)
{
    rt_sem_release(&sem_struct.adc1_sem);
}
/**********************************************************************/
/* ec600 turn on timer callback */
void pwk_on_tm_callback(void*parameter)
{
    /* 开机EC600 */
    EC600_PWK_HIGH();

    /* 等待20秒寻找网络· */
    delay_secs(20);

    /* 保险起见多重置一次 */
    rxbuf_struct.rbit=0;
    memset(rxbuf_struct.rxbuf,0,sizeof(rxbuf_struct.rxbuf));
    ec600_struct.step=0;

    /* 开始发送 */
    rt_sem_release(&sem_struct.ec600_sendm_sem);
    rt_timer_start(tim_struct.ec600_tim.sendm_ot_tm);
}

/* ec600 send message over time callback */
void sendm_ot_tm_callback(void*parameter)
{
    LOG_D("this message send failed...\n");
    ec600_struct.send_message_stat=SEND_MESSAGE_STAT_FAILED;
    rt_sem_release(&sem_struct.ec600_pwk_sem);
}

/* 温度计采集温度定时回调函数 - 首选 */
void temp_tm_callback(void*parameter)
{
    int16_t dat;
    dat =  DS18B20_Get_Temp();
    temp_struct.i32_temp = (int32_t)dat;
    if(temp_struct.i32_temp>=0){
        temp_struct.temp_sign=TEMP_SIGN_POSITIVE;
    }
    else{
        temp_struct.temp_sign=TEMP_SIGN_NEGATIVE;
    }
    temp_struct.u32_temp = (uint32_t)abs(temp_struct.i32_temp);
    OLED_ShowTempX32(temp_struct.u32_temp , temp_struct.temp_sign);
//    /* 尝试把显示时间也放到这里，但需要二者同为1秒显示时才用  */
//    OLED_ShowTime();
    return;
}
/**********************************************************************/

/**********************************************************************/
void time_reflash_tm_callback(void*parameter)
{
    OLED_ShowTime();
    return;
}
/**********************************************************************/

/**********************************************************************/
void ntpm_ot_tm_callback(void*parameter)
{
    LOG_D("NTP get time failed...\n");

    menu_struct.menu_stat = MENU_STAT_NTP_PAGE;
    work_struct.work_stat = WORK_STAT_MENU;
    /* 关闭EC600 */
    EC600_PWK_LOW();
    rt_thread_mdelay(2);
    EC600_PWK_HIGH();
    rt_thread_mdelay(700);
    EC600_PWK_LOW();

    OLED_ShowCNX24(0,3,X24_Hanzi_NTPShiBai,5);
    OLED_ShowStr(60, 0,(uint8_t*)&"FAILED! ", 1);

    return;
}
/**********************************************************************/

/**********************************************************************/
void FATFS_th_enty(void*parameter)
{
    while(1){
        rt_sem_take(&fatfs_sem, RT_WAITING_FOREVER);

        int fd;
        if(fatfs_struct.setting_fat.set_type!=FATFS_SET_NONE){
            switch(fatfs_struct.setting_fat.set_type){
            case FATFS_SET_SERIALNUM:{
                fd = open("/setting.txt", O_WRONLY );
                if (fd>= 0)
                {
                    lseek(fd, FATFS_SET_LSK_SERIALNUM, SEEK_SET);
                    write(fd, menu_struct.setting_p.serialnum, sizeof(menu_struct.setting_p.serialnum));
                    close(fd);
                    rt_kprintf("Write SerialNum Setting done.\n");
                }
                break;
            }
            case FATFS_SET_IP:{
                fd = open("/setting.txt", O_WRONLY | O_CREAT);
                if(fd>=0){
                    lseek(fd,FATFS_SET_LSK_IP,SEEK_SET);
                    write(fd, menu_struct.setting_p.ip_add, sizeof(menu_struct.setting_p.ip_add));
                    lseek(fd,FATFS_SET_LSK_IPPORT,SEEK_SET);
                    write(fd, menu_struct.setting_p.ip_port, sizeof(menu_struct.setting_p.ip_port));
                    close(fd);
                    rt_kprintf("Write IP Setting done.\n");
                }
                break;
            }

            case FATFS_SET_READ:{
                fd = open("/setting.txt", O_RDWR );
                if(fd>=0){
                    lseek(fd, FATFS_SET_LSK_SERIALNUM, SEEK_SET);
                    read(fd, menu_struct.setting_p.serialnum, sizeof(menu_struct.setting_p.serialnum));
                    lseek(fd,FATFS_SET_LSK_IP,SEEK_SET);
                    read(fd, menu_struct.setting_p.ip_add, sizeof(menu_struct.setting_p.ip_add));
                    lseek(fd,FATFS_SET_LSK_IPPORT,SEEK_SET);
                    read(fd, menu_struct.setting_p.ip_port, sizeof(menu_struct.setting_p.ip_port));
                    close(fd);
                    rt_kprintf("Read Setting done.\n");
                }
                break;
            }

            case FATFS_FAILERTXT_READ:{
                fd = open("/eror/failed.txt", O_RDWR);
                if(fd>=0){
                    uint8_t endcode=0x1A;
                    uint32_t lsk;lsk=ec600_struct.send_error_fatfs_lsk;
                    if(ec600_struct.send_error_count<=30){
                        for(int i=0;i<ec600_struct.send_error_count;i++){
                            lseek(fd, lsk, SEEK_SET);
                            read(fd, ec600_struct.failed_message[i], 30);
                            lsk=lsk+EC600_MESSAGE_LENGTH;
                        }
                        for(int i=0;i<ec600_struct.send_error_count;i++){
                            rt_device_write(dev_struct.uart1_dev, 0,ec600_struct.failed_message[i], rt_strlen((const char*)ec600_struct.failed_message[i]));
                            rt_thread_mdelay(10);
                        }
                        rt_device_write(dev_struct.uart1_dev, 0,(const char*)&endcode, 1);
                        LOG_D("send-QISEND\n");
                        break;
                    }
                    else{
                        for(int i=0;i<30;i++){
                            lseek(fd, lsk, SEEK_SET);
                            read(fd, ec600_struct.failed_message[i], 30);
                            lsk=lsk+EC600_MESSAGE_LENGTH;
                        }
                        for(int i=0;i<30;i++){
                            rt_device_write(dev_struct.uart1_dev, 0,ec600_struct.failed_message[i], rt_strlen((const char*)ec600_struct.failed_message[i]));
                            rt_thread_mdelay(100);
                        }
                        rt_device_write(dev_struct.uart1_dev, 0,(const char*)&endcode, 1);
                        LOG_D("send-QISEND\n");
                        break;
                    }
                }
                break;
            }
            }
            fatfs_struct.setting_fat.set_type=FATFS_SET_NONE;
            continue;
        }

        if(ec600_struct.send_message_stat==SEND_MESSAGE_STAT_SUCCESS){
            fd = open((const char *)fatfs_struct.path, O_WRONLY |O_APPEND | O_CREAT);
            if (fd>= 0)
            {
                write(fd, ec600_struct.message, sizeof(ec600_struct.message));
                close(fd);
                rt_kprintf("Write done.\n");
            }
            ec600_struct.send_message_stat=SEND_MESSAGE_STAT_INIT;
            continue;
        }

        if(ec600_struct.send_message_stat==SEND_MESSAGE_STAT_FAILED){
            ec600_struct.send_error_count++;
            fd = open((const char *)fatfs_struct.path, O_WRONLY |O_APPEND | O_CREAT);
            if (fd>= 0)
            {
                write(fd, ec600_struct.message, sizeof(ec600_struct.message));
                close(fd);
                rt_kprintf("Write done.\n");
            }

            if(delete_failed_txt==0){
                delete_failed_txt=1;    //第一次会先删除旧的failed.txt
                unlink("/eror/failed.txt");
            }

            fd = open("/eror/failed.txt", O_WRONLY |O_APPEND | O_CREAT);
            if (fd>= 0)
            {
                write(fd, ec600_struct.message, sizeof(ec600_struct.message));
                close(fd);
                rt_kprintf("Write done.\n");
            }
            ec600_struct.send_message_stat=SEND_MESSAGE_STAT_INIT;
            continue;
        }


    }
    return;
}
/**********************************************************************/

/**********************************************************************/
int main(void)
{
    HAL_Init();
//    SYSTEM_PWR_EN_OFF();
    MX_GPIO_Init();
    SYSTEM_PWR_EN_ON();

    OLED_Init();
    OLED_Clear();
//
//    /* 开始界面 */
////    OLED_ShowStr(1,0,(uint8_t*)"[III]",1);
    OLED_ShowCNX16(30,3,X16_Hanzi_AoCeKeJi,4);
    OLED_ShowStr(2, 6,(uint8_t*)&"AUTOTEST TECHNOLOGY", 1);


    /* ADC测电池电量 */
    tim_struct.adc_tim.adc_on_tm = rt_timer_create("adc_on_tm", adc_on_tm_callback, NULL, TIMER_ADC_ON_INTERVAL,RT_TIMER_FLAG_PERIODIC|RT_TIMER_FLAG_SOFT_TIMER);
    rt_timer_start(tim_struct.adc_tim.adc_on_tm);
    rt_sem_init(&sem_struct.adc1_sem, "adc1_sem", 1, RT_IPC_FLAG_FIFO);
    dev_struct.adc_dev.adc_dev=(rt_adc_device_t)rt_device_find("adc1");
    th_struct.dev_tp.adc1_th_entry=ADC1_th_entry;
    th_struct.dev_tp.adc1_th = rt_thread_create("adc1_th", th_struct.dev_tp.adc1_th_entry, NULL, 2048, 10, 5);
//    rt_thread_startup(th_struct.dev_tp.adc1_th);



    /* */
    while(system_stat==0);
    if(system_stat==SYSTEM_STAT_USB){
        mount_to_usb();
    }
    else{
//        /* 测试用初始设置 */
//        uint8_t *p,*d,sn[]="00001";
//        p=menu_struct.setting_p.serialnum;d=sn;
//        for(int i=0;i<5;i++){
//            *p=*d;p++;d++;
//        }
//        uint8_t ipadd[]="120. 25.207. 40";
//        p=menu_struct.setting_p.ip_add;d=ipadd;
//        for(int i=0;i<15;i++){
//            *p=*d;p++;d++;
//        }
//        uint8_t iport[]="9190";
//        p=menu_struct.setting_p.ip_port;d=iport;
//        for(int i=0;i<4;i++){
//            *p=*d;p++;d++;
//        }

        /* UART1 device */
        dev_struct.uart1_dev=rt_device_find("uart1");
        if(dev_struct.uart1_dev==RT_NULL){
            LOG_E("find uart1 error\n");
            return -EINVAL;
        }
        else{
            LOG_D("find uart1 done\n");
        }

        dev_struct.uart1_ret=rt_device_open(dev_struct.uart1_dev, RT_DEVICE_OFLAG_RDWR|RT_DEVICE_FLAG_INT_RX);
        if(dev_struct.uart1_ret<0){
            LOG_E("open uart1 error\n");
            return dev_struct.uart1_ret;
        }
        else{
            LOG_D("open uart1 done\n");
        }
        rt_device_control(dev_struct.uart1_dev, RT_DEVICE_CTRL_CONFIG, (void*)&uart1_configs);

        rt_device_set_rx_indicate(dev_struct.uart1_dev,uart1rx_callback);
        rt_sem_init(&sem_struct.uart1rx_sem, "uart1rx_sem", 0, RT_IPC_FLAG_FIFO);
        th_struct.dev_tp.uart1recv_th_entry = UART1RECV_th_entry;
        th_struct.dev_tp.uart1recv_th=rt_thread_create("uart1_recv", th_struct.dev_tp.uart1recv_th_entry, NULL, 2048, 10, 5); //创建线程
        rt_thread_startup(th_struct.dev_tp.uart1recv_th);


        /* 线程：ec600 send message */
        rt_sem_init(&sem_struct.ec600_sendm_sem, "sendm_sem", 0, RT_IPC_FLAG_FIFO);
        th_struct.ec600_tp.sendm_th_entry = EC600_SENDM_th_entry;
        th_struct.ec600_tp.sendm_th = rt_thread_create("sendm_th", th_struct.ec600_tp.sendm_th_entry, NULL, 4096, 10, 5);
        rt_thread_startup(th_struct.ec600_tp.sendm_th);
        /* 计时器：ec600 send message */
    //    tim_struct.ec600_tim.sendm_tm = rt_timer_create("sendm_tm", sendm_tm_callback, NULL, TIMER_SENDM_OT_INTERVAL,RT_TIMER_FLAG_ONE_SHOT|RT_TIMER_FLAG_SOFT_TIMER);

        /* ec600 turn off */
        rt_sem_init(&sem_struct.ec600_pwk_sem,"ec600_pwk_sem",0,RT_IPC_FLAG_FIFO);
        th_struct.ec600_tp.pwk_th_entry = PWK_th_entry;
        th_struct.ec600_tp.pwk_th=rt_thread_create("pwk_th", th_struct.ec600_tp.pwk_th_entry, NULL, 1024, 10, 5);
        rt_thread_startup(th_struct.ec600_tp.pwk_th);

        /* ec600 turn on ， 此计时器用于数5分钟*/
        tim_struct.ec600_tim.pwk_on_tm = rt_timer_create("pwk_on_tm", pwk_on_tm_callback, NULL, TIMER_PWK_ON_INTERVAL,RT_TIMER_FLAG_ONE_SHOT|RT_TIMER_FLAG_SOFT_TIMER);
        /* ec600发送一次数据，只等30秒，超过则视为发送失败直接关机。此计时器用于数30秒  */
        tim_struct.ec600_tim.sendm_ot_tm = rt_timer_create("sendm_ot_tm", sendm_ot_tm_callback, NULL, TIMER_SENDM_OT_INTERVAL,RT_TIMER_FLAG_ONE_SHOT|RT_TIMER_FLAG_SOFT_TIMER);

        /* NTP获取时间时，等待45秒，超过这个时间则获取NTP时间失败 */
        tim_struct.ec600_tim.ntpm_ot_tm = rt_timer_create("ntpm_ot_tm", ntpm_ot_tm_callback, NULL, TIMER_NTPM_INTERVAL,RT_TIMER_FLAG_ONE_SHOT|RT_TIMER_FLAG_SOFT_TIMER);

        /* 定时采集温度 */
        tim_struct.temp_tim.temp_tm = rt_timer_create("temp_tm", temp_tm_callback, NULL, TIMER_TEMP_INTERVAL,RT_TIMER_FLAG_PERIODIC|RT_TIMER_FLAG_SOFT_TIMER);
    //    /* 线程采集温度 */
    //    th_struct.ds18b20_tp.get_temp_th_entry = GET_TEMP_th_entry;
    //    th_struct.ds18b20_tp.get_temp_th = rt_thread_create("get_temp_th", th_struct.ds18b20_tp.get_temp_th_entry, NULL, 1024, 10, 5);
    //    rt_thread_startup(th_struct.ds18b20_tp.get_temp_th);

        /* 每隔1秒刷新屏幕上显示的时间 */
        tim_struct.oled_tim.time_reflash_tm = rt_timer_create("time_reflash_tm", time_reflash_tm_callback, NULL, TIMER_OLED_REFLASH_INTERVAL,RT_TIMER_FLAG_PERIODIC|RT_TIMER_FLAG_SOFT_TIMER);

        /* FATFS线程 */
        rt_sem_init(&fatfs_sem,"fatfs_sem",0,RT_IPC_FLAG_FIFO);
        th_struct.fatfs_tp.fatfs_th_entry = FATFS_th_enty;
        th_struct.fatfs_tp.fatfs_th=rt_thread_create("fatfs_th", th_struct.fatfs_tp.fatfs_th_entry, NULL, 4096, 10, 5);
        rt_thread_startup(th_struct.fatfs_tp.fatfs_th);

        /* 正式开启EC600 */
        EC600_PWK_HIGH();

        rt_kprintf("main done!\n");

        /* FATFS读设置 */
        fatfs_struct.setting_fat.set_type = FATFS_SET_READ;
        rt_sem_release(&fatfs_sem);

    }

    return RT_EOK;
}

/*
 * 延时sec秒
 */
void delay_secs(uint8_t sec)
{
    for(int i=0;i<sec;i++){
        rt_thread_mdelay(1000);
    }
    return;
}

/*
 * 按键中断回调函数
 */
void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin)
{
    if(GPIO_Pin==KEY_ON_PIN){
        rt_thread_mdelay(1);
        if(HAL_GPIO_ReadPin(KEY_ON_GPIO,KEY_ON_PIN)==KEY_ON_DOWN_LEVEL){
            if(key_on_stat == KEY_ON_STAT_ENABLE) {  //正常状态，且没在发送信息中，关机
    //                key_on_stat = KEY_ON_STAT_DISABLE;
                    /* 关闭EC600 */
                EC600_PWK_LOW();
                rt_thread_mdelay(2);
                EC600_PWK_HIGH();
                rt_thread_mdelay(700);
                EC600_PWK_LOW();
                    /* 关闭系统 */
                SYSTEM_PWR_EN_OFF();
            }
        }
        __HAL_GPIO_EXTI_CLEAR_IT(KEY_ON_PIN);
        return;
    }

    if(work_struct.work_stat == WORK_STAT_MENU){    //运行状态为菜单
        if(menu_struct.menu_stat == MENU_STAT_INIT_PAGE){//在初始化首页
            if(GPIO_Pin==KEY_1_PIN){
                rt_thread_mdelay(20);
                if(HAL_GPIO_ReadPin(KEY_N_GPIO,KEY_1_PIN)==KEY_1_DOWN_LEVEL){
                    switch(menu_struct.oled_p.screen){
                    case SCREEN_MENU_START:{
                        /* 在这里开启ADC */
                        rt_thread_startup(th_struct.dev_tp.adc1_th);

                        menu_struct.oled_p.screen = SCREEN_FIRSTMENU_THERM;
                        OLED_ShowCNX24(0,3,X24_Hanzi_WenDuJi,3);
                        OLED_ShowStr(60, 0,(uint8_t*)&"[_ ]     ", 1);
                        break;
                    }
                    case SCREEN_FIRSTMENU_THERM:{
                        menu_struct.oled_p.screen = SCREEN_FIRSTMENU_USB;
                        OLED_ShowCNX24(0,3,X24_Hanzi_UPan,2);
                        OLED_ShowStr(60, 0,(uint8_t*)&"[ _]     ", 1);
                        break;
                    }
                    case SCREEN_FIRSTMENU_USB:{
                        menu_struct.oled_p.screen = SCREEN_FIRSTMENU_THERM;
                        OLED_ShowCNX24(0,3,X24_Hanzi_WenDuJi,3);
                        OLED_ShowStr(60, 0,(uint8_t*)&"[_ ]     ", 1);
                        break;
                    }
                    }
                }
                __HAL_GPIO_EXTI_CLEAR_IT(KEY_1_PIN);
            }
            else if(GPIO_Pin==KEY_2_PIN){
                rt_thread_mdelay(20);
                if(HAL_GPIO_ReadPin(KEY_N_GPIO,KEY_2_PIN)==KEY_2_DOWN_LEVEL){
                    switch(menu_struct.oled_p.screen){
                    case SCREEN_MENU_START:{
                        menu_struct.oled_p.screen = SCREEN_FIRSTMENU_USB;
                        OLED_ShowCNX24(0,3,X24_Hanzi_UPan,2);
                        OLED_ShowStr(60, 0,(uint8_t*)&"[ _]     ", 1);
                        break;
                    }
                    case SCREEN_FIRSTMENU_THERM:{
                        menu_struct.oled_p.screen = SCREEN_FIRSTMENU_USB;
                        OLED_ShowCNX24(0,3,X24_Hanzi_UPan,2);
                        OLED_ShowStr(60, 0,(uint8_t*)&"[ _]     ", 1);
                        break;
                    }
                    case SCREEN_FIRSTMENU_USB:{
                        menu_struct.oled_p.screen = SCREEN_FIRSTMENU_THERM;
                        OLED_ShowCNX24(0,3,X24_Hanzi_WenDuJi,3);
                        OLED_ShowStr(60, 0,(uint8_t*)&"[_ ]     ", 1);
                        break;
                    }
                    }
                }
                __HAL_GPIO_EXTI_CLEAR_IT(KEY_2_PIN);
            }
            else if(GPIO_Pin==KEY_3_PIN){
                rt_thread_mdelay(20);
                if(HAL_GPIO_ReadPin(KEY_N_GPIO,KEY_3_PIN)==KEY_3_DOWN_LEVEL){
                    switch(menu_struct.oled_p.screen){
                    case SCREEN_FIRSTMENU_THERM:{
                        system_stat=SYSTEM_STAT_THERM;
                        menu_struct.menu_stat = MENU_STAT_FIRST_PAGE;
                        menu_struct.oled_p.screen = SCREEN_MENU_CYBERWORK;
                        OLED_ShowCNX24(0,3,X24_Hanzi_LianWang,4);
                        OLED_ShowStr(60, 0,(uint8_t*)&"[_     ]", 1);
                        break;
                    }
                    case SCREEN_FIRSTMENU_USB:{
                        system_stat=SYSTEM_STAT_USB;
                        work_struct.work_stat = WORK_STAT_USB; //0927-
                        menu_struct.oled_p.screen = SCREEN_USB_A;
                        OLED_ShowStr(60, 0,(uint8_t*)&"USB     ", 1);
                        for(int i=0;i<5;i++){   //清理一行
                            ShowCNX24(i*24,3,i,X24_Line_Clear);
                        }
                        for(int i=0;i<5;i++){   //清理一行
                            ShowCNX24(i*24,3+2,i,X24_Line_Clear);
                        }
                        OLED_ShowCNX16(10,3,X16_Hanzi_UPanShiYongZhong,5);
                        OLED_ShowStr(90, 4,(uint8_t*)&"...", 1);
                        break;
                    }
                    }
                }
                __HAL_GPIO_EXTI_CLEAR_IT(KEY_3_PIN);
            }

        }

        else if(menu_struct.menu_stat == MENU_STAT_FIRST_PAGE){  //在菜单首页
            if(GPIO_Pin==KEY_1_PIN){
                rt_thread_mdelay(20);
                if(HAL_GPIO_ReadPin(KEY_N_GPIO,KEY_1_PIN)==KEY_1_DOWN_LEVEL){
                    switch(menu_struct.oled_p.screen){
                    case SCREEN_MENU_START:{
                        menu_struct.oled_p.screen = SCREEN_MENU_CYBERWORK;
                        OLED_ShowCNX24(0,3,X24_Hanzi_LianWang,4);
                        OLED_ShowStr(60, 0,(uint8_t*)&"[_     ]", 1);
                        break;
                    }
                    case SCREEN_MENU_CYBERWORK:{
                        menu_struct.oled_p.screen = SCREEN_MENU_OFFLINEWORK;
                        OLED_ShowCNX24(0,3,X24_Hanzi_LiXian,4);
                        OLED_ShowStr(60, 0,(uint8_t*)&"[ _    ]", 1);
                        break;
                    }
                    case SCREEN_MENU_OFFLINEWORK:{
                        menu_struct.oled_p.screen = SCREEN_MENU_SERIALNUM;
                        OLED_ShowCNX24(0,3,X24_Hanzi_BianHao,4);
                        OLED_ShowStr(60, 0,(uint8_t*)&"[  _   ]", 1);
                        break;
                    }
                    case SCREEN_MENU_SERIALNUM:{
                        menu_struct.oled_p.screen = SCREEN_MENU_IPSET;
                        OLED_ShowCNX24(0,3,X24_Hanzi_IPSheZhi,4);
                        OLED_ShowStr(60, 0,(uint8_t*)&"[   _  ]", 1);

                        break;
                    }
                    case SCREEN_MENU_IPSET:{
                        menu_struct.oled_p.screen = SCREEN_MENU_NTPSET;
                        OLED_ShowCNX24(0,3,X24_Hanzi_NTPSheZhi,5);
                        OLED_ShowStr(60, 0,(uint8_t*)&"[    _ ]", 1);
                        break;
                    }
                    case SCREEN_MENU_NTPSET:{
                        menu_struct.oled_p.screen = SCREEN_MENU_POWEROFF;
                        OLED_ShowCNX24(0,3,X24_Hanzi_GuanJi,2);
                        OLED_ShowStr(60, 0,(uint8_t*)&"[     _]", 1);
                        /* 允许关机 */
                        key_on_stat = KEY_ON_STAT_ENABLE;
                        break;
                    }
                    case SCREEN_MENU_POWEROFF:{
                        menu_struct.oled_p.screen = SCREEN_MENU_CYBERWORK;
                        OLED_ShowCNX24(0,3,X24_Hanzi_LianWang,4);
                        OLED_ShowStr(60, 0,(uint8_t*)&"[_     ]", 1);
                        key_on_stat = KEY_ON_STAT_DISABLE;
                        break;
                    }
                    }
                }
                __HAL_GPIO_EXTI_CLEAR_IT(KEY_1_PIN);
            }

            else if(GPIO_Pin==KEY_2_PIN){
                rt_thread_mdelay(20);
                if(HAL_GPIO_ReadPin(KEY_N_GPIO,KEY_2_PIN)==KEY_2_DOWN_LEVEL){
                    switch(menu_struct.oled_p.screen){
                    case SCREEN_MENU_START:{
                        menu_struct.oled_p.screen = SCREEN_MENU_POWEROFF;
                        OLED_ShowCNX24(0,3,X24_Hanzi_GuanJi,2);
                        OLED_ShowStr(60, 0,(uint8_t*)&"[     _]", 1);
                        /* 允许关机 */
                        key_on_stat = KEY_ON_STAT_ENABLE;
                        break;
                    }
                    case SCREEN_MENU_POWEROFF:{
                        menu_struct.oled_p.screen = SCREEN_MENU_NTPSET;
                        OLED_ShowCNX24(0,3,X24_Hanzi_NTPSheZhi,5);
                        OLED_ShowStr(60, 0,(uint8_t*)&"[    _ ]", 1);
                        break;
                    }
                    case SCREEN_MENU_NTPSET:{
                        menu_struct.oled_p.screen = SCREEN_MENU_IPSET;
                        OLED_ShowCNX24(0,3,X24_Hanzi_IPSheZhi,4);
                        OLED_ShowStr(60, 0,(uint8_t*)&"[   _  ]", 1);
                        break;
                    }
                    case SCREEN_MENU_IPSET:{
                        menu_struct.oled_p.screen = SCREEN_MENU_SERIALNUM;
                        OLED_ShowCNX24(0,3,X24_Hanzi_BianHao,4);
                        OLED_ShowStr(60, 0,(uint8_t*)&"[  _   ]", 1);
                        break;
                    }
                    case SCREEN_MENU_SERIALNUM:{
                        menu_struct.oled_p.screen = SCREEN_MENU_OFFLINEWORK;
                        OLED_ShowCNX24(0,3,X24_Hanzi_LiXian,4);
                        OLED_ShowStr(60, 0,(uint8_t*)&"[ _    ]", 1);
                        break;
                    }
                    case SCREEN_MENU_OFFLINEWORK:{
                        menu_struct.oled_p.screen = SCREEN_MENU_CYBERWORK;
                        OLED_ShowCNX24(0,3,X24_Hanzi_LianWang,4);
                        OLED_ShowStr(60, 0,(uint8_t*)&"[_     ]", 1);
                        break;
                    }
                    case SCREEN_MENU_CYBERWORK:{
                        menu_struct.oled_p.screen = SCREEN_MENU_POWEROFF;
                        OLED_ShowCNX24(0,3,X24_Hanzi_GuanJi,2);
                        OLED_ShowStr(60, 0,(uint8_t*)&"[     _]", 1);
                        key_on_stat = KEY_ON_STAT_ENABLE;
                        break;
                    }
                    }
                }
                __HAL_GPIO_EXTI_CLEAR_IT(KEY_2_PIN);
            }

            else if(GPIO_Pin==KEY_3_PIN){
                rt_thread_mdelay(20);
                if(HAL_GPIO_ReadPin(KEY_N_GPIO,KEY_3_PIN)==KEY_3_DOWN_LEVEL){
                    switch(menu_struct.oled_p.screen){
                    case SCREEN_MENU_CYBERWORK:{
                        menu_struct.menu_stat = MENU_STAT_NTP_PAGE;  //进入NTP菜单
                        menu_struct.oled_p.screen = SCREEN_NTPMENU_ONLINE;
                        OLED_ShowCNX24(0,3,X24_Hanzi_NTPShiJian,5);
                        OLED_ShowStr(60, 0,(uint8_t*)&"[_ ]    ", 1);
                        break;
                    }
                    case SCREEN_MENU_OFFLINEWORK:{
                        work_struct.work_stat = WORK_STAT_OFFLINE_WORK;
                        menu_struct.menu_stat = MENU_STAT_END;
                        OLED_ShowCNX24(0,3,X24_Line_Clear,5);
                        /* 开启采集温度 */
                        rt_timer_start(tim_struct.temp_tim.temp_tm);
//                        rt_thread_startup(th_struct.ds18b20_tp.get_temp_th);
                        break;
                    }
                    case SCREEN_MENU_SERIALNUM:{
                        /* 编号页面 */
                        menu_struct.menu_stat = MENU_STAT_SERIALNUM_PAGE;
                        menu_struct.oled_p.screen=SCREEN_SERIALNUMMENU_HORI;
                        menu_struct.setting_p.serialnum_bit=0;
                        for(int i=0;i<5;i++){   //清理一行
                            ShowCNX24(i*24,3,i,X24_Line_Clear);
                        }
                        for(int i=0;i<5;i++){   //清理一行
                            ShowCNX24(i*24,3+2,i,X24_Line_Clear);
                        }
                        OLED_ShowSerialNumX32(menu_struct.setting_p.serialnum);
                        OLED_ShowStr(1, 6,(uint8_t*)&" ^            ", 1);
                        break;
                    }
                    case SCREEN_MENU_IPSET:{
                        /* IP设置页面 */
                        menu_struct.menu_stat = MENU_STAT_IP_SET_PAGE;
                        menu_struct.oled_p.screen=SCREEN_IPSETMENU_HORI;
                        for(int i=0;i<5;i++){   //清理一行
                            ShowCNX24(i*24,3,i,X24_Line_Clear);
                        }
                        for(int i=0;i<5;i++){   //清理一行
                            ShowCNX24(i*24,3+2,i,X24_Line_Clear);
                        }
                        /* 获取设置的IP地址 */
                        OLED_ShowAdd(0,3,menu_struct.setting_p.ip_add,0);
                        OLED_ShowStr(0, 6,(uint8_t*)&"^               ", 1);
                        break;
                    }
                    }
                }
                __HAL_GPIO_EXTI_CLEAR_IT(KEY_3_PIN);
            }
        }
        /* IP设置菜单 */
        else if(menu_struct.menu_stat == MENU_STAT_IP_SET_PAGE){
            if(GPIO_Pin==KEY_1_PIN){
                rt_thread_mdelay(20);
                if(HAL_GPIO_ReadPin(KEY_N_GPIO,KEY_1_PIN)==KEY_1_DOWN_LEVEL){
                    switch(menu_struct.oled_p.screen){
                    case SCREEN_IPSETMENU_HORI:{
                        if(menu_struct.setting_p.ip_add_bit<18){
                            menu_struct.setting_p.ip_add_bit++;
                        }
                        if(menu_struct.setting_p.ip_add_bit<15){
                            if(menu_struct.setting_p.ip_add[menu_struct.setting_p.ip_add_bit]=='.'){
                                menu_struct.setting_p.ip_add_bit++;
                            }
                        }
                        if(menu_struct.setting_p.ip_add_bit==12){
                            for(int i=0;i<5;i++){   //清理一行
                                ShowCNX24(i*24,3,i,X24_Line_Clear);
                            }
                            for(int i=0;i<5;i++){   //清理一行
                                ShowCNX24(i*24,3+2,i,X24_Line_Clear);
                            }
                            OLED_ShowAdd(0,3,menu_struct.setting_p.ip_add,1);
                            OLED_ShowAdd(0,3,menu_struct.setting_p.ip_port,2);
                        }
                        switch(menu_struct.setting_p.ip_add_bit){
                        case 0:{OLED_ShowStr(0, 6,(uint8_t*)&"^                   ", 1);break;}
                        case 1:{OLED_ShowStr(0, 6,(uint8_t*)&"  ^                 ", 1);break;}
                        case 2:{OLED_ShowStr(0, 6,(uint8_t*)&"   ^                ", 1);break;}
                        case 4:{OLED_ShowStr(2, 6,(uint8_t*)&"      ^             ", 1);break;}
                        case 5:{OLED_ShowStr(0, 6,(uint8_t*)&"        ^           ", 1);break;}
                        case 6:{OLED_ShowStr(0, 6,(uint8_t*)&"          ^         ", 1);break;}
                        case 8:{OLED_ShowStr(8, 6,(uint8_t*)&"            ^       ", 1);break;}
                        case 9:{OLED_ShowStr(0, 6,(uint8_t*)&"               ^    ", 1);break;}
                        case 10:{OLED_ShowStr(0, 6,(uint8_t*)&"                 ^ ", 1);break;}

                        case 12:{OLED_ShowStr(0, 6,(uint8_t*)&"^                  ", 1);break;}
                        case 13:{OLED_ShowStr(0, 6,(uint8_t*)&"  ^                ", 1);break;}
                        case 14:{OLED_ShowStr(0, 6,(uint8_t*)&"   ^               ", 1);break;}

                        case 15:{OLED_ShowStr(0, 6,(uint8_t*)&"            ^      ", 1);break;}
                        case 16:{OLED_ShowStr(0, 6,(uint8_t*)&"             ^     ", 1);break;}
                        case 17:{OLED_ShowStr(0, 6,(uint8_t*)&"               ^   ", 1);break;}
                        case 18:{OLED_ShowStr(0, 6,(uint8_t*)&"                 ^ ", 1);break;}
                        }
                        break;
                    }

                    case SCREEN_IPSETMENU_VERT:{
                        if(menu_struct.setting_p.ip_add_bit>=15){
                            if((menu_struct.setting_p.ip_port[menu_struct.setting_p.ip_add_bit-15]<'9')&&(menu_struct.setting_p.ip_port[menu_struct.setting_p.ip_add_bit-15]!=' ')){
                                menu_struct.setting_p.ip_port[menu_struct.setting_p.ip_add_bit-15]++;
                            }
                            else if(menu_struct.setting_p.ip_port[menu_struct.setting_p.ip_add_bit-15]=='9'){
                                menu_struct.setting_p.ip_port[menu_struct.setting_p.ip_add_bit-15]=' ';
                            }
                            else{
                                menu_struct.setting_p.ip_port[menu_struct.setting_p.ip_add_bit-15]='0';
                            }
                            OLED_ShowAdd(0,3,menu_struct.setting_p.ip_port,2);
                        }
                        else{
                            if((menu_struct.setting_p.ip_add[menu_struct.setting_p.ip_add_bit]<'9')&&(menu_struct.setting_p.ip_add[menu_struct.setting_p.ip_add_bit]!=' ')){
                                menu_struct.setting_p.ip_add[menu_struct.setting_p.ip_add_bit]++;
                            }
                            else if(menu_struct.setting_p.ip_add[menu_struct.setting_p.ip_add_bit]=='9'){
                                menu_struct.setting_p.ip_add[menu_struct.setting_p.ip_add_bit]=' ';
                            }
                            else{
                                menu_struct.setting_p.ip_add[menu_struct.setting_p.ip_add_bit]='0';
                            }
                            if(menu_struct.setting_p.ip_add_bit<12){
                                OLED_ShowAdd(0,3,menu_struct.setting_p.ip_add,0);
                            }
                            else if(menu_struct.setting_p.ip_add_bit<15){
                                OLED_ShowAdd(0,3,menu_struct.setting_p.ip_add,1);
                            }
                        }
                        break;
                    }
                    }
                }
                __HAL_GPIO_EXTI_CLEAR_IT(KEY_1_PIN);
            }
            if(GPIO_Pin==KEY_2_PIN){
                rt_thread_mdelay(20);
                if(HAL_GPIO_ReadPin(KEY_N_GPIO,KEY_2_PIN)==KEY_2_DOWN_LEVEL){
                    switch(menu_struct.oled_p.screen){
                    case SCREEN_IPSETMENU_HORI:{
                        if(menu_struct.setting_p.ip_add_bit>0){
                            menu_struct.setting_p.ip_add_bit--;
                        }
                        if(menu_struct.setting_p.ip_add_bit<15){
                            if(menu_struct.setting_p.ip_add[menu_struct.setting_p.ip_add_bit]=='.'){
                                menu_struct.setting_p.ip_add_bit--;
                            }
                        }
                        if(menu_struct.setting_p.ip_add_bit==10){
                            for(int i=0;i<5;i++){   //清理一行
                                ShowCNX24(i*24,3,i,X24_Line_Clear);
                            }
                            for(int i=0;i<5;i++){   //清理一行
                                ShowCNX24(i*24,3+2,i,X24_Line_Clear);
                            }
                            OLED_ShowAdd(0,3,menu_struct.setting_p.ip_add,0);
                        }
                        switch(menu_struct.setting_p.ip_add_bit){
                        case 0:{OLED_ShowStr(0, 6,(uint8_t*)&"^                   ", 1);break;}
                        case 1:{OLED_ShowStr(0, 6,(uint8_t*)&"  ^                 ", 1);break;}
                        case 2:{OLED_ShowStr(0, 6,(uint8_t*)&"   ^                ", 1);break;}
                        case 4:{OLED_ShowStr(2, 6,(uint8_t*)&"      ^             ", 1);break;}
                        case 5:{OLED_ShowStr(0, 6,(uint8_t*)&"        ^           ", 1);break;}
                        case 6:{OLED_ShowStr(0, 6,(uint8_t*)&"          ^         ", 1);break;}
                        case 8:{OLED_ShowStr(8, 6,(uint8_t*)&"            ^       ", 1);break;}
                        case 9:{OLED_ShowStr(0, 6,(uint8_t*)&"               ^    ", 1);break;}
                        case 10:{OLED_ShowStr(0, 6,(uint8_t*)&"                 ^ ", 1);break;}

                        case 12:{OLED_ShowStr(0, 6,(uint8_t*)&"^                  ", 1);break;}
                        case 13:{OLED_ShowStr(0, 6,(uint8_t*)&"  ^                ", 1);break;}
                        case 14:{OLED_ShowStr(0, 6,(uint8_t*)&"   ^               ", 1);break;}

                        case 15:{OLED_ShowStr(0, 6,(uint8_t*)&"            ^      ", 1);break;}
                        case 16:{OLED_ShowStr(0, 6,(uint8_t*)&"             ^     ", 1);break;}
                        case 17:{OLED_ShowStr(0, 6,(uint8_t*)&"               ^   ", 1);break;}
                        case 18:{OLED_ShowStr(0, 6,(uint8_t*)&"                 ^ ", 1);break;}
                        }

                        break;
                    }
                    case SCREEN_IPSETMENU_VERT:{
                        if(menu_struct.setting_p.ip_add_bit>=15){
                            if((menu_struct.setting_p.ip_port[menu_struct.setting_p.ip_add_bit-15]>'0')&&(menu_struct.setting_p.ip_port[menu_struct.setting_p.ip_add_bit-15]!=' ')){
                                menu_struct.setting_p.ip_port[menu_struct.setting_p.ip_add_bit-15]--;
                            }
                            else if(menu_struct.setting_p.ip_port[menu_struct.setting_p.ip_add_bit-15]=='0'){
                                menu_struct.setting_p.ip_port[menu_struct.setting_p.ip_add_bit-15]=' ';
                            }
                            else{
                                menu_struct.setting_p.ip_port[menu_struct.setting_p.ip_add_bit-15]='9';
                            }
                            OLED_ShowAdd(0,3,menu_struct.setting_p.ip_port,2);
                        }
                        else{
                            if((menu_struct.setting_p.ip_add[menu_struct.setting_p.ip_add_bit]>'0')&&(menu_struct.setting_p.ip_add[menu_struct.setting_p.ip_add_bit]!=' ')){
                                menu_struct.setting_p.ip_add[menu_struct.setting_p.ip_add_bit]--;
                            }
                            else if(menu_struct.setting_p.ip_add[menu_struct.setting_p.ip_add_bit]=='0'){
                                menu_struct.setting_p.ip_add[menu_struct.setting_p.ip_add_bit]=' ';
                            }
                            else{
                                menu_struct.setting_p.ip_add[menu_struct.setting_p.ip_add_bit]='9';
                            }
                            if(menu_struct.setting_p.ip_add_bit<12){
                                OLED_ShowAdd(0,3,menu_struct.setting_p.ip_add,0);
                            }
                            else if(menu_struct.setting_p.ip_add_bit<15){
                                OLED_ShowAdd(0,3,menu_struct.setting_p.ip_add,1);
                            }
                        }
                        break;
                    }
                    }
                }
                __HAL_GPIO_EXTI_CLEAR_IT(KEY_2_PIN);
            }
            if(GPIO_Pin==KEY_3_PIN){
                rt_thread_mdelay(20);
                if(HAL_GPIO_ReadPin(KEY_N_GPIO,KEY_3_PIN)==KEY_3_DOWN_LEVEL){
                    switch(menu_struct.oled_p.screen){
                    case SCREEN_IPSETMENU_HORI:{
                        menu_struct.oled_p.screen = SCREEN_IPSETMENU_VERT;
                        switch(menu_struct.setting_p.ip_add_bit){
                        case 0:{OLED_ShowStr(0, 6,(uint8_t*)&"`                   ", 1);break;}
                        case 1:{OLED_ShowStr(0, 6,(uint8_t*)&"  `                 ", 1);break;}
                        case 2:{OLED_ShowStr(0, 6,(uint8_t*)&"   `                ", 1);break;}
                        case 4:{OLED_ShowStr(2, 6,(uint8_t*)&"      `             ", 1);break;}
                        case 5:{OLED_ShowStr(0, 6,(uint8_t*)&"        `           ", 1);break;}
                        case 6:{OLED_ShowStr(0, 6,(uint8_t*)&"          `         ", 1);break;}
                        case 8:{OLED_ShowStr(8, 6,(uint8_t*)&"            `       ", 1);break;}
                        case 9:{OLED_ShowStr(0, 6,(uint8_t*)&"               `    ", 1);break;}
                        case 10:{OLED_ShowStr(0, 6,(uint8_t*)&"                 ` ", 1);break;}

                        case 12:{OLED_ShowStr(0, 6,(uint8_t*)&"`                  ", 1);break;}
                        case 13:{OLED_ShowStr(0, 6,(uint8_t*)&"  `                ", 1);break;}
                        case 14:{OLED_ShowStr(0, 6,(uint8_t*)&"   `               ", 1);break;}

                        case 15:{OLED_ShowStr(0, 6,(uint8_t*)&"            `      ", 1);break;}
                        case 16:{OLED_ShowStr(0, 6,(uint8_t*)&"             `     ", 1);break;}
                        case 17:{OLED_ShowStr(0, 6,(uint8_t*)&"               `   ", 1);break;}
                        case 18:{OLED_ShowStr(0, 6,(uint8_t*)&"                 ` ", 1);break;}
                        }
                        break;
                    }
                    case SCREEN_IPSETMENU_VERT:{
                        menu_struct.oled_p.screen = SCREEN_IPSETMENU_HORI;
                        switch(menu_struct.setting_p.ip_add_bit){
                        case 0:{OLED_ShowStr(0, 6,(uint8_t*)&"^                   ", 1);break;}
                        case 1:{OLED_ShowStr(0, 6,(uint8_t*)&"  ^                 ", 1);break;}
                        case 2:{OLED_ShowStr(0, 6,(uint8_t*)&"   ^                ", 1);break;}
                        case 4:{OLED_ShowStr(2, 6,(uint8_t*)&"      ^             ", 1);break;}
                        case 5:{OLED_ShowStr(0, 6,(uint8_t*)&"        ^           ", 1);break;}
                        case 6:{OLED_ShowStr(0, 6,(uint8_t*)&"          ^         ", 1);break;}
                        case 8:{OLED_ShowStr(8, 6,(uint8_t*)&"            ^       ", 1);break;}
                        case 9:{OLED_ShowStr(0, 6,(uint8_t*)&"               ^    ", 1);break;}
                        case 10:{OLED_ShowStr(0, 6,(uint8_t*)&"                 ^ ", 1);break;}

                        case 12:{OLED_ShowStr(0, 6,(uint8_t*)&"^                  ", 1);break;}
                        case 13:{OLED_ShowStr(0, 6,(uint8_t*)&"  ^                ", 1);break;}
                        case 14:{OLED_ShowStr(0, 6,(uint8_t*)&"   ^               ", 1);break;}

                        case 15:{OLED_ShowStr(0, 6,(uint8_t*)&"            ^      ", 1);break;}
                        case 16:{OLED_ShowStr(0, 6,(uint8_t*)&"             ^     ", 1);break;}
                        case 17:{OLED_ShowStr(0, 6,(uint8_t*)&"               ^   ", 1);break;}
                        case 18:{OLED_ShowStr(0, 6,(uint8_t*)&"                 ^ ", 1);break;}
                        }
                        break;
                    }
                    }
                }
            }
            if(GPIO_Pin==KEY_4_PIN){
                rt_thread_mdelay(20);
                if(HAL_GPIO_ReadPin(KEY_N_GPIO,KEY_4_PIN)==KEY_4_DOWN_LEVEL){
                    if(menu_struct.oled_p.screen==SCREEN_IPSETMENU_HORI){
                        /* 保存设置 */
                        fatfs_struct.setting_fat.set_type = FATFS_SET_IP;
                        rt_sem_release(&fatfs_sem);
                        /* BACK */
                        menu_struct.setting_p.ip_add_bit=0;
                        menu_struct.oled_p.screen=SCREEN_MENU_IPSET;
                        menu_struct.menu_stat = MENU_STAT_FIRST_PAGE;
                        OLED_ShowCNX24(0,3,X24_Hanzi_IPSheZhi,4);
                    }
                }
                __HAL_GPIO_EXTI_CLEAR_IT(KEY_4_PIN);
            }
        }

        /* 编号菜单 */
        else if(menu_struct.menu_stat == MENU_STAT_SERIALNUM_PAGE){
            if(GPIO_Pin==KEY_1_PIN){
                rt_thread_mdelay(20);
                if(HAL_GPIO_ReadPin(KEY_N_GPIO,KEY_1_PIN)==KEY_1_DOWN_LEVEL){
                    switch(menu_struct.oled_p.screen){
                    case SCREEN_SERIALNUMMENU_HORI:{
                        if(menu_struct.setting_p.serialnum_bit<4){
                            menu_struct.setting_p.serialnum_bit++;
                        }
                        else{
                            menu_struct.setting_p.serialnum_bit=0;
                        }
                        switch(menu_struct.setting_p.serialnum_bit){
                        case 0:{OLED_ShowStr(1, 6,(uint8_t*)&" ^            ", 1);break;}
                        case 1:{OLED_ShowStr(1, 6,(uint8_t*)&"    ^         ", 1);break;}
                        case 2:{OLED_ShowStr(4, 6,(uint8_t*)&"      ^       ", 1);break;}
                        case 3:{OLED_ShowStr(2, 6,(uint8_t*)&"         ^    ", 1);break;}
                        case 4:{OLED_ShowStr(1, 6,(uint8_t*)&"            ^ ", 1);break;}
                        }
                        break;
                    }
                    case SCREEN_SERIALNUMMENU_VERT:{
                        if(menu_struct.setting_p.serialnum[menu_struct.setting_p.serialnum_bit]=='9'){
                            menu_struct.setting_p.serialnum[menu_struct.setting_p.serialnum_bit]='0';
                        }
                        else{
                            menu_struct.setting_p.serialnum[menu_struct.setting_p.serialnum_bit]++;
                        }
                        OLED_ShowSerialNumX32(menu_struct.setting_p.serialnum);
                        break;
                    }
                    }
                }
                __HAL_GPIO_EXTI_CLEAR_IT(KEY_1_PIN);
            }
            if(GPIO_Pin==KEY_2_PIN){
                rt_thread_mdelay(20);
                if(HAL_GPIO_ReadPin(KEY_N_GPIO,KEY_2_PIN)==KEY_2_DOWN_LEVEL){
                    switch(menu_struct.oled_p.screen){
                    case SCREEN_SERIALNUMMENU_HORI:{
                        if(menu_struct.setting_p.serialnum_bit==0){
                            menu_struct.setting_p.serialnum_bit=4;
                        }
                        else{
                            menu_struct.setting_p.serialnum_bit--;
                        }
                        switch(menu_struct.setting_p.serialnum_bit){
                        case 0:{OLED_ShowStr(1, 6,(uint8_t*)&" ^            ", 1);break;}
                        case 1:{OLED_ShowStr(1, 6,(uint8_t*)&"    ^         ", 1);break;}
                        case 2:{OLED_ShowStr(4, 6,(uint8_t*)&"      ^       ", 1);break;}
                        case 3:{OLED_ShowStr(2, 6,(uint8_t*)&"         ^    ", 1);break;}
                        case 4:{OLED_ShowStr(1, 6,(uint8_t*)&"            ^ ", 1);break;}
                        }
                        break;
                    }
                    case SCREEN_SERIALNUMMENU_VERT:{
                        if(menu_struct.setting_p.serialnum[menu_struct.setting_p.serialnum_bit]=='0'){
                            menu_struct.setting_p.serialnum[menu_struct.setting_p.serialnum_bit]='9';
                        }
                        else{
                            menu_struct.setting_p.serialnum[menu_struct.setting_p.serialnum_bit]--;
                        }
                        OLED_ShowSerialNumX32(menu_struct.setting_p.serialnum);
                        break;
                    }
                    }
                }
                __HAL_GPIO_EXTI_CLEAR_IT(KEY_1_PIN);
            }
            if(GPIO_Pin==KEY_3_PIN){
                rt_thread_mdelay(20);
                if(HAL_GPIO_ReadPin(KEY_N_GPIO,KEY_3_PIN)==KEY_3_DOWN_LEVEL){
                    switch(menu_struct.oled_p.screen){
                    case SCREEN_SERIALNUMMENU_HORI:{
                        menu_struct.oled_p.screen=SCREEN_SERIALNUMMENU_VERT;
                        switch(menu_struct.setting_p.serialnum_bit){
                        case 0:{OLED_ShowStr(1, 6,(uint8_t*)&" `            ", 1);break;}
                        case 1:{OLED_ShowStr(1, 6,(uint8_t*)&"    `         ", 1);break;}
                        case 2:{OLED_ShowStr(4, 6,(uint8_t*)&"      `       ", 1);break;}
                        case 3:{OLED_ShowStr(2, 6,(uint8_t*)&"         `    ", 1);break;}
                        case 4:{OLED_ShowStr(1, 6,(uint8_t*)&"            ` ", 1);break;}
                        }
                        break;
                    }
                    case SCREEN_SERIALNUMMENU_VERT:{
                        menu_struct.oled_p.screen=SCREEN_SERIALNUMMENU_HORI;
                        switch(menu_struct.setting_p.serialnum_bit){
                        case 0:{OLED_ShowStr(1, 6,(uint8_t*)&" ^            ", 1);break;}
                        case 1:{OLED_ShowStr(1, 6,(uint8_t*)&"    ^         ", 1);break;}
                        case 2:{OLED_ShowStr(4, 6,(uint8_t*)&"      ^       ", 1);break;}
                        case 3:{OLED_ShowStr(2, 6,(uint8_t*)&"         ^    ", 1);break;}
                        case 4:{OLED_ShowStr(1, 6,(uint8_t*)&"            ^ ", 1);break;}
                        }
                        break;
                    }
                    }
                }
                __HAL_GPIO_EXTI_CLEAR_IT(KEY_3_PIN);
            }
            if(GPIO_Pin==KEY_4_PIN){
                rt_thread_mdelay(20);
                if(HAL_GPIO_ReadPin(KEY_N_GPIO,KEY_4_PIN)==KEY_4_DOWN_LEVEL){
                    if(menu_struct.oled_p.screen==SCREEN_SERIALNUMMENU_HORI){
                        menu_struct.setting_p.serialnum_bit=0;
                        fatfs_struct.setting_fat.set_type = FATFS_SET_SERIALNUM;
                        rt_sem_release(&fatfs_sem);

                        /* back */
                        menu_struct.menu_stat = MENU_STAT_FIRST_PAGE;
                        menu_struct.oled_p.screen = SCREEN_MENU_SERIALNUM;
                        OLED_ShowCNX24(0,3,X24_Hanzi_BianHao,4);
                    }
                }
                __HAL_GPIO_EXTI_CLEAR_IT(KEY_4_PIN);
            }

        }
        /* NTP菜单 */
        else if(menu_struct.menu_stat == MENU_STAT_NTP_PAGE){
            if(GPIO_Pin==KEY_1_PIN){
                rt_thread_mdelay(20);
                if(HAL_GPIO_ReadPin(KEY_N_GPIO,KEY_1_PIN)==KEY_1_DOWN_LEVEL){
                    switch(menu_struct.oled_p.screen){
                    case SCREEN_NTPMENU_ONLINE:{
                        menu_struct.oled_p.screen = SCREEN_NTPMENU_HAND;
                        OLED_ShowStr(60, 0,(uint8_t*)&"[ _]    ", 1);
                        OLED_ShowCNX24(0,3,X24_Hanzi_ShouDongShiJian,4);
                        break;
                    }
                    case SCREEN_NTPMENU_HAND:{
                        menu_struct.oled_p.screen = SCREEN_NTPMENU_ONLINE;
                        OLED_ShowCNX24(0,3,X24_Hanzi_NTPShiJian,5);
                        OLED_ShowStr(60, 0,(uint8_t*)&"[_ ]    ", 1);
                        break;
                    }
                    }
                }
                __HAL_GPIO_EXTI_CLEAR_IT(KEY_1_PIN);
            }
            else if(GPIO_Pin==KEY_2_PIN){
                rt_thread_mdelay(20);
                if(HAL_GPIO_ReadPin(KEY_N_GPIO,KEY_2_PIN)==KEY_2_DOWN_LEVEL){
                    switch(menu_struct.oled_p.screen){
                    case SCREEN_NTPMENU_ONLINE:{
                        menu_struct.oled_p.screen = SCREEN_NTPMENU_HAND;
                        OLED_ShowStr(60, 0,(uint8_t*)&"[ _]    ", 1);
                        OLED_ShowCNX24(0,3,X24_Hanzi_ShouDongShiJian,4);
                        break;
                    }
                    case SCREEN_NTPMENU_HAND:{
                        menu_struct.oled_p.screen = SCREEN_NTPMENU_ONLINE;
                        OLED_ShowCNX24(0,3,X24_Hanzi_NTPShiJian,5);
                        OLED_ShowStr(60, 0,(uint8_t*)&"[_ ]    ", 1);
                        break;
                    }
                    }
                }
                __HAL_GPIO_EXTI_CLEAR_IT(KEY_2_PIN);
            }
            else if(GPIO_Pin==KEY_3_PIN){
                rt_thread_mdelay(20);
                if(HAL_GPIO_ReadPin(KEY_N_GPIO,KEY_3_PIN)==KEY_3_DOWN_LEVEL){
                    switch(menu_struct.oled_p.screen){
                    case SCREEN_NTPMENU_ONLINE:{
                        menu_struct.menu_stat = MENU_STAT_END;
                        work_struct.work_stat = WORK_STAT_GET_NTP_TIME;     //进入获取NTP工作
                        OLED_ShowCNX24(0,3,X24_Hanzi_HuoQuShiJian,5);
                        OLED_ShowStr(60, 0,(uint8_t*)&"LOADING ", 1);

                        /* NTP采集时间 */
                        ec600_struct.step=10;
                        rt_sem_release(&sem_struct.ec600_sendm_sem);
                        rt_timer_start(tim_struct.ec600_tim.ntpm_ot_tm);
                        break;
                    }
                    case SCREEN_NTPMENU_HAND:{
                        /* 手动设置当前时间 */
                        menu_struct.menu_stat = MENU_STAT_NTP_SETTIME_PAGE;
                        break;
                    }
                    }
                }
                __HAL_GPIO_EXTI_CLEAR_IT(KEY_3_PIN);
            }
            else if(GPIO_Pin==KEY_4_PIN){
                rt_thread_mdelay(20);
                if(HAL_GPIO_ReadPin(KEY_N_GPIO,KEY_4_PIN)==KEY_4_DOWN_LEVEL){
                    switch(menu_struct.oled_p.screen){
                    case SCREEN_NTPMENU_FAILED:{
                        menu_struct.menu_stat = MENU_STAT_FIRST_PAGE;
                        work_struct.work_stat = WORK_STAT_MENU;
                        menu_struct.oled_p.screen = SCREEN_MENU_CYBERWORK;
                        OLED_ShowCNX24(0,3,X24_Hanzi_LianWang,4);
                        OLED_ShowStr(60, 0,(uint8_t*)&"[_     ]", 1);
                        /* 正式开启EC600 */
                        EC600_PWK_HIGH();

                        break;
                    }

                    case SCREEN_NTPMENU_ONLINE:
                    case SCREEN_NTPMENU_HAND:{
                        /* 回首页 */
                        menu_struct.menu_stat = MENU_STAT_FIRST_PAGE;
                        work_struct.work_stat = WORK_STAT_MENU;

                        menu_struct.oled_p.screen = SCREEN_MENU_CYBERWORK;
                        OLED_ShowCNX24(0,3,X24_Hanzi_LianWang,4);
                        OLED_ShowStr(60, 0,(uint8_t*)&"[_     ]", 1);
                        break;
                    }
                    }
                }
                __HAL_GPIO_EXTI_CLEAR_IT(KEY_4_PIN);
            }
        }
    }

    /* U盘工作 */
    else if(work_struct.work_stat == WORK_STAT_USB){
        if(GPIO_Pin==KEY_1_PIN){
            rt_thread_mdelay(20);
            if(HAL_GPIO_ReadPin(KEY_N_GPIO,KEY_1_PIN)==KEY_1_DOWN_LEVEL){
                switch(menu_struct.oled_p.screen){
                case SCREEN_USB_A:{
                    menu_struct.oled_p.screen=SCREEN_USB_POWEROFF;
                    for(int i=0;i<5;i++){   //清理一行
                        ShowCNX24(i*24,3,i,X24_Line_Clear);
                    }
                    for(int i=0;i<5;i++){   //清理一行
                        ShowCNX24(i*24,3+2,i,X24_Line_Clear);
                    }
                    OLED_ShowCNX16(40,3,X16_Hanzi_GuanJi,2);
                    key_on_stat = KEY_ON_STAT_ENABLE;
                    break;
                }
                case SCREEN_USB_POWEROFF:{
                    menu_struct.oled_p.screen=SCREEN_USB_A;
                    for(int i=0;i<5;i++){   //清理一行
                        ShowCNX24(i*24,3,i,X24_Line_Clear);
                    }
                    for(int i=0;i<5;i++){   //清理一行
                        ShowCNX24(i*24,3+2,i,X24_Line_Clear);
                    }
                    OLED_ShowCNX16(10,3,X16_Hanzi_UPanShiYongZhong,5);
                    OLED_ShowStr(90, 4,(uint8_t*)&"...", 1);
                    key_on_stat = KEY_ON_STAT_DISABLE;
                    break;
                }
                }
            }
            __HAL_GPIO_EXTI_CLEAR_IT(KEY_1_PIN);
        }
        if(GPIO_Pin==KEY_2_PIN){
            rt_thread_mdelay(20);
            if(HAL_GPIO_ReadPin(KEY_N_GPIO,KEY_2_PIN)==KEY_2_DOWN_LEVEL){
                switch(menu_struct.oled_p.screen){
                case SCREEN_USB_A:{
                    menu_struct.oled_p.screen=SCREEN_USB_POWEROFF;
                    for(int i=0;i<5;i++){   //清理一行
                        ShowCNX24(i*24,3,i,X24_Line_Clear);
                    }
                    for(int i=0;i<5;i++){   //清理一行
                        ShowCNX24(i*24,3+2,i,X24_Line_Clear);
                    }
                    OLED_ShowCNX16(40,3,X16_Hanzi_GuanJi,2);
                    key_on_stat = KEY_ON_STAT_ENABLE;
                    break;
                }
                case SCREEN_USB_POWEROFF:{
                    menu_struct.oled_p.screen=SCREEN_USB_A;
                    OLED_ShowStr(35, 0,(uint8_t*)&"USB", 1);
                    for(int i=0;i<5;i++){   //清理一行
                        ShowCNX24(i*24,3,i,X24_Line_Clear);
                    }
                    for(int i=0;i<5;i++){   //清理一行
                        ShowCNX24(i*24,3+2,i,X24_Line_Clear);
                    }
                    OLED_ShowCNX16(10,3,X16_Hanzi_UPanShiYongZhong,5);
                    OLED_ShowStr(90, 4,(uint8_t*)&"...", 1);
                    key_on_stat = KEY_ON_STAT_DISABLE;
                    break;
                }
                }
            }
            __HAL_GPIO_EXTI_CLEAR_IT(KEY_2_PIN);
        }
    }

    /* 联网工作  */
    else if(work_struct.work_stat == WORK_STAT_ONLINE_WORK){
        if(GPIO_Pin==KEY_1_PIN){
            rt_thread_mdelay(20);
            if(HAL_GPIO_ReadPin(KEY_N_GPIO,KEY_1_PIN)==KEY_1_DOWN_LEVEL){
                if(online_send_stat == ONLINE_SEND_READY){
                    online_send_stat = ONLINE_SEND_WORKING;
                    OLED_ShowStr(60, 0,(uint8_t*)&"WORKING.", 1);

                    /* 发送信息总数清空 */
                    ec600_struct.send_count=0;
                    ec600_struct.send_message_stat=SEND_MESSAGE_STAT_INIT;
                    delete_failed_txt=0;
                    ec600_struct.send_error_fatfs_lsk=0;
                    ec600_struct.send_error_count=0;
                    /* 正式开始联网发送信息 */
                    /* FATFS记录文件 */
                    fatfs_struct.setting_fat.set_type=FATFS_SET_NONE;
                    fatfs_struct.lsk=0;
                    uint8_t *p,*d ;
                    p=fatfs_struct.path;
                    d=(uint8_t*)&"/data/";
                    for(int i=0;i<6;i++){
                        *p=*d;
                        p++;d++;
                    }
                    time_struct.now_timestamp = time(RT_NULL);
                    time_struct.localtime = *localtime(&time_struct.now_timestamp);
                    *p= 0x30| ((time_struct.localtime.tm_year+1900)/10%10) ; p++;
                    *p=0x30| ((time_struct.localtime.tm_year+1900)/1%10);p++;
                    *p='-';p++;
                    *p= 0x30|( (time_struct.localtime.tm_mon+1)/10%10 );p++;
                    *p=0x30|( (time_struct.localtime.tm_mon+1)/1%10 );p++;
                    *p='-';p++;
                    *p= 0x30|( time_struct.localtime.tm_mday/10%10 );p++;
                    *p=0x30|( time_struct.localtime.tm_mday/1%10 );p++;
                    *p='_';p++;
                    *p= 0x30|( time_struct.localtime.tm_hour/10%10 );p++;
                    *p=0x30|(time_struct.localtime.tm_hour/1%10);p++;
                    *p='-';p++;
                    *p= 0x30|(time_struct.localtime.tm_min/10%10);p++;
                    *p=0x30|(time_struct.localtime.tm_min/1%10);p++;
                    *p='-';p++;
                    *p= 0x30|(time_struct.localtime.tm_sec/10%10);p++;
                    *p=0x30|(time_struct.localtime.tm_sec/1%10);p++;

                    d=(uint8_t*)&".txt";
                    for(int i=0;i<4;i++){
                        *p=*d;
                        p++;d++;
                    }

//                    /* 写入发送失败的温度信息的txt文件名 */
//                    p=fatfs_struct.errpath;
//                    d=(uint8_t*)&"/eror/";
//                    for(int i=0;i<6;i++){
//                        *p=*d;
//                        p++;d++;
//                    }
//                    *p= 0x30| ((time_struct.localtime.tm_year+1900)/10%10) ; p++;
//                    *p=0x30| ((time_struct.localtime.tm_year+1900)/1%10);p++;
//                    *p='-';p++;
//                    *p= 0x30|( (time_struct.localtime.tm_mon+1)/10%10 );p++;
//                    *p=0x30|( (time_struct.localtime.tm_mon+1)/1%10 );p++;
//                    *p='-';p++;
//                    *p= 0x30|( time_struct.localtime.tm_mday/10%10 );p++;
//                    *p=0x30|( time_struct.localtime.tm_mday/1%10 );p++;
//                    *p='_';p++;
//                    *p= 0x30|( time_struct.localtime.tm_hour/10%10 );p++;
//                    *p=0x30|(time_struct.localtime.tm_hour/1%10);p++;
//                    *p='-';p++;
//                    *p= 0x30|(time_struct.localtime.tm_min/10%10);p++;
//                    *p=0x30|(time_struct.localtime.tm_min/1%10);p++;
//                    *p='-';p++;
//                    *p= 0x30|(time_struct.localtime.tm_sec/10%10);p++;
//                    *p=0x30|(time_struct.localtime.tm_sec/1%10);p++;
//
//                    d=(uint8_t*)&"E.txt";
//                    for(int i=0;i<5;i++){
//                        *p=*d;
//                        p++;d++;
//                    }

                    /* 开始发送 */
                    rt_sem_release(&sem_struct.ec600_sendm_sem);
                    rt_timer_start(tim_struct.ec600_tim.sendm_ot_tm);

                }
            }
            __HAL_GPIO_EXTI_CLEAR_IT(KEY_1_PIN);
        }

        else if(GPIO_Pin==KEY_4_PIN){   //退出联网工作，关机
            rt_thread_mdelay(20);
            if(HAL_GPIO_ReadPin(KEY_N_GPIO,KEY_4_PIN)==KEY_4_DOWN_LEVEL){
                OLED_ShowStr(60, 0,(uint8_t*)&"QUIT.   ", 1);
                key_on_stat = KEY_ON_STAT_ENABLE;
//                if(online_end_count<3){
//                    online_end_count++;
//                }
//                else{
//                    online_end_count = 10;  //进入最后一次发送
//                }
            }
            __HAL_GPIO_EXTI_CLEAR_IT(KEY_4_PIN);
        }

    }

}

/*
 *
 * 填写发送的温度信息
 */
void Message_Write(void)
{
    uint8_t*p, *d;
    p=ec600_struct.message;
    d=(uint8_t*)&"SN";
    for(int i=0;i<2;i++){
        *p=*d;
        p++;
        d++;
    }
    d=menu_struct.setting_p.serialnum;
    for(int i=0;i<5;i++){
        *p=*d;
        p++;
        d++;
    }
    *p='T';
    p++;
    if(temp_struct.temp_sign==0){*p='+';}
    else{*p='-';}
    p++;
    *p=  0x30|(uint8_t)((temp_struct.u32_temp/100)%10);
    p++;
    *p=  0x30|(uint8_t)((temp_struct.u32_temp/10)%10);
    p++;
    *p='.';
    p++;
    *p=  0x30|(uint8_t)((temp_struct.u32_temp/1)%10);
    p++;
    *p='S';
    p++;
    if(temp_struct.u32_temp<22){*p='L';}
    else if(temp_struct.u32_temp>75){*p='H';}
    else{*p='U';}
    p++;
    *p='t';
    p++;
    /* 获取时间 */
    time_struct.now_timestamp = time(RT_NULL);
        /* 校准测试：每12小时减15秒 */
        if((ec600_struct.send_count%144==1)&&(ec600_struct.send_count>50)){
            time_struct.now_timestamp=time_struct.now_timestamp-15;
            time_struct.localtime = *localtime(&time_struct.now_timestamp);
            set_date( (2000+((time_struct.localtime.tm_year+1900)/10%10)*10 + (time_struct.localtime.tm_year+1900)/1%10) ,
                    time_struct.localtime.tm_mon+1, time_struct.localtime.tm_mday);
            set_time(time_struct.localtime.tm_hour , time_struct.localtime.tm_min , time_struct.localtime.tm_sec);
        }
    time_struct.localtime = *localtime(&time_struct.now_timestamp);
    *p= 0x30| ((time_struct.localtime.tm_year+1900)/10%10) ;
    p++;
    *p=0x30| ((time_struct.localtime.tm_year+1900)/1%10);
    p++;
    *p= 0x30|( (time_struct.localtime.tm_mon+1)/10%10 );
    p++;
    *p=0x30|( (time_struct.localtime.tm_mon+1)/1%10 );
    p++;
    *p= 0x30|( time_struct.localtime.tm_mday/10%10 );
    p++;
    *p=0x30|( time_struct.localtime.tm_mday/1%10 );
    p++;

    *p= 0x30|( time_struct.localtime.tm_hour/10%10 );
    p++;
    *p=0x30|(time_struct.localtime.tm_hour/1%10);
    p++;
    *p= 0x30|(time_struct.localtime.tm_min/10%10);
    p++;
    *p=0x30|(time_struct.localtime.tm_min/1%10);
    p++;
    *p= 0x30|(time_struct.localtime.tm_sec/10%10);
    p++;
    *p=0x30|(time_struct.localtime.tm_sec/1%10);
    p++;
    *p='\r';
    p++;
    *p='\n';
}

/*
  * 根据ADC获取到的电池电量，改变电池电量OLED显示
 */
void Battery_Display(uint32_t vol)
{
    if(vol>=97){   //3.9V
        OLED_ShowStr(1,0,(uint8_t*)"[III]",1);
        return;
    }
    else if((vol>=92)&&(vol<97)){  //3.7V-3.9V
        OLED_ShowStr(1,0,(uint8_t*)"[II ]",1);
        return;
    }
    else if((vol>=88)&&(vol<92)){  //3.5V-3.7V
        OLED_ShowStr(1,0,(uint8_t*)"[I  ]",1);
        return;
    }
    else if(vol<88){
        OLED_ShowStr(1,0,(uint8_t*)"[   ]",1);
        return;
    }
}
