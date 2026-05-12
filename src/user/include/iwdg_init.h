#ifndef __IWDG_INIT_H__
#define __IWDG_INIT_H__

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

/**
 * @brief 初始化独立看门狗（IWDG）
 * @note 看门狗使用LSI（约40kHz），超时时间约1秒
 *       需要在系统初始化时调用，且只能初始化一次
 */
void IWDG_Init(void);

/**
 * @brief 喂狗（重载看门狗计数器）
 * @note 需要在看门狗超时前定期调用，建议间隔小于超时时间的1/2
 */
void IWDG_Feed(void);

/**
 * @brief 看门狗喂狗任务（用于任务调度器）
 * @note 此任务会定期喂狗，建议注册为高优先级任务，间隔500ms
 */
void IWDG_Task(void);

#ifdef __cplusplus
}
#endif

#endif /* __IWDG_INIT_H__ */

