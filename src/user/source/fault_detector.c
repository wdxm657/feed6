#include "fault_detector.h"
#include "battery_monitor.h"
#include "motor_ctrl.h"
#include "systick_tick.h"
#include "led.h"
#include "ft32f0xx.h"
#include "ft32f0xx_gpio.h"

/* 异常检测相关配置 */
#define FAULT_CHECK_INTERVAL_MS    100U  /* 异常检测间隔（毫秒） */
#define LIMIT_SWITCH_DEBOUNCE_MS   50U   /* 限位开关去抖时间（毫秒） */

/* 异常状态 */
static uint8_t s_fault_status = FAULT_NONE;
static uint32_t s_last_update_ms = 0U;

/* 限位开关状态 */
static uint8_t s_limit_switch_last_state = 0U;
static uint32_t s_limit_switch_trigger_time = 0U;
static uint8_t s_limit_switch_debounced = 0U;

/**
 * @brief 读取限位开关状态
 * @return 1-触发（按下），0-未触发（释放）
 */
static uint8_t Fault_ReadLimitSwitch(void)
{
	/* PB3: 限位开关，上拉输入，按下为低电平 */
	return (GPIO_ReadInputDataBit(GPIOB, GPIO_Pin_3) == Bit_RESET) ? 1U : 0U;
}

/**
 * @brief 控制异常指示LED
 * @param enable 1-亮起，0-熄灭
 */
static void Fault_SetLED(uint8_t enable)
{
	/* 使用LED模块控制普通红色LED作为异常指示 */
	LED_Set(LED_RED, enable);
}

/**
 * @brief 检测限位开关异常
 * @return 1-有异常，0-无异常
 */
static uint8_t Fault_CheckLimitSwitch(void)
{
	uint8_t current_state = Fault_ReadLimitSwitch();
	uint32_t current_ms = Systick_Tick_GetMs();
	
	/* 检测限位开关按下边沿 */
	if (current_state && !s_limit_switch_last_state)
	{
		/* 限位开关刚按下，记录时间 */
		s_limit_switch_trigger_time = current_ms;
		s_limit_switch_debounced = 0U;
	}
	
	/* 检测限位开关释放边沿 */
	if (!current_state && s_limit_switch_last_state)
	{
		/* 限位开关释放，清除触发状态 */
		s_limit_switch_trigger_time = 0U;
		s_limit_switch_debounced = 0U;
	}
	
	/* 去抖处理：限位开关按下后持续一段时间才认为有效 */
	if (current_state && s_limit_switch_trigger_time > 0)
	{
		if ((current_ms - s_limit_switch_trigger_time) >= LIMIT_SWITCH_DEBOUNCE_MS)
		{
			s_limit_switch_debounced = 1U;
		}
	}
	
	s_limit_switch_last_state = current_state;
	
	/* 如果限位开关触发且电机正在运行，则认为电机卡住 */
	if (s_limit_switch_debounced && Motor_IsRunning())
	{
		return 1U;  /* 有异常：电机运行时触发限位开关 */
	}
	
	return 0U;  /* 无异常 */
}

/**
 * @brief 初始化异常检测模块
 */
void Fault_Init(void)
{
	s_fault_status = FAULT_NONE;
	s_last_update_ms = 0U;
	s_limit_switch_last_state = 0U;
	s_limit_switch_trigger_time = 0U;
	s_limit_switch_debounced = 0U;
	
	/* 初始LED状态：熄灭 */
	Fault_SetLED(0);
}

/**
 * @brief 更新异常检测（需要在主循环中定期调用）
 */
void Fault_Update(void)
{
	/* 读取当前时间 */
	uint32_t current_ms = Systick_Tick_GetMs();
	
	/* 检查是否需要更新（避免频繁更新） */
	if ((current_ms - s_last_update_ms) < FAULT_CHECK_INTERVAL_MS)
	{
		return;
	}
	
	s_last_update_ms = current_ms;
	
	/* 清除之前的异常标志 */
	uint8_t new_fault_status = FAULT_NONE;
	
	/* 检测限位开关异常 */
	if (Fault_CheckLimitSwitch())
	{
		new_fault_status |= FAULT_MOTOR_STUCK;
		
		/* 如果电机正在运行，立即停止电机（安全保护） */
		if (Motor_IsRunning())
		{
			Motor_Stop();
		}
	}
	
	
	/* 更新异常状态 */
	s_fault_status = new_fault_status;
	
	/* 控制LED指示 */
	if (s_fault_status != FAULT_NONE)
	{
		Fault_SetLED(1);  /* 有异常，LED亮起 */
	}
	else
	{
		Fault_SetLED(0);  /* 无异常，LED熄灭 */
	}
}

/**
 * @brief 获取当前异常状态
 */
uint8_t Fault_GetStatus(void)
{
	return s_fault_status;
}

/**
 * @brief 检查是否有异常
 */
uint8_t Fault_HasFault(void)
{
	return (s_fault_status != FAULT_NONE) ? 1U : 0U;
}

/**
 * @brief 清除异常标志（手动清除）
 */
void Fault_Clear(void)
{
	s_fault_status = FAULT_NONE;
	s_limit_switch_trigger_time = 0U;
	s_limit_switch_debounced = 0U;
	Fault_SetLED(0);
}

