#include "motor_ctrl.h"
#include "ft32f0xx.h"
#include "ft32f0xx_rcc.h"
#include "ft32f0xx_tim.h"
#include "ft32f0xx_gpio.h"
#include "adc_init.h"
#include "systick_tick.h"
#include "led.h"
#include "log.h"
#include "buzzer.h"
#include <stdio.h>

/* 基本电机运行状态 */
static volatile uint8_t s_motor_running = 0U;
static volatile uint32_t s_motor_duration_ms = 0U;
static volatile uint32_t s_motor_elapsed_ms = 0U;

/* 电机方向：0=正转，1=反转 */
static volatile uint8_t s_motor_direction = 0U;

/* 电机完成回调函数指针 */
static MotorCompleteCallback_t s_motor_complete_callback = NULL;

/* 旋转一圈流程控制 */
#define MOTOR_CYCLE_TIMEOUT_MS 20000U				/* 每个阶段的最大保护时间，避免无穷运行 */
#define MOTOR_CURRENT_SAMPLE_INTERVAL_MS 20U		/* 电流采样周期，单位ms */
#define MOTOR_SHUNT_RESISTANCE_MOHM 620U			/* 采样电阻：0.62Ω = 620mΩ */
#define MOTOR_NORMAL_CURRENT_MA 70U					/* 正常运行电流：70mA */
#define MOTOR_STALL_CURRENT_MA 150U					/* 堵转电流：150mA */
#define MOTOR_OVERCURRENT_THRESHOLD_DEFAULT 80U	/* 默认堵转阈值（mA） */
#define MOTOR_OVERCURRENT_MAX_COUNT 2U				/* 第二次超电流即视为故障 */
#define MOTOR_REVERSE_OVERCURRENT_DELAY_MS 1000U	/* 反转后忽略过流检测的时间 */
#define MOTOR_THRESHOLD_CALIBRATION_DELAY_MS 1500U	/* 电机启动后等待1500ms再校准阈值 */
#define MOTOR_BUZZER_SINGLE_ON_MS 500U
#define MOTOR_BUZZER_REVERSE_ON_MS 100U
#define MOTOR_BUZZER_REVERSE_GAP_MS 200U
#define MOTOR_BUZZER_REVERSE_COUNT 5U

typedef enum
{
	MOTOR_CYCLE_STATE_IDLE = 0,
	MOTOR_CYCLE_STATE_FORWARD,
	MOTOR_CYCLE_STATE_REVERSE,
	MOTOR_CYCLE_STATE_FAULT
} MotorCycleState_t;

static volatile MotorCycleState_t s_cycle_state = MOTOR_CYCLE_STATE_IDLE;
static volatile uint8_t s_cycle_overcurrent_count = 0U;
static volatile uint8_t s_cycle_overcurrent_latched = 0U;
static volatile uint8_t s_cycle_overcurrent_blocked = 0U;
static volatile uint32_t s_cycle_overcurrent_block_time = 0U;
static volatile uint32_t s_cycle_last_sample_ms = 0U;
static volatile uint8_t s_limit_switch_triggered = 0U; /* 限位开关中断标志 */
static uint8_t s_limit_switch_feedback_enabled = 1U;	 /* 产测时可关闭，避免限位停电机 */

/* 动态阈值相关 */
static volatile uint16_t s_motor_current_threshold = MOTOR_OVERCURRENT_THRESHOLD_DEFAULT; /* 动态过流阈值 */
static volatile uint8_t s_threshold_calibrated = 0U;									  /* 阈值是否已校准 */
static volatile uint32_t s_motor_start_time_ms = 0U;									  /* 电机开始旋转的时间 */
static volatile uint8_t s_pending_reverse = 0U;										  /* 堵转后等待反转的标记 */
static volatile uint8_t s_pending_forward = 0U;										  /* 反转限位后等待正转的标记 */
static volatile uint32_t s_pending_action_time_ms = 0U;								  /* 等待动作的起始时间 */

