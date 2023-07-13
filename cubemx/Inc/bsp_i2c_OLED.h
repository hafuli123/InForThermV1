#ifndef __I2C_OLED_H__
#define	__I2C_OLED_H__

#include "stm32f4xx_hal.h"

#define OLED_DEV_ADDR			   0x78

extern uint8_t X16_Hanzi_AoCeKeJi[],X16_Hanzi_UPanShiYongZhong[],X16_Hanzi_GuanJi[];

extern uint8_t X32_NUM_0[],X32_NUM_1[],X32_NUM_2[],X32_NUM_3[],X32_NUM_4[],X32_NUM_5[],X32_NUM_6[],X32_NUM_7[],X32_NUM_8[],X32_NUM_9[];

extern uint8_t X32_Hanzi_BianHao[] ;

extern uint8_t X24_Line_Clear[],X24_Hanzi_BianHao[] , X24_Hanzi_LiXian[] , X24_Hanzi_LianWang[],X24_Hanzi_IPSheZhi[],X24_Hanzi_NTPSheZhi[],X24_Hanzi_UPan[],X24_Hanzi_WenDuJi[],
            X24_Hanzi_HuoQuShiJian[],X24_Hanzi_GuanJi[],X24_Hanzi_NTPShiJian[],X24_Hanzi_ShouDongShiJian[],X24_Hanzi_NTPShiBai[];
extern uint8_t X24_NUM_0[],X24_NUM_1[],X24_NUM_2[],X24_NUM_3[],X24_NUM_4[],X24_NUM_5[],X24_NUM_6[],X24_NUM_7[],X24_NUM_8[],X24_NUM_9[];

void OLED_Init(void);
void OLED_SetPos(unsigned char x, unsigned char y);
void OLED_Fill(unsigned char fill_Data);
void OLED_CLS(void);
void OLED_ON(void);
void OLED_OFF(void);
void OLED_ShowStr(unsigned char x, unsigned char y, unsigned char ch[], unsigned char TextSize);
void OLED_ShowCN(unsigned char x, unsigned char y, unsigned char N,unsigned char * words) ;
void OLED_DrawBMP(unsigned char x0,unsigned char y0,unsigned char x1,unsigned char y1,unsigned char BMP[]);
void OLED_Clear(void);

void OLED_ShowAdd(uint8_t x,uint8_t y,uint8_t *add,uint8_t page);

void ShowCNX32(unsigned char x, unsigned char y, unsigned char N,unsigned char * words)  ;
void ShowCNX24(unsigned char x, unsigned char y, unsigned char N,unsigned char * words);
void OLED_ShowCNX24(uint8_t x,uint8_t y,uint8_t*words,uint8_t wordnum);
void OLED_ShowCNX16(uint8_t x,uint8_t y,uint8_t*words,uint8_t wordnum);

void ShowNumX32(unsigned char x, unsigned char y, unsigned char N,unsigned char * words) ;

void OLED_ShowSerialNumX32(uint8_t* num);

void OLED_ShowTempX32(uint32_t val , uint8_t sign);

void OLED_ShowTime(void);
#endif /* __I2C_OLED_H__ */

