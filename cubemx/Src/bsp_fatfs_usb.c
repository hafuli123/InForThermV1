#include "bsp_fatfs_usb.h"
#include "bsp_i2c_OLED.h"
//FATFS
//static char SPIFLASHPath[4];             /* 串行Flash逻辑设备路径 */
//static FATFS fs;                                                    /* FatFs文件系统对象 */
//static FIL file;                                                    /* 文件对象 */
//static FRESULT f_res;                    /* 文件操作结果 */
//static UINT fnum;                                 /* 文件成功读写数量 */
//static BYTE ReadBuffer[1024]={0};        /* 读缓冲区 */

uint8_t filesystem_init(void)
{

//    if(FATFS_LinkDriver(&SPIFLASH_Driver, SPIFLASHPath) == 0){
//    /* 在串行FLASH挂载文件系统，文件系统挂载时会对串行FLASH初始化 */
//
//        f_res = f_mount(&fs,(TCHAR const*)SPIFLASHPath,1);
//
//    /*----------------------- 格式化测试 ---------------------------*/
//    /* 如果没有文件系统就格式化创建创建文件系统 */
//        if(f_res == FR_NO_FILESYSTEM){
//
//          /* 格式化 */
//          f_res=f_mkfs((TCHAR const*)SPIFLASHPath,0,0);
//          OLED_ShowStr(1,4,(uint8_t*)"159",2);
//          if(f_res == FR_OK){
//    //        printf("》串行FLASH已成功格式化文件系统。\n");
//            /* 格式化后，先取消挂载 */
//            f_res = f_mount(NULL,(TCHAR const*)SPIFLASHPath,1);
//            f_res = f_mount(&fs,(TCHAR const*)SPIFLASHPath,1);
//          }
//          else{
//                    return FATFS_ERROR;
//          }
//        }
//        else if(f_res!=FR_OK){
//                return FATFS_ERROR;
//        }
//        else{
//                return FATFS_SUCCESS;
//        }
//    }
    return FATFS_ERROR;
}
