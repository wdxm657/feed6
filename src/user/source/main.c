/**
 * @file main_task_scheduler_example.c
 * @brief 使用任务调度器的main.c示例
 *
 * 这个文件展示了如何使用任务调度器来管理多个功能模块
 * 可以将此代码复制到main.c中使用
 */

#include "main.h"
#include "board_init.h"
#include "task_scheduler.h"
#include "systick_tick.h"
#include "key.h"
#include "led.h"
#include "log.h"
#include "motor_ctrl.h"
#include "rtc_init.h"
#include "audio_player.h"
#include "battery_monitor.h"
#include "fault_detector.h"
#include "flash_schedule.h"
#include "gpio_init.h"
#include "buzzer.h"
#include "iwdg_init.h"
#include "factory_test.h"

#include "ft32f0xx_gpio.h"
#include "ft32f0xx_rcc.h"

/* 音频采样率（Hz） */
#define AUDIO_SAMPLE_RATE 8000U

/* 电机完成回调函数：电机旋转完成后触发 */
static void Motor_CompleteCallback(void)
{
	/* 从Flash播放主人音频 */
	// if (Audio_PlayFromFlash(AUDIO_SAMPLE_RATE) == 0U)
	// {
	// 	/* Flash中没有音频数据，可以播放默认提示音或静默 */
	// }
}

static uint8_t FactoryTest_ShouldEnterByPA9(void)
{
	/* 工厂测试入口：PA9或PA10 上电接地（低电平） */
	RCC_AHBPeriphClockCmd(RCC_AHBPeriph_GPIOA, ENABLE);

	GPIO_InitTypeDef gpio_init;
	GPIO_StructInit(&gpio_init);
	gpio_init.GPIO_Pin = GPIO_Pin_9 | GPIO_Pin_10;
	gpio_init.GPIO_Mode = GPIO_Mode_IN;
	gpio_init.GPIO_PuPd = GPIO_PuPd_UP;
	GPIO_Init(GPIOA, &gpio_init);

	return (GPIO_ReadInputDataBit(GPIOA, GPIO_Pin_9) == Bit_RESET || GPIO_ReadInputDataBit(GPIOA, GPIO_Pin_10) == Bit_RESET) ? 1U : 0U;
}

int main(void)
{
	uint8_t enter_factory_test = FactoryTest_ShouldEnterByPA9();

	/* 系统初始化 */
	Board_Init();
	/* 初始化任务调度器 */
	TaskScheduler_Init();

	if (enter_factory_test != 0U)
	{
		FactoryTest_Init();

		TaskScheduler_Register(Motor_AdcFilterTask, TASK_PRIORITY_HIGH, 20);
		TaskScheduler_Register(Battery_AdcFilterTask, TASK_PRIORITY_HIGH, 20);
		TaskScheduler_Register(Buzzer_Task, TASK_PRIORITY_HIGH, 50);

		TaskScheduler_Register(FactoryTest_Task, TASK_PRIORITY_NORMAL, 20);

		wifi_protocol_init();
		// LOG_DEBUG("main: enter factory test");
	}
	else
	{
		/* 注册任务（按优先级从高到低） */
		/* 高优先级任务：按键检测（需要快速响应） */
		// TaskScheduler_Register(IWDG_Task, TASK_PRIORITY_HIGH, 500);		   /* 500ms间隔，看门狗喂狗（最高优先级，防止系统死机） */
		TaskScheduler_Register(key1_control, TASK_PRIORITY_HIGH, 10);		   /* 10ms间隔 */
		TaskScheduler_Register(key2_control, TASK_PRIORITY_HIGH, 10);		   /* 10ms间隔 */
		TaskScheduler_Register(Motor_AdcFilterTask, TASK_PRIORITY_HIGH, 20);   /* 20ms间隔，电机ADC滤波（ADC由DMA中断自动更新） */
		TaskScheduler_Register(Battery_AdcFilterTask, TASK_PRIORITY_HIGH, 20); /* 20ms间隔，电池ADC滤波（ADC由DMA中断自动更新） */
		TaskScheduler_Register(Buzzer_Task, TASK_PRIORITY_HIGH, 50);		   /* 50ms间隔，蜂鸣器控制 */

		// /* 普通优先级任务：实时功能 */
		TaskScheduler_Register(Motor_CycleProcess, TASK_PRIORITY_NORMAL, 100);	   /* 100ms监测电流 */
		TaskScheduler_Register(Battery_Update, TASK_PRIORITY_NORMAL, 200);		   /* 200ms间隔，电池业务逻辑处理 */
		TaskScheduler_Register(LED_WifiStatusControl, TASK_PRIORITY_NORMAL, 200);  /* 100ms间隔，WIFI状态LED控制 */
		TaskScheduler_Register(USB_Detect_Process, TASK_PRIORITY_NORMAL, 100);	   /* 1000ms间隔，读取USB插入和拔出状态 */
		TaskScheduler_Register(LED_PowerStatusControl, TASK_PRIORITY_NORMAL, 300); /* 100ms间隔，电源指示灯控制 */
		// TaskScheduler_Register(Audio_Update, TASK_PRIORITY_NORMAL, 0);			   /* 每个循环执行 */

		// /* 低优先级任务：监控功能（不需要频繁执行） */
		TaskScheduler_Register(Flash_Schedule_Process, TASK_PRIORITY_LOW, 1000); /* 1000ms间隔，处理Flash读写 */
		TaskScheduler_Register(RTC_Motor_TimerControl, TASK_PRIORITY_LOW, 1000); /* 1000ms间隔 */
		TaskScheduler_Register(mcu_Dp_Update, TASK_PRIORITY_LOW, 5000);			 /* 5000ms间隔，电源插入检测上报 */
		TaskScheduler_Register(RTC_Update, TASK_PRIORITY_LOW, 60000);			 /* 60000ms间隔 */

		wifi_protocol_init();
		LOG_DEBUG("main: wifi protocol init success");
		Flash_Schedule_Load();
	}

	/* 主循环：运行任务调度器 */
	while (1)
	{
		/* 任务调度器会按照优先级和时间间隔自动调度所有任务 */
		TaskScheduler_Run();

		/* WIFI串口数据处理 */
		wifi_uart_service();
		// LOG_DEBUG("main: wifi uart service success");
	}
}
