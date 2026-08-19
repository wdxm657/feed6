#include "battery_monitor.h"
#include "adc_init.h"
#include "systick_tick.h"
#include "led.h"
#include "ft32f0xx.h"
#include "ft32f0xx_gpio.h"
#include "ft32f0xx_pwr.h"
#include "ft32f0xx_rcc.h"
#include "ft32f0xx_flash.h"
#include "ft32f0xx_exti.h"
#include "ft32f0xx_rtc.h"
#include "log.h"
#include "task_scheduler.h"
#include "motor_ctrl.h"
#include "pwm_init.h"
#include "iwdg_init.h"

/* 电池电压阈值定义（单位：mV） */
#define BATTERY_VOLTAGE_LOW_THRESHOLD 3000U /* 3.0V，电量不足阈值 */

/* 电池电压-电量百分比对应表（单位：mV） */
typedef struct
{
	uint16_t voltage_mv; /* 电压（mV） */
	uint8_t percentage;	 /* 电量百分比 */
} BatteryVoltageTable_t;

/* 电压-百分比对应表（按电压从高到低排序） */
static const BatteryVoltageTable_t s_voltage_table[] = {
	{4200, 100}, /* 100%----4.20V */
	{4060, 90},	 /* 90%-----4.06V */
	{3980, 80},	 /* 80%-----3.98V */
	{3920, 70},	 /* 70%-----3.92V */
	{3870, 60},	 /* 60%-----3.87V */
	{3820, 50},	 /* 50%-----3.82V */
	{3790, 40},	 /* 40%-----3.79V */
	{3770, 30},	 /* 30%-----3.77V */
	{3740, 20},	 /* 20%-----3.74V */
	{3680, 10},	 /* 10%-----3.68V */
	{3450, 5},	 /* 5%------3.45V */
	{3000, 0}	 /* 0%------3.00V */
};

#define VOLTAGE_TABLE_SIZE (sizeof(s_voltage_table) / sizeof(s_voltage_table[0]))

/* ADC配置 */
#define ADC_VOLTAGE_MV_PER_COUNT 0.00076f /* ADC电压每计数（mV） */
#define ADC_VOLTAGE_MV_OFFSET 0.0524f	  /* ADC电压偏移（mV） */

/* 电池电压分压比例（需要根据实际电路调整）
 * 假设：电池电压通过分压电阻连接到PA0
 * 如果分压比例为2:1，则实际电压 = ADC电压 * 2
 */
#define BATTERY_VOLTAGE_DIVIDER_RATIO 0.263157 /* 分压比例，需要根据实际电路调整 */

/* 电池状态检测相关 */
#define CHARGING_CHECK_INTERVAL_MS 100U /* 充电状态检测间隔（毫秒） */
#define VOLTAGE_FILTER_SAMPLES 10U		/* 电压滤波采样次数 */

/* 电池状态 */
static uint16_t s_battery_voltage_mv = 0U;
static uint8_t s_battery_percentage = 0U;
static uint32_t s_last_update_ms = 0U;

/* ADC滤波相关 */
#define BATTERY_ADC_FILTER_SIZE 8U /* 均值滤波窗口大小 */
static volatile uint16_t s_battery_adc_buffer[BATTERY_ADC_FILTER_SIZE] = {0U};
static volatile uint8_t s_battery_adc_index = 0U;
static volatile uint8_t s_battery_adc_count = 0U;			 /* 已填充的样本数 */
static volatile uint16_t s_battery_voltage_filtered_mv = 0U; /* 滤波后的电压值（mV） */

/* 低功耗模式状态 */
static volatile uint8_t s_low_power_mode = 0U;			/* 低功耗模式标志 */
#define LOW_POWER_EXIT_PROTECTION_MS 3000U				/* 退出低功耗模式后的保护时间：5秒 */
static volatile uint32_t s_low_power_exit_time_ms = 0U; /* 退出低功耗模式的时间戳 */

