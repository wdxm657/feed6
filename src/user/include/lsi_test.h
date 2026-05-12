#ifndef __LSI_TEST_H__
#define __LSI_TEST_H__

#ifdef __cplusplus
extern "C" {
#endif

#include "main.h"

/**
 * @brief 初始化HSE时钟输出测试模块
 * @note  将HSE时钟输出到PA8引脚，可用示波器测量实际HSE频率
 *        输出频率：HSE / 64 = 125kHz（便于示波器测量）
 *        使用方法：调用 LSI_Test_Init() 后，用示波器测量PA8引脚的频率
 *        注意：PA8会覆盖原有的LED功能
 */
void LSI_Test_Init(void);

/**
 * @brief 停止HSE时钟输出测试
 */
void LSI_Test_Stop(void);

#ifdef __cplusplus
}
#endif

#endif /* __LSI_TEST_H__ */

