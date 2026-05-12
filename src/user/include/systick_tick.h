#ifndef __SYSTICK_TICK_H__
#define __SYSTICK_TICK_H__

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

/**
 * @brief 初始化SysTick定时器，配置为1ms中断
 */
void Systick_Tick_Init(void);

/**
 * @brief 获取系统运行时间（毫秒）
 * @return 系统运行时间，单位：毫秒
 */
uint32_t Systick_Tick_GetMs(void);

/**
 * @brief 检查是否经过了指定时间
 * @param start_ms 开始时间（毫秒）
 * @param duration_ms 持续时间（毫秒）
 * @return 1-已到达，0-未到达
 */
uint8_t Systick_Tick_IsTimeout(uint32_t start_ms, uint32_t duration_ms);

/**
 * @brief 获取系统时钟频率（用于调试）
 * @return 系统时钟频率（Hz）
 */
uint32_t Systick_Tick_GetSystemClock(void);

/**
 * @brief 测试SysTick是否正常工作（用于调试）
 * @return 1-正常，0-异常
 */
uint8_t Systick_Tick_Test(void);

/**
 * @brief 停止SysTick定时器（用于低功耗模式）
 */
void Systick_Tick_Stop(void);

/**
 * @brief 恢复SysTick定时器（从低功耗模式唤醒后）
 */
void Systick_Tick_Resume(void);

#ifdef __cplusplus
}
#endif

#endif /* __SYSTICK_TICK_H__ */

