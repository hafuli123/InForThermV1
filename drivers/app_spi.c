/*
 * Copyright (c) 2006-2021, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2022-09-21     aocekeji5       the first version
 */
#include <rtthread.h>
#include <board.h>
#include <rtdevice.h>
#include "drv_spi.h"
#include "app_spi.h"
#include "spi_flash_sfud.h"
#include <dfs_fs.h>
#include <dfs_file.h>
#include <finsh.h>

extern struct rt_semaphore fatfs_sem;

int rt_hw_spi_flash_init(void)
{
    __HAL_RCC_GPIOA_CLK_ENABLE();
    rt_hw_spi_device_attach("spi1", "spi10", GPIOA, GPIO_PIN_4);// spi10 表示挂载在 spi3 总线上的 0 号设备,PA4是片选，这一步就可以将从设备挂在到总线中。

    if (RT_NULL == rt_sfud_flash_probe("W25Q128", "spi10"))  //注册块设备，这一步可以将外部flash抽象为系统的块设备
    {
        return -RT_ERROR;
    };

    return RT_EOK;
}
/* 导出到自动初始化 */
INIT_COMPONENT_EXPORT(rt_hw_spi_flash_init);

int mnt_init(void)
{
//    mkfs("elm","W25Q128");//第一次挂载前需格式化，因此要烧录两次
    if(dfs_mount("W25Q128","/","elm",0,0)==0) //挂载文件系统，参数：块设备名称、挂载目录、文件系统类型、读写标志、私有数据0
    {
        rt_kprintf("dfs mount success\r\n");
    }
    else
    {
        rt_kprintf("dfs mount failed\r\n");
    }
}
INIT_ENV_EXPORT(mnt_init);

int mount_to_usb(void)
{
   char *fullpath = NULL;

   fullpath = dfs_normalize_path(NULL, "/");
   if (dfs_unmount(fullpath) == 0)
   {
       rt_kprintf("mount_to_usb ok!\n");
   }
   else
   {
       rt_kprintf("mount_to_usb fail!\n");
   }
}
//INIT_APP_EXPORT(mount_to_usb);

void w25q128_unmount(void)
{
    if(rt_device_find("W25Q128")!=RT_NULL){
        if(dfs_unmount("/")==0){
            rt_kprintf("mount_to_usb ok!\n");
        }
        else{
            rt_kprintf("mount_to_usb fail!\n");
        }
    }
}
