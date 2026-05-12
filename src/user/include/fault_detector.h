#ifndef __FAULT_DETECTOR_H__
#define __FAULT_DETECTOR_H__

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

/* 异常类型定义（位标志） */
#define FAULT_NONE           0x00U
#define FAULT_MOTOR_STUCK    0x01U  /* 电机卡住（限位开关触发） */
#define FAULT_BATTERY_LOW    0x02U  /* 电池电量过低 */
#define FAULT_SYSTEM_ERROR   0x04U  /* 系统错误（预留） */

/**
 * @brief 初始化异常检测模块
 */
void Fault_Init(void);

/**
 * @brief 更新异常检测（需要在主循环中定期调用）
 */
void Fault_Update(void);

/**
 * @brief 获取当前异常状态
 * @return 异常标志位（0=无异常，非0=有异常）
 */
uint8_t Fault_GetStatus(void);

/**
 * @brief 检查是否有异常
 * @return 1-有异常，0-无异常
 */
uint8_t Fault_HasFault(void);

/**
 * @brief 清除异常标志（手动清除）
 */
void Fault_Clear(void);

#ifdef __cplusplus
}
#endif

#endif /* __FAULT_DETECTOR_H__ */