static volatile uint8_t s_close_power_mode = 0U; /* 关闭电源模式标志 */
static uint8_t s_non_charge_exti_disabled = 0U;
static uint8_t s_ultra_low_entered = 0U;	 /* 已进入究极低功耗的标记，避免重复30s等待和蜂鸣 */
static uint8_t s_low_power_flag = 0U;		 /* 低电量标志 */
static uint8_t s_ultra_low_flag_loaded = 0U; /* 是否已从Flash加载究极低功耗标记 */
static uint32_t s_motor_stop_time_ms = 0U;
static uint8_t s_ever_low_power = 0U;	 /* 是否曾经进入过低电量状态（低于UPPER_POWER_THRESHOLD） */
static uint8_t s_usb_ever_inserted = 0U; /* USB是否曾经插入过（用于保护机制） */
/* 关机前倒计时与蜂鸣器提示 */
#define CRITICAL_LOW_POWER_DELAY_MS 30000U /* 低电量关机倒计时：30秒 */
#define CRITICAL_LOW_BEEP_PERIOD_MS 800U   /* di-di 组合之间的停顿：800ms */
static uint8_t s_critical_low_started = 0U;
static uint32_t s_critical_low_start_ms = 0U;
static uint32_t s_critical_low_last_beep_ms = 0U;

static void Battery_DisableNonChargeExti(void);
static void Battery_RestoreNonChargeExti(void);

/* 究极低功耗标记Flash存储：独立使用0x08006800页（2KB） */
#define ULP_FLAG_FLASH_BASE 0x08006800U
#define ULP_FLAG_MAGIC 0x554C5046U /* 'ULPF' */

typedef struct
{
	uint32_t magic;
	uint32_t flag; /* 0-未进入，1-已进入究极低功耗 */
	uint32_t reserved[2];
} UlpFlagStorage_t;

static uint8_t UlpFlag_Save(uint8_t flag)
{
	FLASH_Status status;

	/* 解锁Flash */
	FLASH_Unlock();

	/* 清除所有Flash标志 */
	FLASH_ClearFlag(FLASH_FLAG_EOP | FLASH_FLAG_PGERR | FLASH_FLAG_WRPERR);

	/* 擦除页面 */
	status = FLASH_ErasePage(ULP_FLAG_FLASH_BASE);
	if (status != FLASH_COMPLETE)
	{
		FLASH_Lock();
		return 0U;
	}

	/* 写入魔数 */
	status = FLASH_ProgramWord(ULP_FLAG_FLASH_BASE, ULP_FLAG_MAGIC);
	if (status != FLASH_COMPLETE)
	{
		FLASH_Lock();
		return 0U;
	}

	/* 写入标志 */
	status = FLASH_ProgramWord(ULP_FLAG_FLASH_BASE + 4U, (uint32_t)flag);

	/* 锁定Flash */
	FLASH_Lock();

	return (status == FLASH_COMPLETE) ? 1U : 0U;
}

static void UlpFlag_Load(void)
{
	if (s_ultra_low_flag_loaded != 0U)
	{
		return;
	}

	UlpFlagStorage_t *storage = (UlpFlagStorage_t *)ULP_FLAG_FLASH_BASE;
	if (storage->magic == ULP_FLAG_MAGIC)
	{
		s_ultra_low_entered = (storage->flag == 1U) ? 1U : 0U;
	}
	s_ultra_low_flag_loaded = 1U;
}

/**
 * @brief 将ADC原始值转换为电压（mV）
 * @param adc_raw ADC原始值
 * @param usb_flag USB插入标志
 * @return 电池电压（mV）
 */
static uint16_t Battery_AdcToVoltage(uint16_t adc_raw)
{
	/* 计算ADC对应的电压（mV） */
	/* ADC电压 = (ADC值 / ADC最大值) * 参考电压 */
	// 点 (1339|1.07); 点 (1510|1.2);
	// float adc_voltage_mv = (float)adc_raw * ADC_VOLTAGE_MV_PER_COUNT + ADC_VOLTAGE_MV_OFFSET;
	float adc_voltage_mv = (float)adc_raw * 2500.0f / 0xFFF;
	return (uint16_t)(adc_voltage_mv);
}

