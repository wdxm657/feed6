#include "rtc_init.h"
#include "ft32f0xx.h"
#include "ft32f0xx_rcc.h"
#include "ft32f0xx_rtc.h"
#include "ft32f0xx_pwr.h"
#include "ft32f0xx_exti.h"
#include "ft32f0xx_misc.h"
#include "motor_ctrl.h"
#include "log.h"
#include "battery_monitor.h"

/* RTC时钟源配置说明：
 * 当前使用HSE（外部8MHz晶振）作为RTC时钟源
 * HSE通过硬件自动除以32后提供给RTC，即RTC时钟 = 8MHz / 32 = 250kHz
 *
 * 预分频器计算公式：RTC时钟频率 = (ASYNC_PRESCALER+1) * (SYNC_PRESCALER+1) * 1Hz
 *
 * 对于250kHz的RTC时钟：
 * 250000 = (ASYNC_PRESCALER+1) * (SYNC_PRESCALER+1)
 * 可以设置为：ASYNC=99, SYNC=2499，总计 = 100 * 2500 = 250000
 *
 * 如果HSE频率有偏差，可以根据实际测试结果调整预分频器
 */
#define RTC_ASYNC_PRESCALER 99  /* 异步预分频器：99，固定值 */
#define RTC_SYNC_PRESCALER 2499 /* 同步预分频器：2499，总计 (99+1)*(2499+1) = 250000，对应HSE/32=250kHz */

/* 电机定时控制相关 */
#define MAX_TIME_POINTS_PER_DAY 5   /* 每天最多5个时间点 */
#define MOTOR_RUN_DURATION_MS 6000U /* 电机运行时长：6秒 */

/* 时间点结构体 */
typedef struct
{
    uint8_t hour;   /* 小时 (0-23) */
    uint8_t minute; /* 分钟 (0-59) */
} TimePoint_t;

/* 每天的时间点配置 */
typedef struct
{
    TimePoint_t time_points[MAX_TIME_POINTS_PER_DAY]; /* 时间点数组 */
    uint8_t count;                                    /* 有效时间点数量 */
} DaySchedule_t;

/* 周一到周日的时间表 */
static DaySchedule_t week_schedule[7]; /* 索引0=周一，1=周二，...，6=周日 */

/* 触发标志：记录每个时间点今天是否已触发 */
static uint8_t triggered_flags[7][MAX_TIME_POINTS_PER_DAY];
static uint8_t last_date = 0xFF; /* 记录上次检查的日期，用于检测日期变化 */

static uint8_t s_rtc_irq_flag = 0;

/* RTC闹钟中断相关函数声明 */
static void RTC_Alarm_ConfigNVIC(void);
static void RTC_Alarm_ConfigEXTI(void);
static void RTC_Alarm_UpdateNextAlarm(void);

/**
 * @brief  RTC初始化
 * @param  None
 * @retval None
 */
