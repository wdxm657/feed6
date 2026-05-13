#ifndef __MOTOR_CTRL_H__
#define __MOTOR_CTRL_H__

#ifdef __cplusplus
extern "C" {
#endif

#include "main.h"

/* 电机完成回调函数类型定义 */
typedef void (*MotorCompleteCallback_t)(void);

/* 电机控制初始化 */
void Motor_Init(void);

/* 启动电机正转，持续 duration_ms 毫秒后自动停止（非阻塞） */
void Motor_StartForward(uint32_t duration_ms);

/* 启动电机反转，持续 duration_ms 毫秒后自动停止（非阻塞） */
void Motor_StartReverse(uint32_t duration_ms);

/* 立即停止电机 */
void Motor_Stop(void);

/* 获取电机运行状态：1=运行中，0=停止 */
uint8_t Motor_IsRunning(void);

/* 运行一次完整旋转流程：正转 -> (若必要)反转 -> 限位停止 */
uint8_t Motor_RunOneCycle(void);

/* 电机旋转流程状态机，需要周期调用（例如放入任务调度器） */
void Motor_CycleProcess(void);

/* 限位信号中断回调，由EXTI中断调用 */
void Motor_HandleLimitSwitchInterrupt(void);

/* 1=限位中断参与停机和周期状态；0=忽略（产测：限位仅用于唤醒，电机按定时运行） */
void Motor_SetLimitSwitchFeedbackEnabled(uint8_t enable);

/* 故障处理接口 */
void Motor_ClearCycleFault(void);
uint8_t Motor_IsCycleFault(void);

/* 设置电机完成回调函数（电机自动停止时调用） */
void Motor_SetCompleteCallback(MotorCompleteCallback_t callback);

/* 读取电机电流 */
void Motor_ReadCurrent(void);

/* ADC读取和滤波任务，需要周期调用（例如放入任务调度器） */
void Motor_AdcFilterTask(void);

/* 获取滤波后的电机电流值 */
uint16_t Motor_GetCurrentFiltered(void);

/* 停止电机定时器（用于低功耗模式） */
void Motor_StopTimer(void);

/* 恢复电机定时器（从低功耗模式唤醒后） */
void Motor_ResumeTimer(void);

#ifdef __cplusplus
}
#endif

#endif /* __MOTOR_CTRL_H__ */

