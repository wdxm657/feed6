#include "main.h"
#include "pwm_init.h"
#include <stdint.h>
#include "ft32f0xx_gpio.h"
#include "ft32f0xx_rcc.h"
#include "ft32f0xx_tim.h"
#include "delay.h"

static void PWM_ConfigGPIO(void);
static void PWM_Apply(uint32_t prescaler, uint32_t period, uint32_t pulse);

void PWM_Init(void)
{
	/* 时钟 */
	RCC_AHBPeriphClockCmd(RCC_AHBPeriph_GPIOB, ENABLE);
	RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM3, ENABLE);

	PWM_ConfigGPIO();

	/* 默认初始化为静音 */
	PWM_Stop();
}

void PWM_SetFrequency(uint32_t freq_hz)
{
	if (freq_hz == 0)
	{
		PWM_Stop();
		return;
	}

	uint32_t timer_clk = SystemCoreClock;
	uint32_t prescaler = 0U;
	uint32_t period = 0U;

	if (freq_hz == 0U)
	{
		PWM_Stop();
		return;
	}

	/* 参考官方例程：先尝试不分频 */
	period = (timer_clk / freq_hz) - 1U;

	while (period > 0xFFFFU && prescaler < 0xFFFFU)
	{
		prescaler++;
		period = (timer_clk / ((prescaler + 1U) * freq_hz)) - 1U;
	}

	if (period > 0xFFFFU)
	{
		PWM_Stop();
		return;
	}

	/* 固定50%占空比 */
	uint32_t pulse = (period + 1U) / 2U;
	
	/* 重新配置GPIO为AF模式（因为PWM_Stop可能将GPIO切换回了普通输出模式） */
	PWM_ConfigGPIO();
	
	PWM_Apply((uint16_t)prescaler, (uint16_t)period, (uint16_t)pulse);
}

void PWM_Stop(void)
{
	/* 禁用定时器 */
	TIM_Cmd(TIM3, DISABLE);
	
	/* 将GPIO从AF模式切换回普通输出模式，并设置为低电平 */
	GPIO_InitTypeDef gpio_init;
	GPIO_StructInit(&gpio_init);
	gpio_init.GPIO_Pin = GPIO_Pin_4;
	gpio_init.GPIO_Mode = GPIO_Mode_OUT;
	gpio_init.GPIO_OType = GPIO_OType_PP;
	gpio_init.GPIO_Speed = GPIO_Speed_2MHz;
	gpio_init.GPIO_PuPd = GPIO_PuPd_NOPULL;
	GPIO_Init(GPIOB, &gpio_init);
	
	/* 设置为低电平 */
	GPIO_ResetBits(GPIOB, GPIO_Pin_4);
}

void PWM_PlayMelodyTest(void)
{
	/* 简单旋律：A4(440Hz), B4(494Hz), C5(523Hz), D5(587Hz) */
	const uint32_t notes[] = {440, 494, 523, 587, 659, 698, 784, 880};
	const uint32_t durations[] = {250, 250, 250, 250, 250, 250, 250, 500};
	const uint32_t count = sizeof(notes)/sizeof(notes[0]);

	for (uint32_t i = 0; i < count; i++)
	{
		PWM_SetFrequency(notes[i]);
		Delay_ms(durations[i]);
	}

	PWM_Stop();
}

static void PWM_ConfigGPIO(void)
{
	/* PB4 -> TIM3_CH1 (AF1) */
	GPIO_InitTypeDef gpio_init;
	GPIO_StructInit(&gpio_init);
	gpio_init.GPIO_Pin = GPIO_Pin_4;
	gpio_init.GPIO_Mode = GPIO_Mode_AF;
	gpio_init.GPIO_OType = GPIO_OType_PP;
	gpio_init.GPIO_Speed = GPIO_Speed_10MHz;
	gpio_init.GPIO_PuPd = GPIO_PuPd_NOPULL;
	GPIO_Init(GPIOB, &gpio_init);

	GPIO_PinAFConfig(GPIOB, GPIO_PinSource4, GPIO_AF_1);
}

static void PWM_Apply(uint32_t prescaler, uint32_t period, uint32_t pulse)
{
	TIM_TimeBaseInitTypeDef tb;
	TIM_OCInitTypeDef oc;

	TIM_TimeBaseStructInit(&tb);
	tb.TIM_Prescaler = prescaler;
	tb.TIM_CounterMode = TIM_CounterMode_Up;
	tb.TIM_Period = period;
	tb.TIM_ClockDivision = TIM_CKD_DIV1;
	TIM_TimeBaseInit(TIM3, &tb);

	TIM_OCStructInit(&oc);
	oc.TIM_OCMode = TIM_OCMode_PWM1;
	oc.TIM_OutputState = TIM_OutputState_Enable;
	oc.TIM_Pulse = pulse;
	oc.TIM_OCPolarity = TIM_OCPolarity_High;
	TIM_OC1Init(TIM3, &oc);
	TIM_OC1PreloadConfig(TIM3, TIM_OCPreload_Enable);

	TIM_ARRPreloadConfig(TIM3, ENABLE);
	TIM_Cmd(TIM3, ENABLE);
}

