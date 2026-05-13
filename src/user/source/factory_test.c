#include "factory_test.h"

#include "battery_monitor.h"
#include "buzzer.h"
#include "factory_wifi_test_status.h"
#include "ft32f0xx_exti.h"
#include "ft32f0xx_gpio.h"
#include "led.h"
#include "log.h"
#include "motor_ctrl.h"
#include "systick_tick.h"

#include "mcu_api.h"

typedef enum
{
	FACTORY_TEST_STATE_BOOT_SLEEP_WAIT_LIMIT = 0,
	FACTORY_TEST_STATE_RUNNING
} FactoryTestState_t;

static FactoryTestState_t s_state = FACTORY_TEST_STATE_BOOT_SLEEP_WAIT_LIMIT;

static uint8_t s_battery_ok = 1U;
static uint8_t s_motor_current_ok = 1U;

static uint32_t s_led3_last_toggle_ms = 0U;
static uint8_t s_led3_toggle = 0U;

static uint32_t s_motor_phase_start_ms = 0U;
static uint8_t s_motor_phase = 0U; /* 0: forward, 1: stop gap, 2: reverse, 3: stop gap */

static uint8_t s_last_key1 = 1U;
static uint8_t s_last_key2 = 1U;

static uint8_t s_wifi_test_started = 0U;
static uint32_t s_wifi_test_start_ms = 0U;
/* 调用 mcu_start_connect_wifitest 的时刻，满 3s 后读产测 result 决定 LED1 */
static uint32_t s_wifi_connect_call_ms = 0U;
static uint8_t s_wifi_led_finalized = 0U;
/* 读取 FactoryWifiTest_GetConnectTestResult 的轮询时间戳（1s 轮询） */
static uint32_t s_wifi_get_status_sent_ms = 0U;
static uint32_t s_wifi_led1_blink_last_ms = 0U;
static uint8_t s_wifi_led1_blink_on = 0U;

static void FactoryTest_SetLed2BatteryIndicator(uint8_t ok)
{
	/* 测试.md: LED2 红灯常亮，异常熄灭 */
	if (ok != 0U)
	{
		LED_On(LED_RED);
	}
	else
	{
		LED_Off(LED_RED);
	}
}

static void FactoryTest_UpdateLed3ChargeIndicator(void)
{
	/* 测试.md: LED3 红绿灯交替闪烁，异常熄灭 */
	if (s_motor_current_ok == 0U)
	{
		LED_Off(LED_CHARGE_RED);
		LED_Off(LED_CHARGE_GREEN);
		return;
	}

	uint32_t now = Systick_Tick_GetMs();
	if (s_led3_last_toggle_ms == 0U || Systick_Tick_IsTimeout(s_led3_last_toggle_ms, 500U))
	{
		s_led3_last_toggle_ms = now;
		s_led3_toggle = (s_led3_toggle == 0U) ? 1U : 0U;

		if (s_led3_toggle != 0U)
		{
			LED_On(LED_CHARGE_RED);
			LED_Off(LED_CHARGE_GREEN);
		}
		else
		{
			LED_Off(LED_CHARGE_RED);
			LED_On(LED_CHARGE_GREEN);
		}
	}
}

static void FactoryTest_Led1WifiBlink(uint32_t now_ms, uint32_t off_ms)
{
	/* 测试.md: WIFI 未连接时 LED1 按固定频率闪烁 */
	if (s_wifi_led1_blink_last_ms == 0U || Systick_Tick_IsTimeout(s_wifi_led1_blink_last_ms, off_ms) != 0U)
	{
		s_wifi_led1_blink_last_ms = now_ms;
		s_wifi_led1_blink_on = (s_wifi_led1_blink_on == 0U) ? 1U : 0U;
		if (s_wifi_led1_blink_on != 0U)
		{
			LED_On(LED_BLUE);
		}
		else
		{
			LED_Off(LED_BLUE);
		}
	}
}

