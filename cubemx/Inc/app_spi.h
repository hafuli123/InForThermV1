/*
 * Copyright (c) 2006-2021, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2022-09-27     aocekeji5       the first version
 */
#ifndef CUBEMX_INC_APP_SPI_H_
#define CUBEMX_INC_APP_SPI_H_


int rt_hw_spi_flash_init(void);
int mnt_init(void);
int mount_to_usb(void);
void w25q128_unmount(void);

#endif /* CUBEMX_INC_APP_SPI_H_ */
