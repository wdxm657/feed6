/**
 * @file retarget.c
 * @brief printf重定向到串口
 * 
 * 此文件实现fputc函数，将printf输出重定向到USART1
 * 
 * 使用说明：
 * 1. 确保在Board_Init()中已调用USART_Init_All()初始化串口
 * 2. 在Keil项目设置中，可以选择使用MicroLIB或标准C库
 * 3. 使用MicroLIB可以减小代码体积，适合资源受限的单片机
 * 
 * 配置方法（Keil MDK）：
 * Options for Target -> Target -> 勾选"Use MicroLIB"
 * 或者
 * Options for Target -> Target -> 使用标准C库（不勾选MicroLIB）
 */

#include <stdio.h>
#include "usart_init.h"

/**
 * @brief 重定向fputc函数，将printf输出到串口
 * 
 * 此函数会被printf、puts等标准输出函数调用
 * 
 * @param ch 要输出的字符
 * @param f 文件指针（未使用，但必须声明以匹配函数签名）
 * @return 成功返回ch，失败返回EOF
 */
int fputc(int ch, FILE *f)
{
	(void)f;  /* 避免未使用参数警告 */
	
	/* 将字符发送到USART1 */
	USART_SendByte((uint8_t)ch);
	
	/* 如果发送换行符，自动添加回车符（Windows风格换行） */
	/* 这样在串口助手中可以正确显示换行 */
	if (ch == '\n')
	{
		USART_SendByte('\r');
		USART_SendByte('\n');
	}
	
	return ch;
}

/**
 * @brief 重定向fgetc函数，从串口读取字符（可选实现）
 * 
 * 此函数会被scanf、getchar等标准输入函数调用
 * 
 * @param f 文件指针（未使用，但必须声明以匹配函数签名）
 * @return 读取到的字符，失败返回EOF
 */
int fgetc(FILE *f)
{
	(void)f;  /* 避免未使用参数警告 */
	
	/* 从USART1接收一个字符，超时时间100ms */
	int ch = USART_GetChar(100);
	
	if (ch < 0)
	{
		return EOF;
	}
	
	return ch;
}