static void FactoryTest_UpdateWifiTest(void)
{
	uint32_t now = Systick_Tick_GetMs();

	/* 测试.md: 前 5 秒为初始化阶段，LED1 闪烁；满 5 秒后发起连接 12345678/12345678 */
	if (s_wifi_test_started == 0U)
	{
		if (s_wifi_test_start_ms == 0U)
		{
			s_wifi_test_start_ms = now;
		}

		if (Systick_Tick_IsTimeout(s_wifi_test_start_ms, 5000U) == 0U)
		{
			FactoryTest_Led1WifiBlink(now, 1000);
			return;
		}

		// unsigned char ssid[] = "AFP(2.4G)";
		// unsigned char pwd[] = "87850216";
		unsigned char ssid[] = "12345678";
		unsigned char pwd[] = "12345678";
		mcu_start_connect_wifitest(ssid, pwd);
		s_wifi_test_started = 1U;
		s_wifi_connect_call_ms = now;
		s_wifi_led_finalized = 0U;
		s_wifi_get_status_sent_ms = 0U;
		LOG_DEBUG("");
		LOG_DEBUG("FactoryTest: start wifi connect test cmd 0x%x", WIFI_CONNECT_TEST_CMD);
		return;
	}

	/* 连接命令发出后：每 1s 轮询 result，result=0x01 后继续看 wifi status */
	if (s_wifi_led_finalized == 0U)
	{
		/* 15s 超时仍未拿到连接状态 0x03，判定失败 */
		if (Systick_Tick_IsTimeout(s_wifi_connect_call_ms, 15000U) != 0U)
		{
			LED_Off(LED_BLUE);
			s_wifi_led_finalized = 1U;
			return;
		}

		if (s_wifi_get_status_sent_ms == 0U || Systick_Tick_IsTimeout(s_wifi_get_status_sent_ms, 1000U) != 0U)
		{
			LOG_DEBUG("get wifi statu");
			uint8_t has_result = 0U;
			uint8_t result = FactoryWifiTest_GetConnectTestResult(&has_result);
			s_wifi_get_status_sent_ms = now;

			/* result=0x01 表示路由信息接收成功，继续等待模块自动更新 wifi status */
			if (has_result != 0U && result == 0x01)
			{
				uint8_t wifi_status = 0;
				wifi_status = mcu_get_wifi_work_state();
				if (wifi_status == 0x03)
				{
					LOG_DEBUG("wifi connect success");
					LED_On(LED_BLUE);
					s_wifi_led_finalized = 1U;
					return;
				}
			}
			else if (has_result != 0U && result != 0x01)
			{
				/* result 明确失败，直接关灯结束 */
				LOG_DEBUG("wifi connect failed");
				LED_Off(LED_BLUE);
				s_wifi_led_finalized = 1U;
				return;
			}
		}

		/* 等待期间保持闪烁 */
		FactoryTest_Led1WifiBlink(now, 200);
		return;
	}
}

static void FactoryTest_UpdateKeysAndBuzzer(void)
{
	/* PB6/PA7 上拉输入，按下为低电平 */
	uint8_t key1 = GPIO_ReadInputDataBit(GPIOB, GPIO_Pin_6);
	uint8_t key2 = GPIO_ReadInputDataBit(GPIOA, GPIO_Pin_7);

	if ((key1 == Bit_RESET) && (s_last_key1 != Bit_RESET))
	{
		Buzzer_Play(1U, 100U, 0U);
	}
	if ((key2 == Bit_RESET) && (s_last_key2 != Bit_RESET))
	{
		Buzzer_Play(1U, 100U, 0U);
	}

	s_last_key1 = key1;
	s_last_key2 = key2;
}

static void FactoryTest_UpdateBatteryCheck(void)
{
	/* 电压采集稳定后再判断，避免上电初期抖动 */
	static uint32_t first_check_ms = 0U;
	if (first_check_ms == 0U)
	{
		first_check_ms = Systick_Tick_GetMs();
		return;
	}

	if (Systick_Tick_IsTimeout(first_check_ms, 1500U) == 0U)
	{
		return;
	}

	uint16_t v_adc_mv = Battery_GetVoltageFiltered();
	float v_bat_mv = (float)v_adc_mv / 0.26f;

	/* 测试.md: 固定电压 3.7V，靠芯片判断准确性。这里给出较宽容差，便于产测判定 */
	if (v_bat_mv >= 3650.0f && v_bat_mv <= 3750.0f)
	{
		s_battery_ok = 1U;
	}
	else
	{
		s_battery_ok = 0U;
	}

	FactoryTest_SetLed2BatteryIndicator(s_battery_ok);
}

