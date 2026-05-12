#ifndef __RTC_INIT_H__
#define __RTC_INIT_H__

#ifdef __cplusplus
extern "C" {
#endif

#include "main.h"
#include <stdint.h>

void RTC_Init_All(void);
void RTC_User_GetTime(uint8_t *hour, uint8_t *minute, uint8_t *second);
void RTC_User_SetTime(uint8_t hour, uint8_t minute, uint8_t second);
void RTC_User_GetDate(uint16_t *year, uint8_t *month, uint8_t *day);
void RTC_User_SetDate(uint16_t year, uint8_t month, uint8_t day, uint8_t weekday);
uint8_t RTC_User_GetWeekDay(void);  /* 获取星期 (1=周一，2=周二，...，7=周日) */

/* 电机定时控制函数 */
uint8_t RTC_Motor_SetTimePoint(uint8_t weekday, uint8_t time_index, uint8_t hour, uint8_t minute);
/* 设置时间点：weekday=1-7(周一到周日), time_index=0-4(最多5个时间点), hour=0-23, minute=0-59 */

uint8_t RTC_Motor_ClearDaySchedule(uint8_t weekday);
/* 清除指定日期的所有时间点：weekday=1-7(周一到周日) */

void RTC_Motor_TimerControl(void);
/* 电机定时控制（需要在主循环中定期调用） */

void RTC_Update(void);
/* 更新RTC时间 */

/* Flash存储相关函数 */
void RTC_Motor_GetSchedule(uint8_t weekday, uint8_t *count, uint8_t *hours, uint8_t *minutes);
/* 获取指定日期的喂食计划：weekday=1-7, count返回时间点数量, hours和minutes数组至少5个元素 */

void RTC_Motor_SetSchedule(uint8_t weekday, uint8_t count, const uint8_t *hours, const uint8_t *minutes);
/* 设置指定日期的喂食计划：weekday=1-7, count为时间点数量, hours和minutes数组 */

uint8_t RTC_Motor_AddTimePoint(uint8_t weekday, uint8_t hour, uint8_t minute);
/* 添加时间点（自动查找可用索引）：weekday=1-7, hour=0-23, minute=0-59
 * 返回0-成功，1-参数错误，2-时间点已存在，3-已达到最大数量(5个)
 */

uint8_t RTC_Motor_RemoveTimePoint(uint8_t weekday, uint8_t hour, uint8_t minute);
/* 删除指定时间点：weekday=1-7, hour=0-23, minute=0-59
 * 返回0-成功，1-参数错误，2-时间点不存在
 */

void RTC_IRQHandler(void);
/* RTC中断处理函数（在中断文件中调用） */

void update_next_alarm(void);
/* 更新下一个闹钟时间 */

#ifdef __cplusplus
}
#endif

#endif /* __RTC_INIT_H__ */
