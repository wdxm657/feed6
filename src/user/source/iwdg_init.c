#include "iwdg_init.h"
#include "ft32f0xx.h"
#include "ft32f0xx_iwdg.h"
#include "ft32f0xx_rcc.h"
#include "systick_tick.h"

/* LSI频率（Hz），典型值约40kHz */
#define LSI_FREQ_HZ 40000U

/* 看门狗配置参数 */
#define IWDG_PRESCALER IWDG_Prescaler_32  /* 预分频器：32 */
/* 重载值计算：超时时间 = (重载值 + 1) / (LSI频率 / 预分频器)
 * 例如：重载值 = 312，超时时间 = (312 + 1) / (40000 / 32) ≈ 0.25秒
 * 例如：重载值 = 1250，超时时间 = (1250 + 1) / (40000 / 32) ≈ 1秒
 * 例如：重载值 = 12500，超时时间 = (12500 + 1) / (40000 / 32) ≈ 10秒
 */
#define IWDG_RELOAD_VALUE (LSI_FREQ_HZ / 4U) /* 重载值：10000，超时时间约8秒 */

/* 看门狗初始化标志 */
static uint8_t s_iwdg_initialized = 0U;

/* 上次喂狗时间（用于任务调度） */
static uint32_t s_last_feed_ms = 0U;
#define IWDG_FEED_INTERVAL_MS 500U /* 喂狗间隔：500ms */

/**
 * @brief 初始化独立看门狗（IWDG）
 * @note 看门狗使用LSI（约40kHz），超时时间约1秒
 *       需要在系统初始化时调用，且只能初始化一次
 */
void IWDG_Init(void)
{
	if (s_iwdg_initialized != 0U)
	{
		return; /* 已经初始化过，避免重复初始化 */
	}

	/* 使能LSI（如果还未使能） */
	RCC_LSICmd(ENABLE);

	/* 等待LSI稳定 */
	uint32_t timeout = 0U;
	while (RCC_GetFlagStatus(RCC_FLAG_LSIRDY) == RESET)
	{
		timeout++;
		if (timeout > 1000U)
		{
			/* LSI启动超时，但继续执行（LSI可能已经稳定） */
			break;
		}
	}

	/* 使能看门狗 */
	IWDG_Enable();

	/* 等待LSI稳定（参考例程中的延时） */
	volatile uint32_t i;
	for (i = 0U; i <= 500U; i++)
	{
		/* 空循环等待 */
	}

	/* 使能写访问 */
	IWDG_WriteAccessCmd(IWDG_WriteAccess_Enable);

	/* 设置预分频器 */
	IWDG_SetPrescaler(IWDG_PRESCALER);

	/* 等待预分频器更新完成 */
	while (IWDG_GetFlagStatus(IWDG_FLAG_PVU) != RESET)
	{
		/* 等待PVU标志清除 */
	}

	/* 设置重载值 */
	IWDG_SetReload(IWDG_RELOAD_VALUE);

	/* 等待重载值更新完成 */
	while (IWDG_GetFlagStatus(IWDG_FLAG_RVU) != RESET)
	{
		/* 等待RVU标志清除 */
	}

	/* 重载计数器（第一次喂狗） */
	IWDG_ReloadCounter();

	/* 标记已初始化 */
	s_iwdg_initialized = 1U;
	s_last_feed_ms = Systick_Tick_GetMs();
}

/**
 * @brief 喂狗（重载看门狗计数器）
 * @note 需要在看门狗超时前定期调用，建议间隔小于超时时间的1/2
 */
void IWDG_Feed(void)
{
	if (s_iwdg_initialized == 0U)
	{
		return; /* 看门狗未初始化，不喂狗 */
	}

	IWDG_ReloadCounter();
	s_last_feed_ms = Systick_Tick_GetMs();
}

/**
 * @brief 看门狗喂狗任务（用于任务调度器）
 * @note 此任务会定期喂狗，建议注册为高优先级任务，间隔500ms
 */
void IWDG_Task(void)
{
	if (s_iwdg_initialized == 0U)
	{
		return; /* 看门狗未初始化 */
	}

	uint32_t current_ms = Systick_Tick_GetMs();

	/* 检查是否到了喂狗时间 */
	if (s_last_feed_ms == 0U || Systick_Tick_IsTimeout(s_last_feed_ms, IWDG_FEED_INTERVAL_MS) != 0U)
	{
		IWDG_Feed();
	}
}