/* ADC滤波相关 */
#define MOTOR_ADC_FILTER_SIZE 8U /* 均值滤波窗口大小 */
static volatile uint16_t s_motor_adc_buffer[MOTOR_ADC_FILTER_SIZE] = {0U};
static volatile uint8_t s_motor_adc_index = 0U;
static volatile uint8_t s_motor_adc_count = 0U;			/* 已填充的样本数 */
static volatile uint16_t s_motor_current_filtered = 0U; /* 滤波后的电流值（mA） */

/* 定时器配置：使用 TIM6 控制电机运行时间 */
#define MOTOR_TIMER TIM6
#define MOTOR_TIMER_IRQn TIM6_DAC_IRQn
#define MOTOR_TIMER_CLK RCC_APB1Periph_TIM6

/* 电机GPIO：PA12=驱动A，PA15=驱动B */
#define MOTOR_GPIO GPIOA
#define MOTOR_PIN_A GPIO_Pin_12
#define MOTOR_PIN_B GPIO_Pin_15

// 读取电机电流
void Motor_ReadCurrent(void)
{
	uint16_t adc_raw = ADC_ReadMotorRaw();
}

/**
 * @brief ADC均值滤波任务
 * @note ADC值由DMA中断自动更新，此任务进行滤波处理并转换为电流值（mA）
 *       需要周期调用（例如放入任务调度器），建议20-50ms间隔
 */
void Motor_AdcFilterTask(void)
{
	/* 读取DMA中断更新的ADC原始值 */
	uint16_t adc_raw = ADC_ReadMotorRaw();
	static uint32_t s_last_print_ms = 0U;

	/* 将ADC原始值转换为电压（mV），再转换为电流（mA） */
	/* 电流(mA) = 电压(mV) / 电阻(Ω) = 电压(mV) / 0.33 = 电压(mV) * 1000 / 330 */
	uint16_t voltage_mv = ADC_MotorRawToVoltage(adc_raw);
	uint16_t current_ma = (voltage_mv * 1000U) / MOTOR_SHUNT_RESISTANCE_MOHM;

	/* 将电流值存入缓冲区 */
	s_motor_adc_buffer[s_motor_adc_index] = current_ma;
	s_motor_adc_index = (s_motor_adc_index + 1U) % MOTOR_ADC_FILTER_SIZE;

	/* 更新已填充的样本数 */
	if (s_motor_adc_count < MOTOR_ADC_FILTER_SIZE)
	{
		s_motor_adc_count++;
	}

	/* 计算均值 */
	uint32_t sum = 0U;
	uint8_t i;
	for (i = 0U; i < s_motor_adc_count; i++)
	{
		sum += s_motor_adc_buffer[i];
	}

	/* 更新滤波后的电流值（mA） */
	if (s_motor_adc_count > 0U)
	{
		s_motor_current_filtered = (uint16_t)(sum / s_motor_adc_count);
	}
	// 0.1秒打印
	// if (Systick_Tick_GetMs() - s_last_print_ms >= 100U)
	// {
	// 	LOG_DEBUG("Motor_AdcFilterTask: adc_raw: %d, voltage_mv: %d, current_ma: %d, filtered_ma: %d",
	// 			  adc_raw, voltage_mv, current_ma, s_motor_current_filtered);
	// 	s_last_print_ms = Systick_Tick_GetMs();
	// }
}

/**
 * @brief 获取滤波后的电机电流值
 * @return 滤波后的电流值（mA）
 */
uint16_t Motor_GetCurrentFiltered(void)
{
	return s_motor_current_filtered;
}

/* 电机正转：PA6=1, PA7=0 */
static void Motor_SetForward(void)
{
	GPIO_SetBits(MOTOR_GPIO, MOTOR_PIN_A);
	GPIO_ResetBits(MOTOR_GPIO, MOTOR_PIN_B);
}

/* 电机反转：PA6=0, PA7=1 */
static void Motor_SetReverse(void)
{
	GPIO_ResetBits(MOTOR_GPIO, MOTOR_PIN_A);
	GPIO_SetBits(MOTOR_GPIO, MOTOR_PIN_B);
}

