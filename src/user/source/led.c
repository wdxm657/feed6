#include "led.h"
#include "ft32f0xx.h"
#include "ft32f0xx_gpio.h"
#include "ft32f0xx_rcc.h"
#include "main.h"
#include "wifi.h"
#include "battery_monitor.h"
#include "gpio_init.h"
#include "systick_tick.h"
#include "log.h"
/* LED GPIO定义 */
#define LED_CHARGE_RED_GPIO_PORT GPIOA
#define LED_CHARGE_RED_GPIO_PIN GPIO_Pin_2
#define LED_CHARGE_RED_GPIO_CLK RCC_AHBPeriph_GPIOA

#define LED_CHARGE_GREEN_GPIO_PORT GPIOB
#define LED_CHARGE_GREEN_GPIO_PIN GPIO_Pin_7
#define LED_CHARGE_GREEN_GPIO_CLK RCC_AHBPeriph_GPIOB

#define LED_BLUE_GPIO_PORT GPIOA
#define LED_BLUE_GPIO_PIN GPIO_Pin_8
#define LED_BLUE_GPIO_CLK RCC_AHBPeriph_GPIOA

#define LED_RED_GPIO_PORT GPIOA
#define LED_RED_GPIO_PIN GPIO_Pin_11
#define LED_RED_GPIO_CLK RCC_AHBPeriph_GPIOA

/* LED为低电平有效（低电平点亮） */
#define LED_ON 0U
#define LED_OFF 1U

/**
 * @brief 初始化LED GPIO
 */
void LED_Init(void)
{
	GPIO_InitTypeDef gpio_init;

	/* 使能GPIO时钟 */
	RCC_AHBPeriphClockCmd(LED_CHARGE_RED_GPIO_CLK | LED_CHARGE_GREEN_GPIO_CLK |
							  LED_BLUE_GPIO_CLK | LED_RED_GPIO_CLK,
						  ENABLE);

	/* 配置LED GPIO为推挽输出 */
	GPIO_StructInit(&gpio_init);
	gpio_init.GPIO_Mode = GPIO_Mode_OUT;
	gpio_init.GPIO_OType = GPIO_OType_PP;
	gpio_init.GPIO_Speed = GPIO_Speed_2MHz;
	gpio_init.GPIO_PuPd = GPIO_PuPd_NOPULL;

	/* 充电指示红色LED：PA2 */
	gpio_init.GPIO_Pin = LED_CHARGE_RED_GPIO_PIN;
	GPIO_Init(LED_CHARGE_RED_GPIO_PORT, &gpio_init);

	/* 充电指示绿色LED：PB7 */
	gpio_init.GPIO_Pin = LED_CHARGE_GREEN_GPIO_PIN;
	GPIO_Init(LED_CHARGE_GREEN_GPIO_PORT, &gpio_init);

	/* 普通蓝色LED：PA8 */
	gpio_init.GPIO_Pin = LED_BLUE_GPIO_PIN;
	GPIO_Init(LED_BLUE_GPIO_PORT, &gpio_init);

	/* 普通红色LED：PA11 */
	gpio_init.GPIO_Pin = LED_RED_GPIO_PIN;
	GPIO_Init(LED_RED_GPIO_PORT, &gpio_init);

	/* 初始化LED状态 */
	LED_Off(LED_BLUE);
	LED_Off(LED_RED);
	LED_Off(LED_CHARGE_RED);
	LED_Off(LED_CHARGE_GREEN);
}

/**
 * @brief 设置LED状态
 */
void LED_Set(LedNum_t led_num, uint8_t on)
{
	uint8_t pin_state = (on != 0U) ? LED_ON : LED_OFF;

	switch (led_num)
	{
	case LED_CHARGE_RED:
		if (pin_state == LED_ON)
		{
			GPIO_ResetBits(LED_CHARGE_RED_GPIO_PORT, LED_CHARGE_RED_GPIO_PIN);
		}
		else
		{
			GPIO_SetBits(LED_CHARGE_RED_GPIO_PORT, LED_CHARGE_RED_GPIO_PIN);
		}
		break;

	case LED_CHARGE_GREEN:
		if (pin_state == LED_ON)
		{
			GPIO_ResetBits(LED_CHARGE_GREEN_GPIO_PORT, LED_CHARGE_GREEN_GPIO_PIN);
		}
		else
		{
			GPIO_SetBits(LED_CHARGE_GREEN_GPIO_PORT, LED_CHARGE_GREEN_GPIO_PIN);
		}
		break;

	case LED_BLUE:
		if (pin_state == LED_ON)
		{
			GPIO_ResetBits(LED_BLUE_GPIO_PORT, LED_BLUE_GPIO_PIN);
		}
		else
		{
			GPIO_SetBits(LED_BLUE_GPIO_PORT, LED_BLUE_GPIO_PIN);
		}
		break;

	case LED_RED:
		if (pin_state == LED_ON)
		{
			GPIO_ResetBits(LED_RED_GPIO_PORT, LED_RED_GPIO_PIN);
		}
		else
		{
			GPIO_SetBits(LED_RED_GPIO_PORT, LED_RED_GPIO_PIN);
		}
		break;

	default:
		break;
	}
}

