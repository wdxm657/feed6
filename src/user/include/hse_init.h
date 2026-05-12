#ifndef __HSE_INIT_H__
#define __HSE_INIT_H__

#ifdef __cplusplus
extern "C" {
#endif

#include "main.h"

/**
 * @brief 初始化HSE（外部8MHz晶振）并配置为系统时钟
 * @note  此函数会配置HSE作为PLL源，并将PLL输出作为系统时钟
 *        系统时钟频率：8MHz * 9 = 72MHz
 */
void HSE_Init(void);

#ifdef __cplusplus
}
#endif

#endif /* __HSE_INIT_H__ */