/* 停止电机 */
static void Motor_SetStop(void)
{
	GPIO_ResetBits(MOTOR_GPIO, MOTOR_PIN_A | MOTOR_PIN_B);
}

static void Motor_ResetCycleContext(void)
{
	s_cycle_overcurrent_count = 0U;
	s_cycle_overcurrent_latched = 0U;
	s_cycle_overcurrent_blocked = 0U;
	s_cycle_overcurrent_block_time = 0U;
	s_cycle_last_sample_ms = 0U;
	s_threshold_calibrated = 0U;									 /* 重置阈值校准标志 */
	s_motor_current_threshold = MOTOR_OVERCURRENT_THRESHOLD_DEFAULT; /* 重置为默认阈值 */
}

static void Motor_EnterFaultState(void)
{
	Motor_Stop();
	s_cycle_state = MOTOR_CYCLE_STATE_FAULT;
	LED_On(LED_RED); /* 红色LED作为故障指示 */
	mcu_dp_fault_update(DPID_FAULT, 1);
	// 通知蜂鸣器响1声，持续1s
	Buzzer_Play(1U, MOTOR_BUZZER_SINGLE_ON_MS, 0U);
	/* 更新下一个闹钟时间 */
	update_next_alarm();
}

static void Motor_HandleOvercurrentEvent(void)
{
	if (s_cycle_state == MOTOR_CYCLE_STATE_IDLE || s_cycle_state == MOTOR_CYCLE_STATE_FAULT)
	{
		return;
	}

	s_cycle_overcurrent_count++;

	if (s_cycle_overcurrent_count >= MOTOR_OVERCURRENT_MAX_COUNT)
	{
		Motor_EnterFaultState();
		return;
	}

	/* 第一次超电流：立即反转 */
	if (s_cycle_state == MOTOR_CYCLE_STATE_FORWARD)
	{
		/* 先停机0.3s再反转，避免瞬时电流过大 */
		Motor_Stop();
		s_cycle_state = MOTOR_CYCLE_STATE_REVERSE; /* 预期下一步是反转 */
		s_pending_reverse = 1U;
		s_pending_action_time_ms = Systick_Tick_GetMs();
		s_cycle_overcurrent_latched = 1U; /* 阻止在等待期间重复触发 */
	}
	else
	{
		/* 如果已经在反转阶段再次触发，直接进入故障 */
		Motor_EnterFaultState();
	}
}

void Motor_Init(void)
{
	/* 使能定时器时钟 */
	RCC_APB1PeriphClockCmd(MOTOR_TIMER_CLK, ENABLE);

	/* 配置定时器：1ms 周期中断 */
	uint32_t timer_clk = SystemCoreClock;
	uint16_t prescaler = (uint16_t)((timer_clk / 1000000U) - 1U);
	uint16_t period = 1000U - 1U;

	TIM_TimeBaseInitTypeDef tb;
	TIM_TimeBaseStructInit(&tb);
	tb.TIM_Prescaler = prescaler;
	tb.TIM_CounterMode = TIM_CounterMode_Up;
	tb.TIM_Period = period;
	tb.TIM_ClockDivision = TIM_CKD_DIV1;
	TIM_TimeBaseInit(MOTOR_TIMER, &tb);

	TIM_ClearITPendingBit(MOTOR_TIMER, TIM_IT_Update);
	TIM_ITConfig(MOTOR_TIMER, TIM_IT_Update, ENABLE);

	/* 配置NVIC */
	NVIC_InitTypeDef nvic;
	nvic.NVIC_IRQChannel = MOTOR_TIMER_IRQn;
	nvic.NVIC_IRQChannelPriority = 2;
	nvic.NVIC_IRQChannelCmd = ENABLE;
	NVIC_Init(&nvic);

	/* 启动定时器 */
	TIM_Cmd(MOTOR_TIMER, ENABLE);

	/* 初始状态：电机停止 */
	Motor_SetStop();
	s_motor_running = 0U;
	s_limit_switch_feedback_enabled = 1U;
	s_limit_switch_triggered = 0U;
}

