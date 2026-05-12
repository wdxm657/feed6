#include "main.h"
#include "delay.h"

static uint32_t s_fac_us = 0U;

void Delay_Init(void)
{
	/* 更新系统时钟频率变量 */
	SystemCoreClockUpdate();

	/* SysTick 使用 HCLK 时钟源 */
	SysTick_CLKSourceConfig(SysTick_CLKSource_HCLK);

	s_fac_us = SystemCoreClock / 1000000U;
}

void Delay_us(uint32_t us)
{
	if (s_fac_us == 0U || us == 0U)
	{
		return;
	}

	uint32_t total_cycles = s_fac_us * us;
	while (total_cycles > 0U)
	{
		uint32_t chunk = (total_cycles > SysTick_LOAD_RELOAD_Msk) ? SysTick_LOAD_RELOAD_Msk : total_cycles;

		SysTick->LOAD = chunk - 1U;
		SysTick->VAL = 0U;
		SysTick->CTRL = SysTick_CTRL_CLKSOURCE_Msk | SysTick_CTRL_ENABLE_Msk;

		while ((SysTick->CTRL & SysTick_CTRL_COUNTFLAG_Msk) == 0U) { }

		SysTick->CTRL = 0U;
		total_cycles -= chunk;
	}
}

void Delay_ms(uint32_t ms)
{
	while (ms--)
	{
		Delay_us(1000U);
	}
}