void RTC_Init_All(void)
{
    RTC_InitTypeDef RTC_InitStruct;
    RTC_TimeTypeDef RTC_TimeStruct;
    RTC_DateTypeDef RTC_DateStruct;

    /* 使能PWR时钟 */
    RCC_APB1PeriphClockCmd(RCC_APB1Periph_PWR, ENABLE);

    /* 允许访问备份域 */
    PWR_BackupAccessCmd(ENABLE);

    /* 检查HSE是否已经就绪（系统时钟初始化时应该已经使能了HSE） */
    if (RCC_GetFlagStatus(RCC_FLAG_HSERDY) == RESET)
    {
        /* 如果HSE未就绪，尝试使能HSE */
        RCC_HSEConfig(RCC_HSE_ON);

        /* 等待HSE就绪，添加超时保护 */
        uint32_t timeout = HSE_STARTUP_TIMEOUT;
        while ((RCC_GetFlagStatus(RCC_FLAG_HSERDY) == RESET) && (timeout > 0))
        {
            timeout--;
        }

        /* 如果HSE启动失败，返回（可能导致RTC无法工作，但不应该阻止系统运行） */
        if (RCC_GetFlagStatus(RCC_FLAG_HSERDY) == RESET)
        {
            LOG_DEBUG("RTC_Init_All: HSE not ready!");
            return;
        }
    }

    /* 如果RTC已经使能，先禁用以重新配置 */
    if ((RCC->BDCR & RCC_BDCR_RTCEN) != RESET)
    {
        RCC_RTCCLKCmd(DISABLE);
        /* 等待RTC时钟禁用 */
        for (volatile uint32_t i = 0; i < 10000; i++)
            ;
    }

    /* 复位备份域（清除之前的RTC配置） */
    RCC_BackupResetCmd(ENABLE);
    RCC_BackupResetCmd(DISABLE);

    /* 等待备份域复位完成 */
    for (volatile uint32_t i = 0; i < 1000; i++)
        ;

    /* 选择HSE/32作为RTC时钟源（8MHz / 32 = 250kHz） */
    /* 注意：必须在使能RTC时钟之前配置时钟源 */
    RCC_RTCCLKConfig(RCC_RTCCLKSource_HSE_Div32);

    /* 使能RTC时钟 */
    RCC_RTCCLKCmd(ENABLE);

    /* 等待RTC时钟稳定（给时钟一些时间稳定，HSE/32需要时间） */
    /* HSE/32 = 250kHz，需要足够的时间让时钟稳定 */
    for (volatile uint32_t i = 0; i < 100000; i++)
        ;

    /* 复位RTC */
    /* 注意：RTC_DeInit内部会调用RTC_EnterInitMode，需要RTC时钟源工作才能进入初始化模式 */
    /* 如果RTC时钟源没有正确配置，RTC_EnterInitMode会超时失败 */
    /* RTC_EnterInitMode需要等待INITF标志置位，这需要RTC时钟源工作 */
    if (RTC_DeInit() == ERROR)
    {
        LOG_DEBUG("RTC_Init_All: RTC_DeInit failed! RTC clock source (HSE/32) may not be working.");
        return;
    }

    /* 配置RTC写保护 */
    RTC_WriteProtectionCmd(DISABLE);

    /* 配置日历值源（不使用旁路阴影寄存器） */
    RTC_BypassShadowCmd(DISABLE);

    /* 等待RTC寄存器同步 */
    /* 注意：RTC_DeInit内部已经调用了RTC_WaitForSynchro，但为了确保同步，这里再次等待 */
    /* 注意：RTC_WaitForSynchro内部有超时保护，如果超时会返回ERROR */
    if (RTC_WaitForSynchro() == ERROR)
    {
        /* RTC同步失败，可能是时钟源配置有问题 */
        LOG_DEBUG("RTC_Init_All: RTC_WaitForSynchro failed! RTC clock source may not be working.");
        /* 为了不影响系统运行，这里直接返回，RTC可能无法正常工作 */
        return;
    }

    /* 初始化RTC结构体 */
    RTC_StructInit(&RTC_InitStruct);
    RTC_InitStruct.RTC_HourFormat = RTC_HourFormat_24;
    RTC_InitStruct.RTC_AsynchPrediv = RTC_ASYNC_PRESCALER;
    RTC_InitStruct.RTC_SynchPrediv = RTC_SYNC_PRESCALER;

    /* 初始化RTC */
    if (RTC_Init(&RTC_InitStruct) == ERROR)
    {
        LOG_DEBUG("RTC_Init_All: RTC_Init failed!");
        return;
    }

    /* 设置初始时间：00:00:00 */
    RTC_TimeStructInit(&RTC_TimeStruct);
    RTC_TimeStruct.RTC_H12 = RTC_H12_AM;
    RTC_TimeStruct.RTC_Hours = 0;
    RTC_TimeStruct.RTC_Minutes = 0;
    RTC_TimeStruct.RTC_Seconds = 0;

    uint32_t set_time_timeout = 0xFFFFU;
    while ((RTC_SetTime(RTC_Format_BIN, &RTC_TimeStruct) != SUCCESS) && (set_time_timeout > 0))
    {
        set_time_timeout--;
    }

    if (set_time_timeout == 0)
    {
        LOG_DEBUG("RTC_Init_All: RTC_SetTime timeout!");
        return;
    }

    /* 设置初始日期：2024-01-01 周一 */
    RTC_DateStructInit(&RTC_DateStruct);
    RTC_DateStruct.RTC_Year = 24; /* 2024年 */
    RTC_DateStruct.RTC_Month = 1;
    RTC_DateStruct.RTC_Date = 1;
    RTC_DateStruct.RTC_WeekDay = 1; /* 周一 */

    uint32_t set_date_timeout = 0xFFFFU;
    while ((RTC_SetDate(RTC_Format_BIN, &RTC_DateStruct) != SUCCESS) && (set_date_timeout > 0))
    {
        set_date_timeout--;
    }

    if (set_date_timeout == 0)
    {
        LOG_DEBUG("RTC_Init_All: RTC_SetDate timeout!");
        return;
    }

    /* 使能RTC写保护 */
    RTC_WriteProtectionCmd(ENABLE);

    /* 初始化时间表（全部清零） */
    for (uint8_t day = 0; day < 7; day++)
    {
        week_schedule[day].count = 0;
        for (uint8_t i = 0; i < MAX_TIME_POINTS_PER_DAY; i++)
        {
            triggered_flags[day][i] = 0;
        }
    }

    /* 配置RTC闹钟中断 */
    RTC_Alarm_ConfigNVIC();
    RTC_Alarm_ConfigEXTI();

    /* 清除RTC闹钟中断标志 */
    RTC_ClearITPendingBit(RTC_IT_ALRA);

    /* 使能RTC闹钟中断 */
    RTC_ITConfig(RTC_IT_ALRA, ENABLE);
}

