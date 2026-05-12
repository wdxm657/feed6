#include "factory_reset.h"
#include "rtc_init.h"
#include "flash_storage.h"
#include "delay.h"
#include "ft32f0xx.h"
#include <stdio.h>
#include "log.h"

/**
 * @brief 恢复出厂设置
 */
void Factory_Reset(uint8_t reset_system)
{
	//printf("Factory reset started...\r\n");
	
	/* 1. 清除所有日期的时间表（周一到周日） */
	mcu_dp_raw_update(DPID_MEAL_PLAN, 0, 1);
	for (uint8_t weekday = 1; weekday <= 7; weekday++)
	{
		RTC_Motor_ClearDaySchedule(weekday);
	}
	Flash_Schedule_Erase();
	LOG_DEBUG("Factory_Reset: RTC schedules cleared");


	//printf("RTC schedules cleared\r\n");
	
	/* 2. 清除Flash中的音频数据 */
	// if (Flash_EraseAudio() != 0U)
	// {
	// 	//printf("Flash audio data erased\r\n");
	// }
	// else
	// {
	// 	//printf("Flash audio erase failed or no data\r\n");
	// }
	
	//printf("Factory reset completed\r\n");
	
	/* 3. 如果需要，系统复位 */
	if (reset_system != 0U)
	{
		/* 等待串口输出完成 */
		// Delay_ms(100);
		
		/* 系统复位 */
		NVIC_SystemReset();
	}
}

