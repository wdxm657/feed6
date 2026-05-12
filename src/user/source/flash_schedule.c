#include "flash_schedule.h"
#include "rtc_init.h"
#include "ft32f0xx_flash.h"
#include "log.h"
#include <string.h>

/* Flash存储区域指针 */
#define FLASH_SCHEDULE_PTR  ((FlashScheduleStorage_t *)FLASH_SCHEDULE_BASE)

/* 读写请求标志 */
volatile uint8_t g_flash_schedule_write_request = 0U;
volatile uint8_t g_flash_schedule_read_request = 0U;

/* 临时缓冲区（用于读写操作） */
static FlashScheduleStorage_t s_schedule_buffer;

/**
 * @brief 擦除Flash存储页
 */
static uint8_t Flash_Schedule_ErasePage(void)
{
    FLASH_Status status;
    
    /* 解锁Flash */
    FLASH_Unlock();
    
    /* 清除所有Flash标志 */
    FLASH_ClearFlag(FLASH_FLAG_EOP | FLASH_FLAG_PGERR | FLASH_FLAG_WRPERR);
    
    /* 擦除页（页地址 = FLASH_SCHEDULE_BASE） */
    status = FLASH_ErasePage(FLASH_SCHEDULE_BASE);
    
    /* 锁定Flash */
    FLASH_Lock();
    
    return (status == FLASH_COMPLETE) ? 1U : 0U;
}

/**
 * @brief 写入一个字（4字节）到Flash
 */
static uint8_t Flash_Schedule_WriteWord(uint32_t address, uint32_t data)
{
    FLASH_Status status;
    
    status = FLASH_ProgramWord(address, data);
    
    return (status == FLASH_COMPLETE) ? 1U : 0U;
}

/**
 * @brief 从RTC模块获取当前喂食计划并保存到Flash
 */
uint8_t Flash_Schedule_Save(void)
{
    uint32_t address;
    uint32_t *data_ptr;
    uint16_t i, j;
    
    /* 构建数据结构 */
    s_schedule_buffer.magic = FLASH_SCHEDULE_MAGIC;
    
    /* 从RTC模块获取喂食计划数据 */
    uint8_t hours[5];
    uint8_t minutes[5];
    uint8_t count;
    
    for (uint8_t weekday = 1; weekday <= 7; weekday++)
    {
        uint8_t day_index = weekday - 1;
        RTC_Motor_GetSchedule(weekday, &count, hours, minutes);
        
        s_schedule_buffer.week_schedule[day_index].count = count;
        for (uint8_t i = 0; i < count && i < 5; i++)
        {
            s_schedule_buffer.week_schedule[day_index].time_points[i].hour = hours[i];
            s_schedule_buffer.week_schedule[day_index].time_points[i].minute = minutes[i];
        }

    }
    
    /* 擦除Flash页 */
    if (Flash_Schedule_ErasePage() == 0U)
    {
        return 0U;
    }
    
    /* 解锁Flash */
    FLASH_Unlock();
    
    /* 清除所有Flash标志 */
    FLASH_ClearFlag(FLASH_FLAG_EOP | FLASH_FLAG_PGERR | FLASH_FLAG_WRPERR);
    
    /* 写入数据（按4字节对齐） */
    address = FLASH_SCHEDULE_BASE;
    data_ptr = (uint32_t *)&s_schedule_buffer;
    
    /* 计算需要写入的字数 */
    uint16_t word_count = sizeof(FlashScheduleStorage_t) / 4;
    if ((sizeof(FlashScheduleStorage_t) % 4) != 0)
    {
        word_count++;
    }
    
    for (i = 0; i < word_count; i++)
    {
        if (Flash_Schedule_WriteWord(address, data_ptr[i]) == 0U)
        {
            FLASH_Lock();
            return 0U;
        }
        address += 4;
    }
    
    /* 锁定Flash */
    FLASH_Lock();
    
    return 1U;
}

/**
 * @brief 从Flash读取喂食计划并应用到RTC模块
 */
uint8_t Flash_Schedule_Load(void)
{
    FlashScheduleStorage_t *storage = FLASH_SCHEDULE_PTR;
    uint8_t weekday, time_index;
    
    /* 检查魔数 */
    if (storage->magic != FLASH_SCHEDULE_MAGIC)
    {
        return 0U;  /* 没有有效数据 */
    }
    
    /* 清除所有现有的时间点 */
    for (weekday = 1; weekday <= 7; weekday++)
    {
        RTC_Motor_ClearDaySchedule(weekday);
    }
    
    /* 恢复喂食计划 */
    uint8_t hours[5];
    uint8_t minutes[5];
    
    for (weekday = 1; weekday <= 7; weekday++)
    {
        uint8_t day_index = weekday - 1;
        FlashDaySchedule_t *day_schedule = &storage->week_schedule[day_index];
        
        /* 提取时间点数据 */
        for (time_index = 0; time_index < day_schedule->count && time_index < 5; time_index++)
        {
            hours[time_index] = day_schedule->time_points[time_index].hour;
            minutes[time_index] = day_schedule->time_points[time_index].minute;
        }
        
        /* 批量设置 */
        if (day_schedule->count > 0)
        {
            RTC_Motor_SetSchedule(weekday, day_schedule->count, hours, minutes);
        }
    }

    return 1U;
}

/**
 * @brief 清除Flash中的喂食计划
 */
uint8_t Flash_Schedule_Erase(void)
{
    return Flash_Schedule_ErasePage();
}

/**
 * @brief 触发写入请求
 */
void Flash_Schedule_RequestWrite(void)
{
    g_flash_schedule_write_request = 1U;
}

/**
 * @brief 触发读取请求
 */
void Flash_Schedule_RequestRead(void)
{
    g_flash_schedule_read_request = 1U;
}

/**
 * @brief Flash读写任务（需要在主循环中定期调用）
 */
void Flash_Schedule_Process(void)
{
    /* 处理写入请求 */
    if (g_flash_schedule_write_request != 0U)
    {
        g_flash_schedule_write_request = 0U;
        
        /* 从RTC模块获取数据并保存到Flash */
        Flash_Schedule_Save();
    }
    
    /* 处理读取请求 */
    if (g_flash_schedule_read_request != 0U)
    {
        g_flash_schedule_read_request = 0U;
        
        /* 从Flash读取并应用到RTC模块 */
        Flash_Schedule_Load();
    }
}