/**
 * @brief  获取当前时间（时:分:秒）
 * @param  hour:   小时指针
 * @param  minute: 分钟指针
 * @param  second: 秒指针
 */
void RTC_User_GetTime(uint8_t *hour, uint8_t *minute, uint8_t *second)
{
    RTC_TimeTypeDef RTC_TimeStruct;

    /* 注意：RTC_GetTime直接读取寄存器，不需要等待同步
     * 但如果RTC没有运行，读取的值可能不会变化
     */
    RTC_GetTime(RTC_Format_BIN, &RTC_TimeStruct);

    *hour = RTC_TimeStruct.RTC_Hours;
    *minute = RTC_TimeStruct.RTC_Minutes;
    *second = RTC_TimeStruct.RTC_Seconds;
}

/**
 * @brief  设置当前时间（时:分:秒）
 * @param  hour:   小时 (0-23)
 * @param  minute: 分钟 (0-59)
 * @param  second: 秒 (0-59)
 *
 * 注意：参考例程实现方式，RTC_SetTime内部会自动调用RTC_WaitForSynchro()
 */
void RTC_User_SetTime(uint8_t hour, uint8_t minute, uint8_t second)
{
    RTC_TimeTypeDef RTC_TimeStruct;
    ErrorStatus status = ERROR;
    uint32_t timeout = 0U;

    /* 参数检查 */
    if (hour >= 24 || minute >= 60 || second >= 60)
    {
        return; /* 参数错误，直接返回 */
    }

    RTC_TimeStructInit(&RTC_TimeStruct);
    RTC_TimeStruct.RTC_H12 = RTC_H12_AM;
    RTC_TimeStruct.RTC_Hours = hour;
    RTC_TimeStruct.RTC_Minutes = minute;
    RTC_TimeStruct.RTC_Seconds = second;

    /* 禁用写保护 */
    RTC_WriteProtectionCmd(DISABLE);

    /* 设置时间，带超时保护
     * 注意：RTC_SetTime内部会自动调用RTC_WaitForSynchro()，所以不需要手动调用
     */
    timeout = 0xFFFFU;
    do
    {
        status = RTC_SetTime(RTC_Format_BIN, &RTC_TimeStruct);
        timeout--;
        if (timeout == 0U)
        {
            /* 超时退出，可能是RTC未初始化或硬件故障 */
            break;
        }
    } while (status != SUCCESS);

    /* 使能写保护 */
    RTC_WriteProtectionCmd(ENABLE);
}