/**
 * @brief 根据电压值计算电量百分比（分段线性插值）
 * @param voltage_mv 电池电压（mV）
 * @return 电量百分比（0-100）
 */
static uint8_t Battery_CalculatePercentage(uint16_t voltage_mv)
{
	/* 电压超出上限，返回100% */
	if (voltage_mv >= s_voltage_table[0].voltage_mv)
	{
		return 100U;
	}

	/* 电压低于下限，返回0% */
	if (voltage_mv <= s_voltage_table[VOLTAGE_TABLE_SIZE - 1].voltage_mv)
	{
		return 0U;
	}

	/* 在表格中查找对应的区间进行线性插值 */
	for (uint8_t i = 0; i < VOLTAGE_TABLE_SIZE - 1; i++)
	{
		uint16_t v_high = s_voltage_table[i].voltage_mv;
		uint16_t v_low = s_voltage_table[i + 1].voltage_mv;
		uint8_t p_high = s_voltage_table[i].percentage;
		uint8_t p_low = s_voltage_table[i + 1].percentage;

		/* 如果电压在当前区间内 */
		if (voltage_mv <= v_high && voltage_mv >= v_low)
		{
			/* 线性插值计算百分比 */
			uint16_t voltage_diff = v_high - v_low;
			uint16_t voltage_offset = voltage_mv - v_low;
			uint8_t percentage_diff = p_high - p_low;

			if (voltage_diff == 0U)
			{
				return p_low;
			}

			/* 计算插值：percentage = p_low + (voltage_offset / voltage_diff) * percentage_diff */
			uint8_t percentage = p_low + (uint8_t)(((uint32_t)voltage_offset * percentage_diff) / voltage_diff);
			return percentage;
		}
	}

	/* 理论上不会执行到这里 */
	return 0U;
}

/**
 * @brief 初始化电池检测模块
 */
void Battery_Init(void)
{
	s_battery_voltage_mv = 0U;
	s_last_update_ms = 0U;
	s_battery_adc_index = 0U;
	s_battery_adc_count = 0U;
	s_battery_voltage_filtered_mv = 0U;

	/* 初始化滤波缓冲区 */
	for (uint8_t i = 0; i < BATTERY_ADC_FILTER_SIZE; i++)
	{
		s_battery_adc_buffer[i] = 0U;
	}
}

/**
 * @brief ADC读取和均值滤波任务
 * @note ADC值由DMA中断自动更新，此任务进行滤波处理
 *       需要周期调用（例如放入任务调度器），建议20ms间隔
 */
void Battery_AdcFilterTask(void)
{
	/* 读取DMA中断更新的ADC原始值 */
	uint16_t adc_raw = ADC_ReadBatteryRaw();
	static uint32_t s_last_print_ms = 0U;

	/* 将ADC值转换为电压（mV） */
	uint16_t voltage_mv = Battery_AdcToVoltage(adc_raw);
	/* 将新值存入缓冲区 */
	s_battery_adc_buffer[s_battery_adc_index] = voltage_mv;
	s_battery_adc_index = (s_battery_adc_index + 1U) % BATTERY_ADC_FILTER_SIZE;

	/* 更新已填充的样本数 */
	if (s_battery_adc_count < BATTERY_ADC_FILTER_SIZE)
	{
		s_battery_adc_count++;
	}

	/* 计算均值 */
	uint32_t sum = 0U;
	uint8_t i;
	for (i = 0U; i < s_battery_adc_count; i++)
	{
		sum += s_battery_adc_buffer[i];
	}

	/* 更新滤波后的电压值 */
	if (s_battery_adc_count > 0U)
	{
		s_battery_voltage_filtered_mv = (uint16_t)(sum / s_battery_adc_count);
	}

	// 0.1秒打印
	// if (Systick_Tick_GetMs() - s_last_print_ms >= 100U)
	// {
	// 	LOG_DEBUG("Battery_AdcFilterTask: adc_raw: %d, voltage_mv: %d", adc_raw, voltage_mv);
	// 	s_last_print_ms = Systick_Tick_GetMs();
	// }
}