void Motor_SetLimitSwitchFeedbackEnabled(uint8_t enable)
{
	s_limit_switch_feedback_enabled = (enable != 0U) ? 1U : 0U;
	if (s_limit_switch_feedback_enabled == 0U)
	{
		s_limit_switch_triggered = 0U;
	}
}

void Motor_StopTimer(void)
{
	/* 停止电机定时器 */
	TIM_Cmd(MOTOR_TIMER, DISABLE);
}

void Motor_ResumeTimer(void)
{
	/* 恢复电机定时器 */
	TIM_Cmd(MOTOR_TIMER, ENABLE);
}

void Motor_StartForward(uint32_t duration_ms)
{
	if (duration_ms == 0U)
	{
		return;
	}

	/* 如果电机正在运行，先停止 */
	if (s_motor_running)
	{
		Motor_Stop();
	}

	/* 设置正转方向 */
	s_motor_direction = 0U;
	s_motor_duration_ms = duration_ms;
	s_motor_elapsed_ms = 0U;
	s_motor_running = 1U;

	/* 记录电机开始时间，用于0.5s后校准阈值 */
	s_motor_start_time_ms = Systick_Tick_GetMs();
	s_threshold_calibrated = 0U; /* 重置阈值校准标志 */

	/* 启动电机正转 */
	Motor_SetForward();
}

void Motor_StartReverse(uint32_t duration_ms)
{
	if (duration_ms == 0U)
	{
		return;
	}

	/* 如果电机正在运行，先停止 */
	if (s_motor_running)
	{
		Motor_Stop();
	}

	/* 设置反转方向 */
	s_motor_direction = 1U;
	s_motor_duration_ms = duration_ms;
	s_motor_elapsed_ms = 0U;
	s_motor_running = 1U;

	/* 记录电机开始时间，用于0.5s后校准阈值 */
	s_motor_start_time_ms = Systick_Tick_GetMs();
	s_threshold_calibrated = 0U; /* 重置阈值校准标志 */

	/* 启动电机反转 */
	Motor_SetReverse();
}

void Motor_Stop(void)
{
	s_motor_running = 0U;
	s_motor_elapsed_ms = 0U;
	s_motor_duration_ms = 0U;
	Motor_SetStop();
}

uint8_t Motor_IsRunning(void)
{
	return s_motor_running;
}

uint8_t Motor_RunOneCycle(void)
{
	if (Battery_GetClosePowerMode() == 1U)
	{
		return 0U;
	}
	if (s_cycle_state == MOTOR_CYCLE_STATE_FAULT)
	{
		/* 处于故障状态时需要先清故障 */
		Motor_ClearCycleFault();
	}

	if (s_cycle_state != MOTOR_CYCLE_STATE_IDLE)
	{
		return 0U;
	}

	Motor_ResetCycleContext();
	s_cycle_state = MOTOR_CYCLE_STATE_FORWARD;
	LED_Off(LED_RED); /* 开始前确保故障灯熄灭 */
	mcu_dp_fault_update(DPID_FAULT, 0);
	mcu_dp_enum_update(DPID_FEED_STATE, 1);
	Motor_StartForward(MOTOR_CYCLE_TIMEOUT_MS);
	return 1U;
}