/**
 * @brief  获取当前日期（年:月:日）
 * @param  year:  年份指针（返回完整年份，如2024）
 * @param  month: 月份指针
 * @param  day:   日期指针
 */
void RTC_User_GetDate(uint16_t *year, uint8_t *month, uint8_t *day)
{
    RTC_DateTypeDef RTC_DateStruct;

    /* 注意：RTC_GetDate直接读取寄存器，不需要等待同步
     * 但如果RTC没有运行，读取的值可能不会变化
     */
    RTC_GetDate(RTC_Format_BIN, &RTC_DateStruct);

    *year = 2000 + RTC_DateStruct.RTC_Year;
    *month = RTC_DateStruct.RTC_Month;
    *day = RTC_DateStruct.RTC_Date;
}

/**
 * @brief  获取当前星期（1=周一，2=周二，...，7=周日）
 * @retval 星期数 (1-7)
 */
uint8_t RTC_User_GetWeekDay(void)
{
    RTC_DateTypeDef RTC_DateStruct;

    RTC_GetDate(RTC_Format_BIN, &RTC_DateStruct);

    return RTC_DateStruct.RTC_WeekDay;
}

/**
 * @brief  设置当前日期（年:月:日）
 * @param  year:  年份 (2000-2099)
 * @param  month: 月份 (1-12)
 * @param  day:   日期 (1-31)
 *
 * 注意：参考例程实现方式，RTC_SetDate内部会自动调用RTC_WaitForSynchro()
 */
void RTC_User_SetDate(uint16_t year, uint8_t month, uint8_t day, uint8_t weekday)
{
    RTC_DateTypeDef RTC_DateStruct;
    ErrorStatus status = ERROR;
    uint32_t timeout = 0U;

    /* 参数检查 */
    if (year < 2000 || year > 2099 || month < 1 || month > 12 || day < 1 || day > 31)
    {
        return; /* 参数错误，直接返回 */
    }

    RTC_DateStructInit(&RTC_DateStruct);
    RTC_DateStruct.RTC_Year = year - 2000;
    RTC_DateStruct.RTC_Month = month;
    RTC_DateStruct.RTC_Date = day;
    RTC_DateStruct.RTC_WeekDay = weekday;

    /* 禁用写保护 */
    RTC_WriteProtectionCmd(DISABLE);

    /* 设置日期，带超时保护
     * 注意：RTC_SetDate内部会自动调用RTC_WaitForSynchro()和RTC_EnterInitMode/ExitInitMode
     * 参考例程使用简单的while循环，这里也使用相同方式，但添加超时保护避免死循环
     */
    timeout = 0xFFFFU;
    do
    {
        status = RTC_SetDate(RTC_Format_BIN, &RTC_DateStruct);
        timeout--;
        if (timeout == 0U)
        {
            /* 超时退出，可能是RTC未初始化或硬件故障 */
            /* 注意：如果超时，说明RTC_SetDate一直返回ERROR，需要检查RTC初始化 */
            break;
        }
    } while (status != SUCCESS);

    /* 使能写保护 */
    RTC_WriteProtectionCmd(ENABLE);
}

/**
 * @brief  设置指定日期的时间点
 * @param  weekday: 星期 (1=周一，2=周二，...，6=周六，7=周日)
 * @param  time_index: 时间点索引 (0-4)
 * @param  hour: 小时 (0-23)
 * @param  minute: 分钟 (0-59)
 * @retval 0-成功，1-参数错误
 */