/**
 * @brief 获取滤波后的电池电压值
 * @return 滤波后的电压值（mV）
 */
uint16_t Battery_GetVoltageFiltered(void)
{
	return s_battery_voltage_filtered_mv;
}

uint8_t Battery_GetClosePowerMode(void)
{
	return s_close_power_mode;
}

void Battery_SetClosePowerMode(uint8_t close_power_mode)
{
	s_close_power_mode = close_power_mode;
}

/**
 * @brief 获取是否曾经进入过低电量状态（低于UPPER_POWER_THRESHOLD）
 * @return 1-曾经进入过低电量，0-未进入过低电量
 */
uint8_t Battery_GetEverLowPower(void)
{
	return s_ever_low_power;
}

uint8_t Battery_GetCriticalLowStarted(void)
{
	return s_critical_low_started;
}

/**
 * @brief 获取USB是否曾经插入过
 * @return 1-USB曾经插入过，0-USB从未插入过
 */
uint8_t Battery_GetUsbEverInserted(void)
{
	return s_usb_ever_inserted;
}

static void Battery_DisableNonChargeExti(void)
{
	if (s_non_charge_exti_disabled != 0U)
	{
		return;
	}

	// EXTI_InitTypeDef exti_init;
	// EXTI_StructInit(&exti_init);
	// exti_init.EXTI_Mode = EXTI_Mode_Interrupt;
	// exti_init.EXTI_Trigger = EXTI_Trigger_Rising_Falling;
	// exti_init.EXTI_LineCmd = DISABLE;

	// /* 禁用 PB3 (Line3)、PB6 (Line6)、PA7 (Line7) */
	// exti_init.EXTI_Line = EXTI_Line3;
	// EXTI_Init(&exti_init);

	// exti_init.EXTI_Line = EXTI_Line6;
	// EXTI_Init(&exti_init);

	// exti_init.EXTI_Line = EXTI_Line7;
	// EXTI_Init(&exti_init);

	// /* 确保充电相关中断保持使能 */
	// exti_init.EXTI_LineCmd = ENABLE;
	// exti_init.EXTI_Line = EXTI_Line4;
	// EXTI_Init(&exti_init);

	// exti_init.EXTI_Line = EXTI_Line5;
	// EXTI_Init(&exti_init);

	/* 禁用 RTC 闹钟唤醒 */
	RTC_DeInit();

	s_non_charge_exti_disabled = 1U;
}

static void Battery_RestoreNonChargeExti(void)
{
	if (s_non_charge_exti_disabled == 0U)
	{
		return;
	}

	// EXTI_InitTypeDef exti_init;
	// EXTI_StructInit(&exti_init);
	// exti_init.EXTI_Mode = EXTI_Mode_Interrupt;
	// exti_init.EXTI_Trigger = EXTI_Trigger_Rising_Falling;
	// exti_init.EXTI_LineCmd = ENABLE;

	// exti_init.EXTI_Line = EXTI_Line3;
	// EXTI_Init(&exti_init);

	// exti_init.EXTI_Line = EXTI_Line6;
	// EXTI_Init(&exti_init);

	// exti_init.EXTI_Line = EXTI_Line7;
	// EXTI_Init(&exti_init);

	RTC_Init_All();
	s_non_charge_exti_disabled = 0U;
	/*  TODO :清除究极低功耗模式标记 */
	/**/
}

/**
 * @brief 更新电池状态（业务逻辑处理）
 * @note 需要在主循环中定期调用，建议50-500ms间隔
 */