void Motor_CycleProcess(void)
{
	static uint8_t over_current_count = 0U;
	/* 优先处理限位开关中断 */
	// 电机至少运转3秒后才能处理限位开关中断
	if (s_limit_switch_triggered /* && Systick_Tick_IsTimeout(s_motor_start_time_ms, 3000U)*/)
	{
		s_limit_switch_triggered = 0U;
		if (s_cycle_state == MOTOR_CYCLE_STATE_FORWARD)
		{
			/* 正转限位触发：代表电机正常运转了一圈，完成 */
			s_cycle_state = MOTOR_CYCLE_STATE_IDLE;
			Motor_ResetCycleContext();
			LOG_DEBUG("Motor: Forward limit switch triggered, cycle completed");
			// 上报电机完成事件
			mcu_dp_value_update(DPID_FEED_REPORT, 1);
			mcu_dp_enum_update(DPID_FEED_STATE, 0);
			// 通知蜂鸣器响5声，每声1秒
			Buzzer_Play(MOTOR_BUZZER_REVERSE_COUNT, MOTOR_BUZZER_REVERSE_ON_MS, MOTOR_BUZZER_REVERSE_GAP_MS);

			/* 更新下一个闹钟时间 */
			update_next_alarm();
		}
		else if (s_cycle_state == MOTOR_CYCLE_STATE_REVERSE)
		{
			/* 反转限位触发：代表电机堵转后回退完成，需要再次正转 */
			s_cycle_state = MOTOR_CYCLE_STATE_FORWARD;
			/* 再次反转后的过流检测延迟 */
			s_cycle_overcurrent_blocked = 1U;
			s_cycle_overcurrent_block_time = Systick_Tick_GetMs();
			/* 停机0.3s后再重新启动正转 */
			Motor_Stop();
			s_pending_forward = 1U;
			s_pending_action_time_ms = Systick_Tick_GetMs();
			LOG_DEBUG("Motor: Reverse limit switch triggered, restart forward");
		}
	}

	if (s_cycle_state == MOTOR_CYCLE_STATE_IDLE || s_cycle_state == MOTOR_CYCLE_STATE_FAULT)
	{
		return;
	}

	uint32_t current_ms = Systick_Tick_GetMs();

	/* 处理堵转后等待反转的延时动作（停机0.3s再反转） */
	if (s_pending_reverse != 0U &&
		Systick_Tick_IsTimeout(s_pending_action_time_ms, 300U))
	{
		s_pending_reverse = 0U;
		Motor_StartReverse(MOTOR_CYCLE_TIMEOUT_MS);
		/* 反转后等待一段时间再检测过流 */
		s_cycle_overcurrent_blocked = 1U;
		s_cycle_overcurrent_block_time = current_ms;
		s_cycle_overcurrent_latched = 1U; /* 阻止在等待期间重复触发 */
	}

	/* 处理反转限位后等待正转的延时动作（停机0.3s再正转） */
	if (s_pending_forward != 0U &&
		Systick_Tick_IsTimeout(s_pending_action_time_ms, 300U))
	{
		s_pending_forward = 0U;
		Motor_StartForward(MOTOR_CYCLE_TIMEOUT_MS);
		/* 正转重新启动后，同样屏蔽短时间过流检测，避免启动浪涌误判 */
		s_cycle_overcurrent_blocked = 1U;
		s_cycle_overcurrent_block_time = current_ms;
	}
	if ((s_cycle_last_sample_ms != 0U) &&
		!Systick_Tick_IsTimeout(s_cycle_last_sample_ms, MOTOR_CURRENT_SAMPLE_INTERVAL_MS))
	{
		return;
	}

	s_cycle_last_sample_ms = current_ms;

	/* 反转后的过流检测延迟 */
	if (s_cycle_overcurrent_blocked)
	{
		if (Systick_Tick_IsTimeout(s_cycle_overcurrent_block_time, MOTOR_REVERSE_OVERCURRENT_DELAY_MS))
		{
			s_cycle_overcurrent_blocked = 0U;
			s_cycle_overcurrent_latched = 0U;
		}
		else
		{
			return;
		}
	}

	/* 检查是否需要校准阈值（电机启动1s后） */
	if (s_motor_running && !s_threshold_calibrated)
	{
		if (Systick_Tick_IsTimeout(s_motor_start_time_ms, MOTOR_THRESHOLD_CALIBRATION_DELAY_MS))
		{
			/* 1.5s后读取滤波后的电流值作为阈值 */
			uint16_t filtered_current = Motor_GetCurrentFiltered();
			if (filtered_current > 0U)
			{
				if (filtered_current <= (MOTOR_NORMAL_CURRENT_MA))
				{
					s_motor_current_threshold = MOTOR_NORMAL_CURRENT_MA;
				}
				else if (filtered_current < MOTOR_STALL_CURRENT_MA)
				{
					s_motor_current_threshold = filtered_current;
				}
				else
				{
					/* 检测到的电流已经接近或超过堵转值，使用默认堵转阈值 */
					s_motor_current_threshold = MOTOR_OVERCURRENT_THRESHOLD_DEFAULT;
				}

				s_threshold_calibrated = 1U;
				LOG_DEBUG("Motor_CycleProcess: threshold: %d mA (normal: %d mA, stall: %d mA, filtered: %d mA)",
						  s_motor_current_threshold, MOTOR_NORMAL_CURRENT_MA, MOTOR_STALL_CURRENT_MA, filtered_current);
			}
		}
	}

	/* 使用滤波后的电流值进行过流检测（单位：mA） */
	uint16_t motor_current = Motor_GetCurrentFiltered();
	// LOG_DEBUG("Motor_CycleProcess: motor_current: %d mA, threshold: %d mA", motor_current, s_motor_current_threshold);

	/* 如果电流大于阈值，则代表堵转 */
	if (motor_current > (s_motor_current_threshold + 20U) && Systick_Tick_IsTimeout(s_motor_start_time_ms, MOTOR_THRESHOLD_CALIBRATION_DELAY_MS))
	{
		over_current_count++;
		if (over_current_count >= 2U)
		{
			LOG_DEBUG("Motor_CycleProcess: stall detected: current: %d mA, threshold: %d mA",
					  motor_current, s_motor_current_threshold);
			if (s_cycle_overcurrent_latched == 0U)
			{
				s_cycle_overcurrent_latched = 1U;
				Motor_HandleOvercurrentEvent();
			}
			over_current_count = 0U;
		}
	}
	else if (motor_current < s_motor_current_threshold)
	{
		/* 电流低于阈值，清除锁存状态 */
		s_cycle_overcurrent_latched = 0U;
		over_current_count = 0U;
	}
}