uint8_t RTC_Motor_SetTimePoint(uint8_t weekday, uint8_t time_index, uint8_t hour, uint8_t minute)
{
    /* 参数检查 */
    if (weekday < 1 || weekday > 7) /* 支持周一到周日 */
    {
        return 1;
    }
    if (time_index >= MAX_TIME_POINTS_PER_DAY)
    {
        return 1;
    }
    if (hour >= 24 || minute >= 60)
    {
        return 1;
    }

    uint8_t day_index = weekday - 1; /* 转换为数组索引（0-6） */

    /* 设置时间点 */
    week_schedule[day_index].time_points[time_index].hour = hour;
    week_schedule[day_index].time_points[time_index].minute = minute;

    /* 更新有效时间点数量 */
    if (time_index >= week_schedule[day_index].count)
    {
        week_schedule[day_index].count = time_index + 1;
    }

    return 0;
}

/**
 * @brief  清除指定日期的所有时间点
 * @param  weekday: 星期 (1=周一，2=周二，...，6=周六，7=周日)
 * @retval 0-成功，1-参数错误
 */
uint8_t RTC_Motor_ClearDaySchedule(uint8_t weekday)
{
    if (weekday < 1 || weekday > 7)
    {
        return 1;
    }

    uint8_t day_index = weekday - 1;
    week_schedule[day_index].count = 0;

    return 0;
}

void RTC_Update(void)
{
    uint8_t hour, minute, second;
    uint8_t weekday;
    uint8_t date;
    uint16_t year;
    uint8_t month;
    uint8_t wifi_state = mcu_get_wifi_work_state();
    static uint8_t s_rtc_update_flag = 0;
    // LOG_DEBUG("RTC_Update: wifi_state: %d", wifi_state);
    if (wifi_state == WIFI_CONNECTED || wifi_state == WIFI_CONN_CLOUD)
    {
        mcu_get_system_time();
        /* 获取当前时间和日期 */
        // RTC_User_GetTime(&hour, &minute, &second);
        // weekday = RTC_User_GetWeekDay();
        // RTC_User_GetDate(&year, &month, &date);
        // LOG_DEBUG("RTC_Update: %d-%d-%d %d:%d:%d, weekday: %d", year, month, date, hour, minute, second, weekday);
    }
}

/**
 * @brief  电机定时控制函数（需要在主循环中定期调用）
 * @param  None
 * @retval None
 * @note   现在只处理日期变化（重置触发标志），时间点触发已改为RTC中断方式
 */
void RTC_Motor_TimerControl(void)
{
    uint16_t year;
    uint8_t month;
    uint8_t date;
    uint8_t hour;
    uint8_t minute;
    uint8_t second;
    static uint8_t cnt = 0;
    static uint8_t last_second = 0xFF;
    static uint32_t same_time_count = 0;

    RTC_User_GetDate(&year, &month, &date);
    RTC_User_GetTime(&hour, &minute, &second);
    int weekday = RTC_User_GetWeekDay();

    /* 检查时间是否在变化 */
    if (second == last_second && last_second != 0xFF)
    {
        same_time_count++;
        if (same_time_count > 10)
        {
            LOG_DEBUG("RTC_Motor_TimerControl: WARNING - RTC time not changing! time=%d:%d:%d", hour, minute, second);
            same_time_count = 0; /* 重置计数，避免日志过多 */
        }
    }
    else
    {
        same_time_count = 0;
        last_second = second;
    }

    /* 每5次调用打印一次时间（减少日志输出） */
    if (cnt > 4)
    {
        cnt = 0;
        // LOG_DEBUG("RTC_Motor_TimerControl: date: %d-%d-%d, time: %d:%d:%d, weekday: %d", year, month, date, hour, minute, second, weekday);
        update_next_alarm();
    }
    else
    {
        cnt++;
    }
    if (s_rtc_irq_flag)
    {
        // 中断触发，代表时间点到了
        /* 启动电机旋转1圈 */
        Motor_RunOneCycle();

        s_rtc_irq_flag = 0;
    }
}

/**
 * @brief  获取指定日期的喂食计划
 * @param  weekday: 星期 (1=周一，2=周二，...，7=周日)
 * @param  count: 返回时间点数量
 * @param  hours: 返回小时数组（至少5个元素）
 * @param  minutes: 返回分钟数组（至少5个元素）
 */
