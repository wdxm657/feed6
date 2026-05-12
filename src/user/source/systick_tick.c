#include "systick_tick.h"
#include "ft32f0xx.h"
#include "ft32f0xx_misc.h"

/* 系统运行时间计数器（毫秒） */
static volatile uint32_t s_systick_ms = 0U;

/**
 * @brief SysTick中断服务函数
 * 
 * 注意：此函数必须与启动文件中的弱定义匹配
 * 启动文件中有弱定义：SysTick_Handler [WEAK]
 * 我们的实现会覆盖弱定义
 */
void SysTick_Handler(void)
{
	s_systick_ms++;
}

/**
 * @brief 初始化SysTick定时器，配置为1ms中断
 * 
 * 参考例程实现方式：
 * SysTick_Config() 函数会自动：
 * - 配置SysTick重载寄存器
 * - 设置SysTick IRQ优先级为最低值(0x0F)
 * - 复位SysTick计数器寄存器
 * - 配置SysTick计数器时钟源为Core Clock Source (HCLK)
 * - 使能SysTick中断
 * - 启动SysTick计数器
 */
void Systick_Tick_Init(void)
{
	/* 更新系统时钟频率变量 */
	SystemCoreClockUpdate();
	
	/* 配置SysTick为1ms中断一次
	 * SystemCoreClock / 1000 = 每毫秒的时钟周期数
	 * SysTick_Config会自动配置时钟源为HCLK并使能中断
	 */
	if (SysTick_Config(SystemCoreClock / 1000) != 0)
	{
		/* 配置失败，捕获错误 */
		while (1) { }
	}
}

/**
 * @brief 获取系统运行时间（毫秒）
 * @return 系统运行时间，单位：毫秒
 */
uint32_t Systick_Tick_GetMs(void)
{
	return s_systick_ms;
}

/**
 * @brief 检查是否经过了指定时间
 * @param start_ms 开始时间（毫秒）
 * @param duration_ms 持续时间（毫秒）
 * @return 1-已到达，0-未到达
 */
uint8_t Systick_Tick_IsTimeout(uint32_t start_ms, uint32_t duration_ms)
{
	uint32_t current_ms = Systick_Tick_GetMs();
	uint32_t elapsed = 0U;

	/* 处理计数器溢出的情况 */
	if (current_ms >= start_ms)
	{
		elapsed = current_ms - start_ms;
	}
	else
	{
		/* 计数器溢出，计算溢出后的时间 */
		elapsed = (0xFFFFFFFFU - start_ms) + current_ms + 1U;
	}

	return (elapsed >= duration_ms) ? 1U : 0U;
}

/**
 * @brief 获取系统时钟频率（用于调试）
 */
uint32_t Systick_Tick_GetSystemClock(void)
{
	SystemCoreClockUpdate();
	return SystemCoreClock;
}

/**
 * @brief 测试SysTick是否正常工作（用于调试）
 * 
 * 此函数会等待100ms，然后检查s_systick_ms是否增加
 * 如果增加，说明SysTick中断正常工作
 * 
 * @return 1-正常，0-异常
 */
uint8_t Systick_Tick_Test(void)
{
	uint32_t start_ms = s_systick_ms;
	uint32_t start_time = Systick_Tick_GetMs();
	
	/* 等待100ms（使用Delay函数，不依赖SysTick中断） */
	/* 注意：这里不能使用Delay_ms，因为它可能依赖SysTick */
	/* 我们使用简单的循环延时 */
	volatile uint32_t i;
	for (i = 0; i < 100000; i++) { }
	
	/* 检查s_systick_ms是否增加 */
	if (s_systick_ms > start_ms)
	{
		return 1U;  /* SysTick中断正常工作 */
	}
	
	return 0U;  /* SysTick中断没有触发 */
}

/**
 * @brief 停止SysTick定时器（用于低功耗模式）
 */
void Systick_Tick_Stop(void)
{
	/* 禁用SysTick中断和计数器 */
	SysTick->CTRL &= ~(SysTick_CTRL_TICKINT_Msk | SysTick_CTRL_ENABLE_Msk);
}

/**
 * @brief 恢复SysTick定时器（从低功耗模式唤醒后）
 */
void Systick_Tick_Resume(void)
{
	/* 重新配置SysTick为1ms中断一次 */
	if (SysTick_Config(SystemCoreClock / 1000) != 0)
	{
		/* 配置失败，捕获错误 */
		while (1) { }
	}
}
