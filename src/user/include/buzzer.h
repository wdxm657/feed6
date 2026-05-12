#ifndef __BUZZER_H__
#define __BUZZER_H__

#ifdef __cplusplus
extern "C" {
#endif

#include "main.h"

void Buzzer_Init(void);
void Buzzer_Task(void);
void Buzzer_Stop(void);
void Buzzer_Play(uint8_t repeat, uint32_t on_ms, uint32_t off_ms);
void Buzzer_PlayWithQuietHours(uint8_t repeat, uint32_t on_ms, uint32_t off_ms); /* 带静音时段检查的播放（用于关机前提示） */
uint8_t Buzzer_IsActive(void);

#ifdef __cplusplus
}
#endif

#endif /* __BUZZER_H__ */

