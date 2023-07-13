#include "time.h"
#include <rtthread.h>
#include "board.h"
#include <rtdevice.h>
#include "bsp_rtc.h"
#include "other_func.h"

/*
 * 从NTP获取时间的ASC码信息，通过时间戳转化为最终时区正确的时间，送入RTC时钟并完成RTC时钟配置
 */
void ntp_Get_RTCLocalTime(uint8_t *bcdval)
{
    struct tm ttime;
    time_t ttimestamp;
    ttime.tm_year = (2000+(0x0f&bcdval[0])*10+(0x0f&bcdval[1]))-1900;
    ttime.tm_mon = (0x0f&bcdval[2])*10+(0x0f&bcdval[3])-1;
    ttime.tm_mday = (0x0f&bcdval[4])*10+(0x0f&bcdval[5]);
    ttime.tm_hour = (0x0f&bcdval[6])*10+(0x0f&bcdval[7]);
    ttime.tm_min = (0x0f&bcdval[8])*10+(0x0f&bcdval[9]);
    ttime.tm_sec = (0x0f&bcdval[10])*10+(0x0f&bcdval[11]);

    ttimestamp = mktime(&ttime)+3600*8; //消除时差
    ttime = *localtime(&ttimestamp);

    /* 给RTC设入时间 */
#ifndef USING_BSP_RTC
    set_date( (2000+((ttime.tm_year+1900)/10%10)*10 + (ttime.tm_year+1900)/1%10) , ttime.tm_mon+1, ttime.tm_mday);
    set_time(ttime.tm_hour , ttime.tm_min , ttime.tm_sec);
    return;
#endif
    RTC_CalendarConfig( ((ttime.tm_year+1900)/10%10)*10+ (ttime.tm_year+1900)/1%10,
                        ttime.tm_mon+1,
                        ttime.tm_mday,
                        ttime.tm_hour,
                        ttime.tm_min,
                        ttime.tm_sec);
    MX_RTC_Init();
    return;
}