void Battery_Update(void)
{
	/* 使用滤波后的电压值 */
	s_battery_voltage_mv = Battery_GetVoltageFiltered();

	/* 确保已从Flash加载究极低功耗标记 */
	// UlpFlag_Load();

	/* 根据电压值计算电量百分比（分段线性插值） */
	float voltage_mv_real = (float)s_battery_voltage_mv / 0.26f;
	/* 当滤波缓冲区满后，才进行业务逻辑处理 */
	if (s_battery_adc_count >= BATTERY_ADC_FILTER_SIZE)
	{
		s_battery_percentage = Battery_CalculatePercentage(voltage_mv_real);
		uint32_t systick_ms = Systick_Tick_GetMs();
		static uint32_t last_update_ms = 0U;
		static uint8_t last_battery_percentage = 0U;
		if (Get_USB_Flag() == 0)
		{
			if (systick_ms - last_update_ms >= 1000U)
			{
				LOG_DEBUG("Battery_Update: voltage_mv_real: %f, battery_percentage: %d", voltage_mv_real, s_battery_percentage);
				last_update_ms = systick_ms;
				/* 当电池百分比低于70% 关闭WIFI电源 进入低功耗模式(USB插入或按键IO中断会唤醒) */

				/* 检查是否在保护期内 且不是关机模式（退出低功耗模式后10秒内不再次进入） */
				uint8_t in_protection = 0U;
				if (s_low_power_exit_time_ms != 0U)
				{
					if (Systick_Tick_IsTimeout(s_low_power_exit_time_ms, LOW_POWER_EXIT_PROTECTION_MS) == 0U)
					{
						/* 还在保护期内，不进入低功耗模式 */
						in_protection = 1U;
					}
					else
					{
						/* 保护期已过，清除时间戳 */
						s_low_power_exit_time_ms = 0U;
					}
				}
				if (s_battery_percentage < UPPER_POWER_THRESHOLD && in_protection == 0U)
				{
					/* 低电量逻辑 */
					if (s_battery_percentage < LOWER_POWER_THRESHOLD || s_low_power_flag == 1U)
					{
						s_low_power_flag = 1U;
						if (Motor_IsRunning() == 1U)
						{
							return;
						}
						/* 已经标记为究极低功耗后，直接进入，不再等待30秒或蜂鸣 */
						if (s_ultra_low_entered != 0U)
						{
							if (Motor_IsRunning() == 0U && Systick_Tick_IsTimeout(s_motor_stop_time_ms, 2500U))
							{
								Battery_SetClosePowerMode(1U);
								Battery_EnterLowPowerMode();
								return;
							}
						}

						s_usb_ever_inserted = 0U;

						/* 进入“即将关机”状态：30 秒倒计时 + 蜂鸣器提示 */
						/* 关闭所有普通指示灯 */
						LED_InitAllOff();
						GPIO_ResetBits(GPIOB, GPIO_Pin_1);

						if (s_critical_low_started == 0U)
						{
							s_critical_low_started = 1U;
							s_critical_low_start_ms = systick_ms;
							s_critical_low_last_beep_ms = 0U;
							LOG_DEBUG("Battery_Update: entering critical low power mode");
							Battery_DisableNonChargeExti();
						}

						/* 蜂鸣器按照 di-di ----- di-di 的节奏提示（需要检查静音时段） */
						if ((s_critical_low_last_beep_ms == 0U) ||
							Systick_Tick_IsTimeout(s_critical_low_last_beep_ms, CRITICAL_LOW_BEEP_PERIOD_MS) != 0U)
						{
							if (Buzzer_IsActive() == 0U && Systick_Tick_IsTimeout(s_critical_low_start_ms, CRITICAL_LOW_POWER_DELAY_MS) == 0U)
							{
								Buzzer_PlayWithQuietHours(2U, 100U, 100U); /* di-di，需要检查静音时段 */
								s_critical_low_last_beep_ms = systick_ms;
							}
						}

						/* 倒计时到达 30 秒，进入究极低功耗模式 */
						if (Systick_Tick_IsTimeout(s_critical_low_start_ms, CRITICAL_LOW_POWER_DELAY_MS) != 0U)
						{
							/* 关闭蜂鸣器与 PWM，进入低功耗 Stop 模式 */
							s_critical_low_last_beep_ms = 0U;
							if (s_low_power_mode == 0U)
							{
								if (Motor_IsRunning() == 0U && Systick_Tick_IsTimeout(s_motor_stop_time_ms, 2500U))
								{
									if (s_ultra_low_entered == 0U)
									{
										s_ultra_low_entered = 1U; /* 标记进入究极低功耗 */
																  // UlpFlag_Save(1U);
									}
									// 红灯灭，绿灯灭
									LED_Off(LED_CHARGE_RED);
									LED_Off(LED_CHARGE_GREEN);
									Battery_EnterLowPowerMode();
								}
							}
						}
					}
					else
					{
						/* 10%~70%：普通低功耗逻辑*/
						/* 标记曾经进入过低电量状态 */

						if (s_ever_low_power != 0U && s_usb_ever_inserted == 0U)
						{
							/* 可能是AD采样突变，保持低功耗状态，不退出 */
							return;
						}
						s_usb_ever_inserted = 0U;
						s_ever_low_power = 1U;
						// Battery_SetClosePowerMode(1U);
						// 红灯亮，绿灯灭
						LED_On(LED_CHARGE_RED);
						LED_Off(LED_CHARGE_GREEN);

						// 固定上报20%让用户知道电量过低了
						if (mcu_get_wifi_work_state() == WIFI_CONNECTED || mcu_get_wifi_work_state() == WIFI_CONN_CLOUD)
						{
							mcu_dp_value_update(DPID_BATTERY_PERCENTAGE, 20);
						}
						if (s_critical_low_started != 0U)
						{
							Buzzer_Stop();

							s_critical_low_started = 0U;
							s_critical_low_start_ms = 0U;
							s_critical_low_last_beep_ms = 0U;
							Battery_RestoreNonChargeExti();
						}

						/* 进入低功耗模式 */
						if (s_low_power_mode == 0U && Systick_Tick_IsTimeout(s_motor_stop_time_ms, 2500U))
						{
							// 仅关闭WIFI，不进普通低功耗了，只有电池电量低于10%了才进入
							GPIO_ResetBits(GPIOB, GPIO_Pin_1);
							LED_Off(LED_BLUE);
							// 等待电机停止
							if (Motor_IsRunning() == 1U)
							{
								s_motor_stop_time_ms = Systick_Tick_GetMs();
							}
							else
							{
								// Battery_EnterLowPowerMode();
							}
						}
					}
				}
				else
				{
					// 电量大于70%，不用做处理。若电量低于70了，低电量部分会处理，usb插入检测会处理，usb拔出后也不处理70以上的时候。
					// /* 退出低功耗1s内的AD数据不准 */
					// if (s_low_power_exit_time_ms != 0U && Systick_Tick_IsTimeout(s_low_power_exit_time_ms, 1000U) == 0U)
					// {
					// 	return;
					// }

					// /* 保护机制：如果曾经进入过低电量，且USB从未插入过，则可能是AD采样突变，不退出低功耗 */
					if (s_ever_low_power != 0U && s_usb_ever_inserted == 0U)
					{
						/* 可能是AD采样突变，保持低功耗状态，不退出 */
						return;
					}

					// 绿灯亮，红灯灭
					LED_Off(LED_CHARGE_RED);
					LED_On(LED_CHARGE_GREEN);

					// /* 电量 >=60%：退出低功耗相关状态 */
					// if (s_critical_low_started != 0U)
					// {
					// 	Buzzer_Stop();
					// 	s_critical_low_started = 0U;
					// 	s_critical_low_start_ms = 0U;
					// 	s_critical_low_last_beep_ms = 0U;
					// 	Battery_RestoreNonChargeExti();
					// 	Battery_SetClosePowerMode(0U);
					// 	if (s_ultra_low_entered != 0U)
					// 	{
					// 		s_ultra_low_entered = 0U; /* 电量恢复，清除究极低功耗标记 */
					// 		UlpFlag_Save(0U);
					// 	}
					// }

					// if (s_low_power_mode != 0U)
					// {
					// 	/* 退出低功耗模式 */
					// 	Battery_ExitLowPowerMode();
					// }

					/* 清除低电量标记（电量已恢复且USB已插入过） */
					if (s_usb_ever_inserted != 0U)
					{
						s_ever_low_power = 0U;
					}

					GPIO_SetBits(GPIOB, GPIO_Pin_1);
				}
			}
		}
		else
		{
			/* USB插入时，标记USB曾经插入过，并清除低电量保护标记 */
			s_usb_ever_inserted = 1U;
			s_ever_low_power = 0U;

			/* 清除低功耗模式标志 */
			s_low_power_mode = 0U;
			s_low_power_flag = 0U;
			if (s_critical_low_started != 0U)
			{
				Battery_SetClosePowerMode(0U);
				Buzzer_Stop();
				s_critical_low_started = 0U;
				s_critical_low_start_ms = 0U;
				s_critical_low_last_beep_ms = 0U;
				Battery_RestoreNonChargeExti();
				if (s_ultra_low_entered != 0U)
				{
					s_ultra_low_entered = 0U; /* USB插入，清除究极低功耗标记 */
											  // UlpFlag_Save(0U);
				}
			}
		}
	}
}

