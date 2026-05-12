#ifndef __FACTORY_RESET_H__
#define __FACTORY_RESET_H__

#ifdef __cplusplus
extern "C" {
#endif
#include "main.h"
/**
 * @brief 恢复出厂设置
 * 清除所有用户数据：
 * - 清除RTC时间表（所有日期）
 * - 清除Flash中的音频数据
 * 
 * @param reset_system 是否在恢复后系统复位（1-复位，0-不复位）
 */
void Factory_Reset(uint8_t reset_system);

#ifdef __cplusplus
}
#endif

#endif /* __FACTORY_RESET_H__ */

