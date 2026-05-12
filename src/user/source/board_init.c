#include "board_init.h"
#include "gpio_init.h"
#include "key.h"
#include "led.h"
#include "exti_init.h"
#include "adc_init.h"
#include "pwm_init.h"
#include "usart_init.h"
#include "rtc_init.h"
#include "delay.h"
#include "motor_ctrl.h"
#include "systick_tick.h"
#include "audio_player.h"
#include "battery_monitor.h"
#include "fault_detector.h"
#include "log.h"
#include "buzzer.h"
#include "iwdg_init.h"
#include "lsi_test.h"

void Board_Init(void)
{
    Delay_Init();
    // IWDG_Init();            /* 初始化看门狗（必须在最前面，防止系统死机） */
    Systick_Tick_Init();  /* 初始化SysTick中断，用于时间基准 */
    GPIO_Init_All();        /* 初始化通用GPIO（不包括按键和LED） */
    Key_Init();             /* 初始化按键GPIO */
    LED_Init();             /* 初始化LED GPIO */
    EXTI_Init_All();
    ADC_Init_All();
    PWM_Init();
    USART_Init_All();
    Buzzer_Init();
    Log_Init();             /* 初始化日志模块（必须在USART初始化之后） */
    RTC_Init_All();
    Motor_Init();
    // LSI_Test_Init();  /* 启用HSE测试输出到PA8，可用示波器测量 */
    // Audio_Init();  /* 初始化音频播放模块 */
    // Battery_Init();  /* 初始化电池检测模块 */
    // Fault_Init();  /* 初始化异常检测模块 */
}