/**
 * @brief 点亮LED
 */
void LED_On(LedNum_t led_num)
{
	LED_Set(led_num, 1U);
}

/**
 * @brief 熄灭LED
 */
void LED_Off(LedNum_t led_num)
{
	LED_Set(led_num, 0U);
}

/**
 * @brief 初始化所有LED为熄灭状态
 */
void LED_InitAllOff(void)
{
	GPIO_SetBits(LED_CHARGE_RED_GPIO_PORT, LED_CHARGE_RED_GPIO_PIN);
	GPIO_SetBits(LED_CHARGE_GREEN_GPIO_PORT, LED_CHARGE_GREEN_GPIO_PIN);
	GPIO_SetBits(LED_BLUE_GPIO_PORT, LED_BLUE_GPIO_PIN);
	GPIO_SetBits(LED_RED_GPIO_PORT, LED_RED_GPIO_PIN);
}

/* WIFI状态LED控制相关 */
#define WIFI_LED_BLINK_FAST_MS 200U	 /* 快闪间隔：200ms */
#define WIFI_LED_BLINK_SLOW_MS 1000U /* 慢闪间隔：1000ms */
static uint8_t s_wifi_led_state = 0U;
static uint32_t s_wifi_led_last_toggle_ms = 0U;
static uint8_t s_wifi_last_state = 0xFF;

/* 电源指示灯控制相关 */
#define POWER_LED_BLINK_MS 500U /* 电源指示灯闪烁间隔：500ms */
static uint8_t s_power_led_toggle_state = 0U;
static uint32_t s_power_led_last_toggle_ms = 0U;

/**
 * @brief WIFI状态LED控制任务
 * @note 根据WIFI状态控制普通蓝灯：
 *       - SMART_CONFIG_STATE: LED快闪（200ms间隔）
 *       - AP_STATE: LED慢闪（1000ms间隔）
 *       - WIFI_NOT_CONNECTED: LED常暗（熄灭）
 *       - WIFI_CONNECTED: LED常亮
 */
