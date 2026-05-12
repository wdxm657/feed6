#ifndef __FLASH_SCHEDULE_H__
#define __FLASH_SCHEDULE_H__

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

/* Flash喂食计划存储区域定义（使用倒数第二页，2KB） */
#define FLASH_SCHEDULE_BASE      0x08007000U
#define FLASH_SCHEDULE_SIZE      2048U
#define FLASH_SCHEDULE_MAGIC     0x53434844U  /* "SCHD" 魔数 */

/* 喂食计划数据结构（与rtc_init.c中的DaySchedule_t兼容） */
typedef struct
{
    uint8_t hour;      /* 小时 (0-23) */
    uint8_t minute;   /* 分钟 (0-59) */
} FlashTimePoint_t;

typedef struct
{
    FlashTimePoint_t time_points[5];  /* 每天最多5个时间点 */
    uint8_t count;  /* 有效时间点数量 */
} FlashDaySchedule_t;

typedef struct
{
    uint32_t magic;  /* 魔数：0x53434844 ("SCHD") */
    FlashDaySchedule_t week_schedule[7];  /* 周一到周日的时间表 */
} FlashScheduleStorage_t;

/* 读写请求标志 */
extern volatile uint8_t g_flash_schedule_write_request;
extern volatile uint8_t g_flash_schedule_read_request;

/**
 * @brief 保存喂食计划到Flash
 * @return 0-失败，1-成功
 */
uint8_t Flash_Schedule_Save(void);

/**
 * @brief 从Flash读取喂食计划
 * @return 0-失败或没有数据，1-成功
 */
uint8_t Flash_Schedule_Load(void);

/**
 * @brief 清除Flash中的喂食计划
 * @return 0-失败，1-成功
 */
uint8_t Flash_Schedule_Erase(void);

/**
 * @brief 触发写入请求（由外部调用，实际写入在任务中执行）
 */
void Flash_Schedule_RequestWrite(void);

/**
 * @brief 触发读取请求（由外部调用，实际读取在任务中执行）
 */
void Flash_Schedule_RequestRead(void);

/**
 * @brief Flash读写任务（需要在主循环中定期调用）
 */
void Flash_Schedule_Process(void);

#ifdef __cplusplus
}
#endif

#endif /* __FLASH_SCHEDULE_H__ */