void Motor_HandleLimitSwitchInterrupt(void)
{
	if (s_limit_switch_feedback_enabled == 0U)
	{
		return;
	}
	// 中断触发后再读取一次限位开关状态，如果是高电平，则认为是毛刺
	if (GPIO_ReadInputDataBit(GPIOB, GPIO_Pin_3) != RESET)
	{
		return;
	}
	/* 中断函数应快速处理：只停止电机并设置标志位 */
	/* 复杂的状态机逻辑将在Motor_CycleProcess()中处理 */
	if (Systick_Tick_GetMs() - s_motor_start_time_ms < 1500U)
	{
		// 误触发
		// LOG_DEBUG("limit switch triggered but start time is less than 1500ms");
		return;
	}
	else
	{
		Motor_Stop();
		s_limit_switch_triggered = 1U;
	}
}

void Motor_ClearCycleFault(void)
{
	if (s_cycle_state == MOTOR_CYCLE_STATE_FAULT)
	{
		LED_Off(LED_RED);
		s_cycle_state = MOTOR_CYCLE_STATE_IDLE;
		Motor_ResetCycleContext();
	}
}

uint8_t Motor_IsCycleFault(void)
{
	return (s_cycle_state == MOTOR_CYCLE_STATE_FAULT) ? 1U : 0U;
}

/* 设置电机完成回调函数 */
void Motor_SetCompleteCallback(MotorCompleteCallback_t callback)
{
	s_motor_complete_callback = callback;
}

/* TIM6 中断处理函数：1ms 周期，累计电机运行时间 */
void TIM6_IRQHandler(void)
{
	if (TIM_GetITStatus(MOTOR_TIMER, TIM_IT_Update) != RESET)
	{
		TIM_ClearITPendingBit(MOTOR_TIMER, TIM_IT_Update);

		/* 如果电机正在运行，累计时间 */
		if (s_motor_running)
		{
			s_motor_elapsed_ms++;

			/* 时间到达，自动停止电机 */
			if (s_motor_elapsed_ms >= s_motor_duration_ms)
			{
				s_cycle_state = MOTOR_CYCLE_STATE_IDLE;
				Motor_Stop();
				// 亮红灯
				LED_On(LED_RED);
				Motor_ResetCycleContext();
				/* 调用完成回调函数（如果已设置） */
				if (s_motor_complete_callback != NULL)
				{
					s_motor_complete_callback();
				}
			}
		}
	}
}