void RTC_Motor_GetSchedule(uint8_t weekday, uint8_t *count, uint8_t *hours, uint8_t *minutes)
{
    if (weekday < 1 || weekday > 7 || count == NULL || hours == NULL || minutes == NULL)
    {
        if (count != NULL)
        {
            *count = 0;
        }
        return;
    }

    uint8_t day_index = weekday - 1;
    DaySchedule_t *schedule = &week_schedule[day_index];

    *count = schedule->count;
    for (uint8_t i = 0; i < schedule->count && i < 5; i++)
    {
        hours[i] = schedule->time_points[i].hour;
        minutes[i] = schedule->time_points[i].minute;
    }
}

/**
 * @brief  设置指定日期的喂食计划
 * @param  weekday: 星期 (1=周一，2=周二，...，7=周日)
 * @param  count: 时间点数量（最多5个）
 * @param  hours: 小时数组
 * @param  minutes: 分钟数组
 */
void RTC_Motor_SetSchedule(uint8_t weekday, uint8_t count, const uint8_t *hours, const uint8_t *minutes)
{
    if (weekday < 1 || weekday > 7 || hours == NULL || minutes == NULL || count > 5)
    {
        return;
    }

    /* 先清除该天的所有时间点 */
    RTC_Motor_ClearDaySchedule(weekday);

    /* 设置新的时间点 */
    for (uint8_t i = 0; i < count; i++)
    {
        RTC_Motor_SetTimePoint(weekday, i, hours[i], minutes[i]);
    }

    /* 更新RTC闹钟 */
    RTC_Alarm_UpdateNextAlarm();
}

/**
 * @brief  添加时间点（自动查找可用索引）
 * @param  weekday: 星期 (1=周一，2=周二，...，7=周日)
 * @param  hour: 小时 (0-23)
 * @param  minute: 分钟 (0-59)
 * @retval 0-成功，1-参数错误，2-时间点已存在，3-已达到最大数量(5个)
 */
uint8_t RTC_Motor_AddTimePoint(uint8_t weekday, uint8_t hour, uint8_t minute)
{
    /* 参数检查 */
    if (weekday < 1 || weekday > 7 || hour >= 24 || minute >= 60)
    {
        return 1; /* 参数错误 */
    }

    uint8_t day_index = weekday - 1;
    DaySchedule_t *schedule = &week_schedule[day_index];

    /* 检查时间点是否已存在 */
    for (uint8_t i = 0; i < schedule->count; i++)
    {
        if (schedule->time_points[i].hour == hour &&
            schedule->time_points[i].minute == minute)
        {
            return 2; /* 时间点已存在 */
        }
    }

    /* 检查是否已达到最大数量 */
    if (schedule->count >= MAX_TIME_POINTS_PER_DAY)
    {
        return 3; /* 已达到最大数量 */
    }

    /* 找到第一个可用位置（应该就是count位置） */
    uint8_t time_index = schedule->count;
    schedule->time_points[time_index].hour = hour;
    schedule->time_points[time_index].minute = minute;
    schedule->count++;
    // LOG_DEBUG("RTC_Motor_AddTimePoint: weekday: %d, hour: %d, minute: %d, count: %d", weekday, hour, minute, schedule->count);

    /* 更新RTC闹钟 */
    // RTC_Alarm_UpdateNextAlarm();

    return 0; /* 成功 */
}

/**
 * @brief  删除指定时间点
 * @param  weekday: 星期 (1=周一，2=周二，...，7=周日)
 * @param  hour: 小时 (0-23)
 * @param  minute: 分钟 (0-59)
 * @retval 0-成功，1-参数错误，2-时间点不存在
 */