/**
 * @brief 获取电池电压（mV）
 */
uint16_t Battery_GetVoltage(void)
{
	return s_battery_voltage_mv;
}

/**
 * @brief 获取电池ADC原始值
 */
uint16_t Battery_GetRawADC(void)
{
	return ADC_ReadBatteryRaw();
}

/**
 * @brief 获取电池电量百分比
 */
uint8_t Battery_GetPercentage(void)
{
	return s_battery_percentage;
}

/**
 * @brief 配置系统时钟（从Stop模式唤醒后需要重新配置）
 */
static void Battery_SystemClockConfig(void)
{
	__IO uint32_t StartUpCounter = 0U, HSIStatus = 0U;

	/* 使能HSI */
	RCC->CR |= ((uint32_t)RCC_CR_HSION);

	/* 等待HSI就绪 */
	do
	{
		HSIStatus = RCC->CR & RCC_CR_HSIRDY;
		StartUpCounter++;
	} while ((HSIStatus == 0U) && (StartUpCounter != 0x1000U));

	if ((RCC->CR & RCC_CR_HSIRDY) != RESET)
	{
		HSIStatus = 1U;
	}
	else
	{
		HSIStatus = 0U;
	}

	if (HSIStatus == 1U)
	{
		/* 使能预取指缓冲并设置Flash延迟 */
		FLASH->ACR = FLASH_ACR_PRFTBE | ((uint32_t)0x00000001);

		/* HCLK = SYSCLK */
		RCC->CFGR |= (uint32_t)RCC_CFGR_HPRE_DIV1;

		/* PCLK = HCLK */
		RCC->CFGR |= (uint32_t)RCC_CFGR_PPRE_DIV1;

		/* PLL配置 */
		RCC->CFGR &= (uint32_t)((uint32_t)~(RCC_CFGR_PLLSRC | RCC_CFGR_PLLXTPRE | RCC_CFGR_PLLMULL));
		RCC->CFGR |= (uint32_t)(RCC_CFGR_PLLSRC_HSI_PREDIV | RCC_CFGR_PLLXTPRE_PREDIV1 | RCC_CFGR_PLLMULL6);

		/* 使能PLL */
		RCC->CR |= RCC_CR_PLLON;

		/* 等待PLL就绪 */
		while ((RCC->CR & RCC_CR_PLLRDY) == 0)
		{
		}

		/* 选择PLL作为系统时钟源 */
		RCC->CFGR &= (uint32_t)((uint32_t)~(RCC_CFGR_SW));
		RCC->CFGR |= (uint32_t)RCC_CFGR_SW_PLL;

		/* 等待PLL被用作系统时钟源 */
		while ((RCC->CFGR & (uint32_t)RCC_CFGR_SWS) != (uint32_t)RCC_CFGR_SWS_PLL)
		{
		}
	}

	/* 更新系统时钟频率 */
	extern void SystemCoreClockUpdate(void);
	SystemCoreClockUpdate();
}

