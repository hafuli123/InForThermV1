#ifndef __BSP_FATFS_USB_H__
#define __BSP_FATFS_USB_H__

#include "stm32f4xx_hal.h"
//#include "ff.h"
//#include "ff_gen_drv.h"



#define FATFS_SUCCESS  0
#define FATFS_ERROR  1

uint8_t filesystem_init(void);

#endif
