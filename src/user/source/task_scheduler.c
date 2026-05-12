#include "task_scheduler.h"
#include "systick_tick.h"
#include <string.h>

/* 最大任务数量（根据实际需求调整，每个任务占用约20字节） */
#define MAX_TASKS  16U

/* 任务控制块数组 */
static TaskControlBlock_t s_tasks[MAX_TASKS];
static uint8_t s_task_count = 0U;
static uint8_t s_initialized = 0U;

/**
 * @brief 初始化任务调度器
 */
void TaskScheduler_Init(void)
{
	if (s_initialized != 0U)
	{
		return;
	}
	
	memset(s_tasks, 0, sizeof(s_tasks));
	s_task_count = 0U;
	s_initialized = 1U;
}

/**
 * @brief 注册一个任务
 */
uint8_t TaskScheduler_Register(TaskFunc_t task_func, TaskPriority_t priority, uint32_t interval_ms)
{
	if (task_func == NULL || s_task_count >= MAX_TASKS)
	{
		return 0U;
	}
	
	/* 查找空闲槽位 */
	uint8_t i;
	for (i = 0U; i < MAX_TASKS; i++)
	{
		if (s_tasks[i].task_func == NULL)
		{
			s_tasks[i].task_func = task_func;
			s_tasks[i].priority = priority;
			s_tasks[i].interval_ms = interval_ms;
			/* 设置last_run_ms为0，让任务在第一次调用时立即执行 */
			s_tasks[i].last_run_ms = 0U;
			s_tasks[i].enabled = 1U;
			s_task_count++;
			return (i + 1U); /* 返回任务ID（从1开始） */
		}
	}
	
	return 0U;
}

/**
 * @brief 使能/禁用任务
 */
void TaskScheduler_SetEnabled(uint8_t task_id, uint8_t enabled)
{
	if (task_id == 0U || task_id > MAX_TASKS)
	{
		return;
	}
	
	uint8_t index = task_id - 1U;
	if (s_tasks[index].task_func != NULL)
	{
		s_tasks[index].enabled = enabled ? 1U : 0U;
	}
}

/**
 * @brief 任务调度器主循环
 * 
 * 按照优先级从高到低执行任务，相同优先级的任务按注册顺序执行
 * 支持时间间隔控制，避免任务执行过于频繁
 */
void TaskScheduler_Run(void)
{
	if (s_initialized == 0U)
	{
		return;
	}
	
	uint32_t current_ms = Systick_Tick_GetMs();
	
	/* 按优先级从高到低执行任务 */
	TaskPriority_t priority;
	for (priority = TASK_PRIORITY_CRITICAL; priority > TASK_PRIORITY_IDLE; priority--)
	{
		uint8_t i;
		for (i = 0U; i < MAX_TASKS; i++)
		{
			TaskControlBlock_t *task = &s_tasks[i];
			/* 检查任务是否有效且使能 */
			if (task->task_func == NULL || task->enabled == 0U)
			{
				continue;
			}
			
			/* 检查优先级是否匹配 */
			if (task->priority != priority)
			{
				continue;
			}
			
			/* 检查是否到了执行时间 */
			if (task->interval_ms == 0U)
			{
				/* 间隔为0，每个循环都执行 */
				task->task_func();
			}
			else
			{
				/* 检查是否超时 */
				uint32_t elapsed = 0U;
				
				/* 如果last_run_ms为0，说明是第一次执行，立即执行 */
				if (task->last_run_ms == 0U)
				{
					task->task_func();
					task->last_run_ms = current_ms;
				}
				else
				{
					/* 计算经过的时间 */
					if (current_ms >= task->last_run_ms)
					{
						elapsed = current_ms - task->last_run_ms;
					}
					else
					{
						/* 处理计数器溢出（这种情况在正常运行中几乎不会发生） */
						elapsed = (0xFFFFFFFFU - task->last_run_ms) + current_ms + 1U;
					}
					
					/* 如果经过的时间大于等于设定的间隔，执行任务 */
					if (elapsed >= task->interval_ms)
					{
						task->task_func();
						task->last_run_ms = current_ms;
					}
					/* 注意：如果elapsed < interval_ms，任务不会执行，这是正常的
					 * 任务会等待直到时间间隔到达
					 */
				}
			}
		}
	}
}

/**
 * @brief 获取任务调度器统计信息
 */
void TaskScheduler_GetStats(uint8_t *total_tasks, uint8_t *enabled_tasks)
{
	if (total_tasks != NULL)
	{
		*total_tasks = s_task_count;
	}
	
	if (enabled_tasks != NULL)
	{
		uint8_t count = 0U;
		uint8_t i;
		for (i = 0U; i < MAX_TASKS; i++)
		{
			if (s_tasks[i].task_func != NULL && s_tasks[i].enabled != 0U)
			{
				count++;
			}
		}
		*enabled_tasks = count;
	}
}

/**
 * @brief 停止所有任务（用于低功耗模式）
 */
void TaskScheduler_StopAll(void)
{
	uint8_t i;
	for (i = 0U; i < MAX_TASKS; i++)
	{
		if (s_tasks[i].task_func != NULL)
		{
			s_tasks[i].enabled = 0U; /* 禁用所有任务 */
		}
	}
}

/**
 * @brief 恢复所有任务（从低功耗模式唤醒后）
 */
void TaskScheduler_ResumeAll(void)
{
	uint8_t i;
	for (i = 0U; i < MAX_TASKS; i++)
	{
		if (s_tasks[i].task_func != NULL)
		{
			s_tasks[i].enabled = 1U; /* 使能所有任务 */
		}
	}
}