void LED_WifiStatusControl(void)
{
	uint8_t wifi_state = mcu_get_wifi_work_state();
	static uint8_t last_wifi_state = 0;
	uint32_t current_ms = Systick_Tick_GetMs();
	uint8_t close_power_mode = Battery_GetClosePowerMode();

	/* 保护机制：如果曾经进入过低电量，且USB从未插入过，则可能是AD采样突变，保持低电量LED状态 */
	uint8_t ever_low_power = Battery_GetEverLowPower();
	uint8_t usb_ever_inserted = Battery_GetUsbEverInserted();

	//  WIFI断电
	if (GPIO_ReadInputDataBit(GPIOB, GPIO_Pin_1) == 0U)
	{
		LED_Off(LED_BLUE);
		s_wifi_last_state = 0xFF;
		return;
	}
	/* 如果曾经进入过低电量且USB从未插入过，可能是AD采样突变，保持低电量LED状态 */
	if ((ever_low_power != 0U || Battery_GetCriticalLowStarted() != 0U) && usb_ever_inserted == 0U && Get_USB_Flag() == 0U)
	{
		LED_Off(LED_BLUE);
		s_wifi_last_state = 0xFF;
		return;
	}

	if (last_wifi_state != wifi_state)
	{
		last_wifi_state = wifi_state;
		/* 如果WIFI状态改变，立即更新LED状态 */
		s_wifi_led_last_toggle_ms = current_ms;

		switch (wifi_state)
		{
		case SMART_CONFIG_STATE:
			/* Smart配置状态：LED快闪 */
			s_wifi_led_state = 1U;
			LED_On(LED_BLUE);
			break;

		case AP_STATE:
			/* AP配置状态：LED慢闪 */
			s_wifi_led_state = 1U;
			LED_On(LED_BLUE);
			break;

		case WIFI_NOT_CONNECTED:
			/* WIFI未连接：LED常暗 */
			s_wifi_led_state = 0U;
			LED_Off(LED_BLUE);
			break;

		case WIFI_CONNECTED:
			/* WIFI已连接：LED常亮 */
			mcu_get_system_time();
			s_wifi_led_state = 1U;
			LED_On(LED_BLUE);
			break;
		case WIFI_CONN_CLOUD:
			/* WIFI连接上云：LED常亮 */
			mcu_get_system_time();
			s_wifi_led_state = 1U;
			LED_On(LED_BLUE);
			break;
		default:
			/* 未知状态：LED熄灭 */
			s_wifi_led_state = 0U;
			LED_Off(LED_BLUE);
			break;
		}
		return;
	}
	/* 根据状态处理闪烁 */
	switch (wifi_state)
	{
	case SMART_CONFIG_STATE:
		/* Smart配置状态：LED快闪（200ms间隔） */
		if (Systick_Tick_IsTimeout(s_wifi_led_last_toggle_ms, WIFI_LED_BLINK_FAST_MS))
		{
			s_wifi_led_last_toggle_ms = current_ms;
			s_wifi_led_state = !s_wifi_led_state;
			if (s_wifi_led_state)
			{
				LED_On(LED_BLUE);
			}
			else
			{
				LED_Off(LED_BLUE);
			}
		}
		break;

	case AP_STATE:
		/* AP配置状态：LED慢闪（1000ms间隔） */
		if (Systick_Tick_IsTimeout(s_wifi_led_last_toggle_ms, WIFI_LED_BLINK_SLOW_MS))
		{
			s_wifi_led_last_toggle_ms = current_ms;
			s_wifi_led_state = !s_wifi_led_state;
			if (s_wifi_led_state)
			{
				LED_On(LED_BLUE);
			}
			else
			{
				LED_Off(LED_BLUE);
			}
		}
		break;
		// case WIFI_NOT_CONNECTED:
		// 	/* WIFI未连接：LED常暗（保持熄灭） */
		// 	/* 状态改变时已处理，这里不需要额外操作 */
		// 	break;

		// case WIFI_CONNECTED:
		// 	/* WIFI已连接：LED常亮（保持点亮） */
		// 	/* 状态改变时已处理，这里不需要额外操作 */
		// 	break;

	default:
		/* 未知状态：LED熄灭 */
		// LED_Off(LED_BLUE);
		break;
	}
}

/**
 * @brief 电源/电量指示灯控制任务
 *
 * 优先使用充电状态：
 *  - USB 插入且 CHRG=0：认为“充电中”，红灯闪烁，绿灯关闭
 *  - USB 插入且 STDBY=0：认为“充满电”，绿灯常亮，红灯关闭
 *
 * 其他情况依据电量百分比：
 *  - USB 拔出且电量 >= 50%：红绿灯交替闪烁（500ms），视觉为黄灯闪烁
 *  - USB 拔出且 30% <= 电量 < 50%：红灯常亮，绿灯关闭
 *  - 电量 < 30%：红绿灯都关闭
 */
void LED_PowerStatusControl(void)
{
	static int last_battery_percent = 0;
	uint8_t battery_percent = Battery_GetPercentage();
	uint8_t usb_inserted = Get_USB_Flag();
	uint32_t current_ms = Systick_Tick_GetMs();
	/* 先处理 USB 插入的情况，根据 TP4056 CHRG/STDBY 状态判定 */
	if (usb_inserted != 0U)
	{
		uint8_t stby = 1U;
		uint8_t chrg = 1U;
		ChargeSignals_ReadFiltered(&stby, &chrg);

		/* CHRG 低电平表示正在充电 */
		if (chrg == 0U)
		{
			/* 充电中：红灯闪烁，绿灯关闭 */
			if (Systick_Tick_IsTimeout(s_power_led_last_toggle_ms, POWER_LED_BLINK_MS))
			{
				s_power_led_last_toggle_ms = current_ms;
				s_power_led_toggle_state = !s_power_led_toggle_state;
			}

			if (s_power_led_toggle_state)
			{
				LED_On(LED_CHARGE_RED);
			}
			else
			{
				LED_Off(LED_CHARGE_RED);
			}
			LED_Off(LED_CHARGE_GREEN);
			return;
		}

		/* STDBY 低电平表示充满电 */
		if (stby == 0U)
		{
			/* 充满电：绿灯常亮，红灯关闭 */
			LED_On(LED_CHARGE_GREEN);
			LED_Off(LED_CHARGE_RED);
			return;
		}

		/* 其余 USB 插入但既不充电也未满的情况：默认都关闭 */
		// LED_Off(LED_CHARGE_RED);
		// LED_Off(LED_CHARGE_GREEN);
		// mcu_dp_fault_update(DPID_FAULT, 0U);
		return;
	}
}
