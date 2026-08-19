#include "key.h"
#include "motor_ctrl.h"
#include "factory_reset.h"
#include "ft32f0xx.h"
#include "ft32f0xx_gpio.h"
#include "ft32f0xx_rcc.h"
#include "mcu_api.h"
#include "buzzer.h"
#include "log.h"

/* 按键GPIO定义 */
#define KEY1_GPIO_PORT GPIOB
#define KEY1_GPIO_PIN GPIO_Pin_6
#define KEY1_GPIO_CLK RCC_AHBPeriph_GPIOB

#define KEY2_GPIO_PORT GPIOA
#define KEY2_GPIO_PIN GPIO_Pin_7
#define KEY2_GPIO_CLK RCC_AHBPeriph_GPIOA

/**
 * @brief 初始化按键GPIO
 */
void Key_Init(void)
{
	GPIO_InitTypeDef gpio_init;

	/* 使能GPIO时钟 */
	RCC_AHBPeriphClockCmd(KEY1_GPIO_CLK | KEY2_GPIO_CLK, ENABLE);

	/* 配置按键GPIO为上拉输入 */
	GPIO_StructInit(&gpio_init);
	gpio_init.GPIO_Mode = GPIO_Mode_IN;
	gpio_init.GPIO_PuPd = GPIO_PuPd_UP;

	/* 按键1：PB6 */
	gpio_init.GPIO_Pin = KEY1_GPIO_PIN;
	GPIO_Init(KEY1_GPIO_PORT, &gpio_init);

	/* 按键2：PA7 */
	gpio_init.GPIO_Pin = KEY2_GPIO_PIN;
	GPIO_Init(KEY2_GPIO_PORT, &gpio_init);
}

/**
 * @brief 读取按键状态（原始读取，不进行消抖）
 */
static KeyState_t Key_Read(KeyNum_t key_num)
{
	uint8_t pin_state = 0U;

	if (key_num == KEY_1)
	{
		/* 按键1：PB6，上拉输入，按下为低电平 */
		pin_state = GPIO_ReadInputDataBit(KEY1_GPIO_PORT, KEY1_GPIO_PIN);
		return (pin_state == Bit_RESET) ? KEY_STATE_PRESSED : KEY_STATE_RELEASED;
	}
	else if (key_num == KEY_2)
	{
		/* 按键2：PA7，上拉输入，按下为低电平 */
		pin_state = GPIO_ReadInputDataBit(KEY2_GPIO_PORT, KEY2_GPIO_PIN);
		return (pin_state == Bit_RESET) ? KEY_STATE_PRESSED : KEY_STATE_RELEASED;
	}

	return KEY_STATE_RELEASED;
}

/**
 * @brief 检查按键是否按下（原始读取）
 */
static uint8_t Key_IsPressed(KeyNum_t key_num)
{
	return (Key_Read(key_num) == KEY_STATE_PRESSED) ? 1U : 0U;
}

/* 按键1控制任务 */
void key1_control(void)
{
	/* 按键1(PB6)长按5秒后电机正转1格 */
	static uint32_t press_start_ms = 0U;
	static uint8_t triggered = 0U;
	static uint8_t last_key_state = 0U;

	/* 使用按键模块读取按键状态 */
	uint8_t key_down = Key_IsPressed(KEY_1);

	/* 检测按键按下边沿 */
	if (key_down && !last_key_state)
	{
		// Buzzer_Play(1U, 100U, 0U);
		press_start_ms = Systick_Tick_GetMs();
		triggered = 0U;
	}

	/* 检测按键释放边沿 */
	if (!key_down && last_key_state)
	{
		press_start_ms = 0U;
		triggered = 0U;
	}

	/* 如果按键持续按下且未触发，检查是否达到5秒 */
	if (key_down && !triggered && press_start_ms != 0U)
	{
		if (Systick_Tick_IsTimeout(press_start_ms, 3000U))
		{
			if (Battery_GetPercentage() < 10 || Battery_GetCriticalLowStarted())
			{
				LOG_DEBUG("Battery too low to run motor cycle");
			}
			else
			{
				Buzzer_Play(1U, 100U, 0U);
				triggered = 1U;
				// 控制电机旋转1圈
				Motor_RunOneCycle();
			}
		}
	}

	last_key_state = key_down;
}

/* 按键2控制任务 */
void key2_control(void)
{
	/* 按键2(PA7)长按5秒后执行动作 */
	static uint32_t press_start_ms = 0U;
	static uint8_t triggered = 0U;
	static uint8_t last_key_state = 0U;

	/* 使用按键模块读取按键状态 */
	uint8_t key_down = Key_IsPressed(KEY_2);

	/* 检测按键按下边沿 */
	if (key_down && !last_key_state)
	{
		press_start_ms = Systick_Tick_GetMs();
		triggered = 0U;
	}

	/* 检测按键释放边沿 */
	if (!key_down && last_key_state)
	{
		press_start_ms = 0U;
		triggered = 0U;
	}

	/* 如果按键持续按下且未触发，检查是否达到5秒 */
	if (key_down && !triggered && press_start_ms != 0U)
	{
		if (Systick_Tick_IsTimeout(press_start_ms, 3000U))
		{
			Buzzer_Play(1U, 100U, 0U);
			triggered = 1U;
			/* 按键2长按5秒：恢复出厂设置 */
			Factory_Reset(0);
			mcu_set_wifi_mode(1);
			mcu_reset_wifi();
		}
	}

	last_key_state = key_down;
}