uint8_t RTC_Motor_RemoveTimePoint(uint8_t weekday, uint8_t hour, uint8_t minute)
{
    /* 参数检查 */
    if (weekday < 1 || weekday > 7 || hour >= 24 || minute >= 60)
    {
        return 1; /* 参数错误 */
    }

    uint8_t day_index = weekday - 1;
    DaySchedule_t *schedule = &week_schedule[day_index];

    /* 查找时间点 */
    uint8_t found_index = 0xFF;
    for (uint8_t i = 0; i < schedule->count; i++)
    {
        if (schedule->time_points[i].hour == hour &&
            schedule->time_points[i].minute == minute)
        {
            found_index = i;
            break;
        }
    }

    if (found_index == 0xFF)
    {
        return 2; /* 时间点不存在 */
    }

    /* 删除时间点：将后面的时间点前移 */
    for (uint8_t i = found_index; i < schedule->count - 1; i++)
    {
        schedule->time_points[i].hour = schedule->time_points[i + 1].hour;
        schedule->time_points[i].minute = schedule->time_points[i + 1].minute;
    }

    schedule->count--;
    // LOG_DEBUG("RTC_Motor_RemoveTimePoint: weekday: %d, hour: %d, minute: %d, count: %d", weekday, hour, minute, schedule->count);

    /* 更新RTC闹钟 */
    // RTC_Alarm_UpdateNextAlarm();

    return 0; /* 成功 */
}

/**
 * @brief  配置RTC闹钟NVIC中断
 */
static void RTC_Alarm_ConfigNVIC(void)
{
    NVIC_InitTypeDef nvic_init;

    nvic_init.NVIC_IRQChannel = RTC_IRQn;
    nvic_init.NVIC_IRQChannelPriority = 0;
    nvic_init.NVIC_IRQChannelCmd = ENABLE;
    NVIC_Init(&nvic_init);
}

/**
 * @brief  配置RTC闹钟EXTI中断（Line17）
 */
static void RTC_Alarm_ConfigEXTI(void)
{
    EXTI_InitTypeDef exti_init;

    exti_init.EXTI_Line = EXTI_Line17;
    exti_init.EXTI_Mode = EXTI_Mode_Interrupt;
    exti_init.EXTI_Trigger = EXTI_Trigger_Rising;
    exti_init.EXTI_LineCmd = ENABLE;
    EXTI_Init(&exti_init);
}

void update_next_alarm(void)
{
    RTC_Alarm_UpdateNextAlarm();
}

/**
 * @brief  查找下一个最近的时间点并设置RTC闹钟
 */
