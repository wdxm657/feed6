#ifndef __KEY_H__
#define __KEY_H__

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

/* 按键编号定义 */
typedef enum
{
	KEY_1 = 0,  /* 按键1：PB6 */
	KEY_2 = 1   /* 按键2：PA7 */
} KeyNum_t;

/* 按键状态 */
typedef enum
{
	KEY_STATE_RELEASED = 0,  /* 释放状态 */
	KEY_STATE_PRESSED = 1    /* 按下状态 */
} KeyState_t;

/**
 * @brief 初始化按键GPIO
 */
void Key_Init(void);

void key1_control(void);
void key2_control(void);

#ifdef __cplusplus
}
#endif

#endif /* __KEY_H__ */