static void FactoryTest_UpdateMotorAndCurrentCheck(void)
{
	uint32_t now = Systick_Tick_GetMs();

	if (s_motor_phase_start_ms == 0U)
	{
		s_motor_phase_start_ms = now;
		s_motor_phase = 0U;
		Motor_StartForward(5000U);
		return;
	}

	switch (s_motor_phase)
	{
	case 0U: /* forward */
		/* 启动浪涌大：本相位开始至少 1s 后再判电流；测试.md 20~150mA */
		if (Systick_Tick_IsTimeout(s_motor_phase_start_ms, 1000U) != 0U)
		{
			uint16_t current_ma = Motor_GetCurrentFiltered();
			if ((current_ma < 20U) || (current_ma > 150U))
			{
				s_motor_current_ok = 0U;
			}
		}
		if (Systick_Tick_IsTimeout(s_motor_phase_start_ms, 5000U))
		{
			Motor_Stop();
			s_motor_phase = 1U;
			s_motor_phase_start_ms = now;
		}
		break;
	case 1U: /* stop gap */
		if (Systick_Tick_IsTimeout(s_motor_phase_start_ms, 500U))
		{
			Motor_StartReverse(5000U);
			s_motor_phase = 2U;
			s_motor_phase_start_ms = now;
		}
		break;
	case 2U: /* reverse */
		if (Systick_Tick_IsTimeout(s_motor_phase_start_ms, 1000U) != 0U)
		{
			uint16_t current_ma = Motor_GetCurrentFiltered();
			if ((current_ma < 20U) || (current_ma > 150U))
			{
				s_motor_current_ok = 0U;
			}
		}
		if (Systick_Tick_IsTimeout(s_motor_phase_start_ms, 5000U))
		{
			Motor_Stop();
			s_motor_phase = 3U;
			s_motor_phase_start_ms = now;
		}
		break;
	default: /* stop gap，结束后进入下一轮正反转 */
		if (Systick_Tick_IsTimeout(s_motor_phase_start_ms, 500U))
		{
			/* 下一轮完整周期：清空电流异常标志，重新检测 */
			s_motor_current_ok = 1U;
			Motor_StartForward(5000U);
			s_motor_phase = 0U;
			s_motor_phase_start_ms = now;
		}
		break;
	}
}

static void FactoryTest_BootSleepWaitLimit(void)
{
	/* 测试.md: 上电先进入低功耗模式，然后触发限位开关信号后唤醒开始运行 */
	if (GPIO_ReadInputDataBit(GPIOB, GPIO_Pin_3) == Bit_RESET)
	{
		s_state = FACTORY_TEST_STATE_RUNNING;
		return;
	}

	// LOG_DEBUG("FactoryTest: enter sleep, wait limit switch(PB3)");

	/* 关闭除WIFI指示外的灯，降低功耗并方便观察唤醒 */
	LED_Off(LED_CHARGE_RED);
	LED_Off(LED_CHARGE_GREEN);
	LED_Off(LED_RED);

	/* 关掉SysTick，避免周期中断把CPU频繁唤醒 */
	Systick_Tick_Stop();

	EXTI_ClearITPendingBit(EXTI_Line3);
	__enable_irq();

	while (GPIO_ReadInputDataBit(GPIOB, GPIO_Pin_3) != Bit_RESET)
	{
		__WFI();
	}

	Systick_Tick_Resume();

	s_state = FACTORY_TEST_STATE_RUNNING;
	// LOG_DEBUG("FactoryTest: woken by limit switch, start running");
}

void FactoryTest_Init(void)
{
	/* 产测：PB3 限位仅用于唤醒进入产测，不参与电机停机/周期逻辑 */
	Motor_SetLimitSwitchFeedbackEnabled(0U);

	s_state = FACTORY_TEST_STATE_BOOT_SLEEP_WAIT_LIMIT;
	s_battery_ok = 1U;
	s_motor_current_ok = 1U;

	s_led3_last_toggle_ms = 0U;
	s_led3_toggle = 0U;

	s_motor_phase_start_ms = 0U;
	s_motor_phase = 0U;

	s_last_key1 = GPIO_ReadInputDataBit(GPIOB, GPIO_Pin_6);
	s_last_key2 = GPIO_ReadInputDataBit(GPIOA, GPIO_Pin_7);

	s_wifi_test_started = 0U;
	s_wifi_test_start_ms = 0U;
	s_wifi_connect_call_ms = 0U;
	s_wifi_led_finalized = 0U;
	s_wifi_get_status_sent_ms = 0U;
	s_wifi_led1_blink_last_ms = 0U;
	s_wifi_led1_blink_on = 0U;

	// LOG_DEBUG("FactoryTest_Init: PA9 strap entered");
}

void FactoryTest_Task(void)
{
	if (s_state == FACTORY_TEST_STATE_BOOT_SLEEP_WAIT_LIMIT)
	{
		FactoryTest_BootSleepWaitLimit();
		return;
	}
	/* 工厂测试下强制给WIFI供电，便于观测连接指示 */
	GPIO_SetBits(GPIOB, GPIO_Pin_1);
	FactoryTest_UpdateBatteryCheck();
	FactoryTest_UpdateMotorAndCurrentCheck();
	FactoryTest_UpdateLed3ChargeIndicator();
	FactoryTest_UpdateWifiTest();
	FactoryTest_UpdateKeysAndBuzzer();
}