static void RTC_Alarm_UpdateNextAlarm(void)
{
    RTC_TimeTypeDef current_time;
    RTC_DateTypeDef current_date;
    RTC_AlarmTypeDef alarm;
    static RTC_AlarmTypeDef s_last_alarm;
    static uint8_t s_last_alarm_valid = 0; /* 标记上次闹钟是否有效 */
    uint8_t current_weekday;
    uint8_t current_hour, current_minute, current_second;
    uint8_t found = 0;
    uint8_t next_weekday = 0;
    uint8_t next_hour = 0;
    uint8_t next_minute = 0;
    uint32_t min_minutes = 0xFFFFFFFF; /* 最小分钟数差值 */

    /* 获取当前时间和日期 */
    RTC_GetTime(RTC_Format_BIN, &current_time);
    RTC_GetDate(RTC_Format_BIN, &current_date);
    current_weekday = current_date.RTC_WeekDay;
    current_hour = current_time.RTC_Hours;
    current_minute = current_time.RTC_Minutes;
    current_second = current_time.RTC_Seconds;

    /* 计算当前时间的总分钟数 */
    uint32_t current_total_minutes = current_hour * 60 + current_minute;

    /* 仅查找今天剩余的时间点（每10秒更新，无需跨天遍历） */
    {
        uint8_t day_index = current_weekday - 1;
        DaySchedule_t *schedule = &week_schedule[day_index];

        for (uint8_t i = 0; i < schedule->count; i++)
        {
            TimePoint_t *tp = &schedule->time_points[i];
            uint32_t tp_total_minutes = tp->hour * 60 + tp->minute;

            /* 只考虑今天尚未到达的时间点 */
            if (tp_total_minutes > current_total_minutes)
            {
                uint32_t minutes_diff = tp_total_minutes - current_total_minutes;
                if (minutes_diff < min_minutes)
                {
                    min_minutes = minutes_diff;
                    next_weekday = current_weekday;
                    next_hour = tp->hour;
                    next_minute = tp->minute;
                    found = 1;
                }
            }
        }
    }

    if (found)
    {
        /* 配置闹钟结构体 */
        RTC_AlarmStructInit(&alarm);
        alarm.RTC_AlarmDateWeekDay = next_weekday;
        alarm.RTC_AlarmDateWeekDaySel = RTC_AlarmDateWeekDaySel_WeekDay;
        alarm.RTC_AlarmMask = RTC_AlarmMask_None; /* 精确匹配 */
        alarm.RTC_AlarmTime.RTC_H12 = (next_hour >= 12) ? RTC_H12_PM : RTC_H12_AM;
        alarm.RTC_AlarmTime.RTC_Hours = next_hour;
        alarm.RTC_AlarmTime.RTC_Minutes = next_minute;
        alarm.RTC_AlarmTime.RTC_Seconds = 0;

        /* 检查新闹钟是否与上次设置的闹钟相同 */
        uint8_t need_update = 0;
        if (s_last_alarm_valid == 0)
        {
            /* 上次闹钟无效，需要设置 */
            need_update = 1;
        }
        else if (s_last_alarm.RTC_AlarmDateWeekDay != alarm.RTC_AlarmDateWeekDay ||
                 s_last_alarm.RTC_AlarmTime.RTC_Hours != alarm.RTC_AlarmTime.RTC_Hours ||
                 s_last_alarm.RTC_AlarmTime.RTC_Minutes != alarm.RTC_AlarmTime.RTC_Minutes)
        {
            /* 闹钟时间不同，需要更新 */
            need_update = 1;
        }

        if (need_update)
        {
            /* 如果找到时间点，设置RTC闹钟 */
            /* 禁用当前闹钟 */
            RTC_AlarmCmd(RTC_Alarm_A, DISABLE);

            /* 设置闹钟 */
            RTC_WriteProtectionCmd(DISABLE);
            RTC_SetAlarm(RTC_Format_BIN, RTC_Alarm_A, &alarm);
            s_last_alarm = alarm;
            s_last_alarm_valid = 1;
            RTC_WriteProtectionCmd(ENABLE);

            /* 配置闹钟子秒（使用当前子秒） */
            RTC_AlarmSubSecondConfig(RTC_Alarm_A, RTC_GetSubSecond(), RTC_AlarmSubSecondMask_None);

            /* 使能闹钟 */
            RTC_AlarmCmd(RTC_Alarm_A, ENABLE);
            LOG_DEBUG("RTC_Alarm_UpdateNextAlarm: alarm: %02d:%02d weekday: %d", alarm.RTC_AlarmTime.RTC_Hours, alarm.RTC_AlarmTime.RTC_Minutes, alarm.RTC_AlarmDateWeekDay);
        }
        /* 如果闹钟相同，跳过配置，避免重复操作 */
    }
    else
    {
        /* 没有找到时间点，禁用闹钟并清除标记 */
        if (s_last_alarm_valid != 0)
        {
            RTC_AlarmCmd(RTC_Alarm_A, DISABLE);
            s_last_alarm_valid = 0;
            LOG_DEBUG("RTC_Alarm_UpdateNextAlarm: today is no alarm");
        }
    }
}

/**
 * @brief  RTC中断处理函数（在中断文件中调用）
 */
void RTC_IRQHandler(void)
{
    if (RTC_GetITStatus(RTC_IT_ALRA) != RESET)
    {
        /* 清除中断标志 */
        /* 注意：闹钟触发后，下次update_next_alarm会查找新的时间点，无需手动清除s_last_alarm_valid */
        RTC_ClearITPendingBit(RTC_IT_ALRA);
        EXTI_ClearITPendingBit(EXTI_Line17);
        s_rtc_irq_flag = 1;
        // RTC中断会自动退出低功耗模式
    }
}
