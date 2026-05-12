#ifndef __USART_INIT_H__
#define __USART_INIT_H__

#ifdef __cplusplus
extern "C" {
#endif

#include "main.h"
#include <stdint.h>

/**
 * @brief 初始化USART
 */
void USART_Init_All(void);

/**
 * @brief 接收一个字符（阻塞式）
 * @param timeout_ms 超时时间（毫秒），<0表示无限等待
 * @return 接收到的字符（0-255），超时返回-1
 */
int USART_GetChar(int timeout_ms);

/**
 * @brief 发送一个字节（阻塞式，等待发送完成）
 * @param data 要发送的数据
 */
void USART_SendByte(uint8_t data);

/**
 * @brief 发送字符串（阻塞式）
 * @param str 要发送的字符串（以'\0'结尾）
 */
void USART_SendString(const char *str);

/**
 * @brief 发送数据缓冲区（阻塞式）
 * @param data 要发送的数据缓冲区
 * @param len 数据长度
 */
void USART_SendBuffer(const uint8_t *data, uint16_t len);

#ifdef __cplusplus
}
#endif

#endif /* __USART_INIT_H__ */

