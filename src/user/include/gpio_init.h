#ifndef __GPIO_INIT_H__
#define __GPIO_INIT_H__

#ifdef __cplusplus
extern "C"
{
#endif

#include "main.h"

/**
 * @brief 初始化所有GPIO（不包括按键和LED，它们由各自模块初始化）
 * 
 * 此函数初始化：
 * - 模拟输入（ADC）：PA0（电池电压）、PA3（电机驱动电压）
 * - 输入引脚：PA4（STDBY）、PA5（CHRG）、PB0（USB检测）、PB3（限位开关）
 * - 输出引脚：PA1（AD读取控制）、PA6（TP4056使能）、PA12/PA15（电机驱动）、
 *            PB1（LC2202电源开关）、PB5（LM4871开关）
 * 
 * 注意：按键（PA7、PB6）和LED（PA2、PA8、PA11、PB7）由各自模块初始化
 */
void GPIO_Init_All(void);

/**
 * @brief USB检测处理
 */
void USB_Detect_Process(void);

/**
 * @brief 读取充电芯片状态引脚（CHRG/STDBY）带滤波
 * @param stby 输出指针，可为NULL
 * @param chrg 输出指针，可为NULL
 * @note 软滤波需在循环中重复调用，避免毛刺误判
 */
void ChargeSignals_ReadFiltered(uint8_t *stby, uint8_t *chrg);
/**
 * @brief 单片机DP上传
 */
void mcu_Dp_Update(void);

/**
 * @brief 获取USB插入状态
 * @return 1-插入，0-拔出
 */
uint8_t Get_USB_Flag(void);
#ifdef __cplusplus
}
#endif

#endif /* __GPIO_INIT_H__ */
