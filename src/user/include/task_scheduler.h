#ifndef __TASK_SCHEDULER_H__
#define __TASK_SCHEDULER_H__

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

/* 任务优先级定义（数值越大优先级越高） */
typedef enum
{
	TASK_PRIORITY_IDLE = 0,      /* 空闲任务 */
	TASK_PRIORITY_LOW = 1,       /* 低优先级 */
	TASK_PRIORITY_NORMAL = 2,    /* 普通优先级 */
	TASK_PRIORITY_HIGH = 3,      /* 高优先级 */
	TASK_PRIORITY_CRITICAL = 4   /* 关键任务 */
} TaskPriority_t;

/* 任务函数类型 */
typedef void (*TaskFunc_t)(void);

/* 任务控制块 */
typedef struct
{
	TaskFunc_t task_func;        /* 任务函数指针 */
	TaskPriority_t priority;     /* 任务优先级 */
	uint32_t interval_ms;        /* 执行间隔（毫秒） */
	uint32_t last_run_ms;        /* 上次执行时间 */
	uint8_t enabled;             /* 任务使能标志 */
} TaskControlBlock_t;

/**
 * @brief 初始化任务调度器
 */
void TaskScheduler_Init(void);

/**
 * @brief 注册一个任务
 * @param task_func 任务函数指针
 * @param priority 任务优先级
 * @param interval_ms 执行间隔（毫秒），0表示每个循环都执行
 * @return 任务ID（0表示注册失败）
 */
uint8_t TaskScheduler_Register(TaskFunc_t task_func, TaskPriority_t priority, uint32_t interval_ms);

/**
 * @brief 使能/禁用任务
 * @param task_id 任务ID
 * @param enabled 1-使能，0-禁用
 */
void TaskScheduler_SetEnabled(uint8_t task_id, uint8_t enabled);

/**
 * @brief 任务调度器主循环（在main的while循环中调用）
 */
void TaskScheduler_Run(void);

/**
 * @brief 获取任务调度器统计信息（用于调试）
 * @param total_tasks 总任务数（输出参数）
 * @param enabled_tasks 使能的任务数（输出参数）
 */
void TaskScheduler_GetStats(uint8_t *total_tasks, uint8_t *enabled_tasks);

/**
 * @brief 停止所有任务（用于低功耗模式）
 */
void TaskScheduler_StopAll(void);

/**
 * @brief 恢复所有任务（从低功耗模式唤醒后）
 */
void TaskScheduler_ResumeAll(void);

#ifdef __cplusplus
}
#endif

#endif /* __TASK_SCHEDULER_H__ */

