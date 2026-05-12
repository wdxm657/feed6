#include "exti_init.h"
#include "gpio_init.h"
#include "led.h"
#include "motor_ctrl.h"
#include "main.h"

static void ConfigureExtiLine(uint8_t port_source, uint8_t pin_source, uint32_t line);
static void InitNvicChannel(uint8_t irq_channel, uint8_t priority);

void EXTI_Init_All(void)
{
    EXTI_InitTypeDef exti_init;

    /* 使能系统配置控制器时钟以重映射EXTI */
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_SYSCFG, ENABLE);

    /* 端口复用到EXTI线路 */
    ConfigureExtiLine(EXTI_PortSourceGPIOA, EXTI_PinSource4, EXTI_Line4);  /* PA4: 充满电信号(STDBY) */
    ConfigureExtiLine(EXTI_PortSourceGPIOA, EXTI_PinSource5, EXTI_Line5);  /* PA5: 充电中信号(CHRG) */
    ConfigureExtiLine(EXTI_PortSourceGPIOB, EXTI_PinSource3, EXTI_Line3);  /* PB3: 电机限位信号 */
    ConfigureExtiLine(EXTI_PortSourceGPIOB, EXTI_PinSource6, EXTI_Line6);  /* PB6: 按键1信号 */
    ConfigureExtiLine(EXTI_PortSourceGPIOA, EXTI_PinSource7, EXTI_Line7);  /* PA7: 按键2信号 */

    EXTI_StructInit(&exti_init);
    exti_init.EXTI_Mode = EXTI_Mode_Interrupt;
    exti_init.EXTI_Trigger = EXTI_Trigger_Rising_Falling;  /* 双边沿触发 */
    exti_init.EXTI_LineCmd = ENABLE;

    /* 初始化各条线路 */
    exti_init.EXTI_Line = EXTI_Line3;  /* PB3: 电机限位信号 */
    EXTI_Init(&exti_init);

    exti_init.EXTI_Line = EXTI_Line4;  /* PA4: 充满电信号 */
    EXTI_Init(&exti_init);

    exti_init.EXTI_Line = EXTI_Line5;  /* PA5: 充电中信号 */
    EXTI_Init(&exti_init);

    exti_init.EXTI_Line = EXTI_Line6;  /* PB6: 按键1信号 */
    EXTI_Init(&exti_init);

    exti_init.EXTI_Line = EXTI_Line7;  /* PA7: 按键2信号 */
    EXTI_Init(&exti_init);

    /* 使能NVIC中断 */
    InitNvicChannel(EXTI2_3_IRQn, 1);   /* PB3 (Line3) 使用此中断 */
    InitNvicChannel(EXTI4_15_IRQn, 1);  /* PA4 (Line4) 和 PA5 (Line5) 和 PB6 (Line6) 和 PA7 (Line7) 使用此中断 */
}

static void ConfigureExtiLine(uint8_t port_source, uint8_t pin_source, uint32_t line)
{
    SYSCFG_EXTILineConfig(port_source, pin_source);
    EXTI_ClearITPendingBit(line);
}

static void InitNvicChannel(uint8_t irq_channel, uint8_t priority)
{
    NVIC_InitTypeDef nvic_init;

    nvic_init.NVIC_IRQChannel = irq_channel;
    nvic_init.NVIC_IRQChannelPriority = priority;
    nvic_init.NVIC_IRQChannelCmd = ENABLE;
    NVIC_Init(&nvic_init);
}

/**
 * @brief  EXTI2_3中断处理函数
 * @note   处理PB3 (Line3) - 电机限位信号
 */
void EXTI2_3_IRQHandler(void)
{
	/* PB3: 电机限位信号 */
	if (EXTI_GetITStatus(EXTI_Line3) != RESET)
	{
		EXTI_ClearITPendingBit(EXTI_Line3);
        if(GPIO_ReadInputDataBit(GPIOB, GPIO_Pin_3) == RESET)
        {
		    Motor_HandleLimitSwitchInterrupt();
        }
	}
}

/**
 * @brief  EXTI4_15中断处理函数
 * @note   处理PA4 (Line4) - 充满电信号, PA5 (Line5) - 充电中信号
 *         在Stop模式下，此中断可以唤醒系统
 */
void EXTI4_15_IRQHandler(void)
{
    /* PA4: 充满电信号(STDBY) */
    if (EXTI_GetITStatus(EXTI_Line4) != RESET)
    {
        EXTI_ClearITPendingBit(EXTI_Line4);
        // 低电平表示充满电
        // 在Stop模式下，此中断会唤醒系统，退出低功耗由Battery_EnterLowPowerMode返回后自动处理
    }

    /* PA5: 充电中信号(CHRG) */
    if (EXTI_GetITStatus(EXTI_Line5) != RESET)
    {
        EXTI_ClearITPendingBit(EXTI_Line5);
        // 低电平表示充电中
        // 在Stop模式下，此中断会唤醒系统，退出低功耗由Battery_EnterLowPowerMode返回后自动处理
    }

    /* PB6: 按键1信号 */
    if (EXTI_GetITStatus(EXTI_Line6) != RESET)
    {
        EXTI_ClearITPendingBit(EXTI_Line6);
        // 按键1按下
    }

    /* PA7: 按键2信号 */
    if (EXTI_GetITStatus(EXTI_Line7) != RESET)
    {
        EXTI_ClearITPendingBit(EXTI_Line7);
        // 按键2按下
    }
}