/**
 * @brief 进入低功耗模式（Stop模式）
 * @note USB插入（PA4/PA5中断）或按键中断会唤醒系统
 */
void Battery_EnterLowPowerMode(void)
{
	if (s_low_power_mode != 0U)
	{
		return; /* 已经在低功耗模式 */
	}

	LOG_DEBUG("Battery: Entering low power mode");

	/* 停止所有任务 */
	TaskScheduler_StopAll();

	/* 停止SysTick定时器 */
	Systick_Tick_Stop();

	/* 停止电机定时器 */
	Motor_StopTimer();

	/* 停止PWM（蜂鸣器） */
	PWM_Stop();

	/* 停止电机（如果正在运行） */
	Motor_Stop();

	/* 关闭WIFI电源 */
	GPIO_ResetBits(GPIOB, GPIO_Pin_1);
	// 关闭所有LED
	LED_Off(LED_CHARGE_RED);
	LED_Off(LED_CHARGE_GREEN);
	LED_Off(LED_BLUE);
	LED_Off(LED_RED);
	LED_Off(LED_BLUE);

	/* 停止ADC和DMA */
	ADC_Stop();
	ADC_ReadBatteryRaw_enable(0U);

	/* 使能PWR时钟（进入Stop模式前必须使能） */
	RCC_APB1PeriphClockCmd(RCC_APB1Periph_PWR, ENABLE);

	/* 进入低功耗前喂一次看门狗 */
	// IWDG_Feed();

	/* 设置低功耗模式标志 */
	s_low_power_mode = 1U;

	/* 确保唤醒源中断挂起标志被清除，避免带着旧挂起进入STOP */
	EXTI_ClearITPendingBit(EXTI_Line4 | EXTI_Line5 | EXTI_Line6 | EXTI_Line7);

	/* 进入Stop模式（低功耗模式，WFI唤醒）
	 * 注意：PA4/PA5（USB检测）和按键（PA7/PB6）的EXTI中断可以唤醒系统
	 * 注意：看门狗在Stop模式下仍然运行，如果系统在低功耗模式下停留时间超过看门狗超时时间（约8秒），系统会被复位
	 */
	LOG_DEBUG("Battery: Entering low power mode");
	__enable_irq(); /* 确保全局中断开启，否则WFI无法被EXTI唤醒 */
	PWR_EnterSTOPMode(PWR_Regulator_LowPower, PWR_STOPEntry_WFI);
	s_battery_adc_count = 0U;

	/* 唤醒后执行到这里 */
	Battery_ExitLowPowerMode();
}

