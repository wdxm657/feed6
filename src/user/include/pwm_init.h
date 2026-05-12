#ifndef __PWM_INIT_H__
#define __PWM_INIT_H__

#ifdef __cplusplus
extern "C" {
#endif

#include "main.h"

void PWM_Init(void);                          /* TIM3 CH1 on PB4 */
void PWM_SetFrequency(uint32_t freq_hz);      /* 设置频率并保持约50%占空比 */
void PWM_Stop(void);

/* 测试：播放简单旋律，不阻塞过久，便于验证 */
void PWM_PlayMelodyTest(void);

#ifdef __cplusplus
}
#endif

#endif /* __PWM_INIT_H__ */

