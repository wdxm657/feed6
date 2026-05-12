#ifndef __BATTERY_MONITOR_H__
#define __BATTERY_MONITOR_H__

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
/**
 * @brief 初始化电池检测模块
 */
void Battery_Init(void);

/**
 * @brief ADC读取和均值滤波任务
 * @note ADC值由DMA中断自动更新，此任务进行滤波处理
 *       需要周期调用（例如放入任务调度器），建议20ms间隔
 */
void Battery_AdcFilterTask(void);

/**
 * @brief 更新电池状态（业务逻辑处理）
 * @note 需要在主循环中定期调用，建议50-500ms间隔
 */
void Battery_Update(void);

/**
 * @brief 获取滤波后的电池电压值
 * @return 滤波后的电压值（mV）
 */
uint16_t Battery_GetVoltageFiltered(void);

/**
 * @brief 获取电池电压（mV）
 * @return 电池电压，单位：毫伏
 */
uint16_t Battery_GetVoltage(void);

/**
 * @brief 获取电池ADC原始值
 * @return ADC原始值（0-4095）
 */
uint16_t Battery_GetRawADC(void);

/**
 * @brief 获取电池电量百分比
 * @return 电池电量百分比（0-100）
 */
uint8_t Battery_GetPercentage(void);

/**
 * @brief 进入低功耗模式（Stop模式）
 * @note USB插入（PA4/PA5中断）或按键中断会唤醒系统
 */
void Battery_EnterLowPowerMode(void);

/**
 * @brief 退出低功耗模式（从Stop模式唤醒后）
 */
void Battery_ExitLowPowerMode(void);

/**
 * @brief 获取关闭电源模式标志
 * @return 关闭电源模式标志
 */
uint8_t Battery_GetClosePowerMode(void);

/**
 * @brief 设置关闭电源模式标志
 * @param close_power_mode 关闭电源模式标志
 */
void Battery_SetClosePowerMode(uint8_t close_power_mode);

/**
 * @brief 获取是否曾经进入过低电量状态（低于UPPER_POWER_THRESHOLD）
 * @return 1-曾经进入过低电量，0-未进入过低电量
 */
uint8_t Battery_GetEverLowPower(void);

/**
 * @brief 获取是否进入过究极低功耗状态
 * @return 1-进入过究极低功耗状态，0-未进入过究极低功耗状态
 */
uint8_t Battery_GetCriticalLowStarted(void);

/**
 * @brief 获取USB是否曾经插入过
 * @return 1-USB曾经插入过，0-USB从未插入过
 */
uint8_t Battery_GetUsbEverInserted(void);

#ifdef __cplusplus
}
#endif

#endif /* __BATTERY_MONITOR_H__ */