/**
 * @brief 退出低功耗模式（从Stop模式唤醒后）
 */
void Battery_ExitLowPowerMode(void)
{
	if (s_low_power_mode == 0U)
	{
		return; /* 不在低功耗模式 */
	}

	LOG_DEBUG("Battery_ExitLowPowerMode: Exiting low power mode");

	// 直接重启系统
	NVIC_SystemReset();
	return;

	/* 重新配置系统时钟（Stop模式会停止系统时钟） */
	Battery_SystemClockConfig();

	/* 恢复SysTick定时器 */
	Systick_Tick_Resume();

	/* 恢复电机定时器 */
	Motor_ResumeTimer();

	/* 恢复ADC和DMA */
	ADC_ReadBatteryRaw_enable(1U);
	ADC_Resume();

	s_battery_adc_count = 0U;

	/* 恢复所有任务 */
	TaskScheduler_ResumeAll();

	/* 退出低功耗后立即喂一次看门狗 */
	// IWDG_Feed();

	/* 记录退出低功耗模式的时间戳，用于10秒保护期 */
	s_low_power_exit_time_ms = Systick_Tick_GetMs();

	/* 注意：WIFI电源控制由Battery_Update根据实际电量百分比决定 */
	/* 如果USB插入，USB_Detect_Process会处理WIFI电源 */
}
