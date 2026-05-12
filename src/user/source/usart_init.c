#include "main.h"
#include "usart_init.h"

#include <stdint.h>

#include "ft32f0xx_gpio.h"
#include "ft32f0xx_rcc.h"
#include "ft32f0xx_usart.h"
#include "ft32f0xx_misc.h"
#include "delay.h"

#define USART_RX_BUF_SIZE 128

static volatile uint8_t s_rx_buf[USART_RX_BUF_SIZE];
static volatile uint16_t s_rx_head = 0;
static volatile uint16_t s_rx_tail = 0;

static void USART_ConfigGPIO(void);
static void USART_ConfigNVIC(void);
static int USART_BufferGetChar(void);

void USART_Init_All(void)
{
	/* 时钟使能 */
	RCC_AHBPeriphClockCmd(RCC_AHBPeriph_GPIOA, ENABLE);
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_USART1, ENABLE);

	USART_ConfigGPIO();
	USART_ConfigNVIC();

	USART_InitTypeDef usart_init;
	USART_StructInit(&usart_init);
	usart_init.USART_BaudRate = 115200;
	usart_init.USART_WordLength = USART_WordLength_8b;
	usart_init.USART_StopBits = USART_StopBits_1;
	usart_init.USART_Parity = USART_Parity_No;
	usart_init.USART_HardwareFlowControl = USART_HardwareFlowControl_None;
	usart_init.USART_Mode = USART_Mode_Rx | USART_Mode_Tx;
	USART_Init(USART1, &usart_init);

	USART_ITConfig(USART1, USART_IT_RXNE, ENABLE);
	USART_Cmd(USART1, ENABLE);
}

int USART_GetChar(int timeout_ms)
{
	if (timeout_ms < 0)
	{
		for (;;)
		{
			int ch = USART_BufferGetChar();
			if (ch >= 0)
			{
				return ch;
			}
		}
	}

	while (timeout_ms-- >= 0)
	{
		int ch = USART_BufferGetChar();
		if (ch >= 0)
		{
			return ch;
		}

		Delay_ms(1);
	}

	return -1;
}

void USART1_IRQHandler(void)
{
	if (USART_GetITStatus(USART1, USART_IT_RXNE) != RESET)
	{
		uint8_t data = (uint8_t)USART_ReceiveData(USART1);
		
		// uint16_t next_head = (uint16_t)((s_rx_head + 1) % USART_RX_BUF_SIZE);

		// if (next_head != s_rx_tail)
		// {
		// 	s_rx_buf[s_rx_head] = data;
		// 	s_rx_head = next_head;
		// }

		uart_receive_input(data);

		USART_ClearITPendingBit(USART1, USART_IT_RXNE);
	}
}

static void USART_ConfigGPIO(void)
{
	GPIO_InitTypeDef gpio_init;

	GPIO_StructInit(&gpio_init);
	gpio_init.GPIO_Pin = GPIO_Pin_9 | GPIO_Pin_10;
	gpio_init.GPIO_Mode = GPIO_Mode_AF;
	gpio_init.GPIO_OType = GPIO_OType_PP;
	gpio_init.GPIO_Speed = GPIO_Speed_10MHz;
	gpio_init.GPIO_PuPd = GPIO_PuPd_UP;
	GPIO_Init(GPIOA, &gpio_init);

	GPIO_PinAFConfig(GPIOA, GPIO_PinSource9, GPIO_AF_1);
	GPIO_PinAFConfig(GPIOA, GPIO_PinSource10, GPIO_AF_1);
}

static void USART_ConfigNVIC(void)
{
	NVIC_InitTypeDef nvic_init;

	nvic_init.NVIC_IRQChannel = USART1_IRQn;
	nvic_init.NVIC_IRQChannelPriority = 2;
	nvic_init.NVIC_IRQChannelCmd = ENABLE;
	NVIC_Init(&nvic_init);
}

static int USART_BufferGetChar(void)
{
	if (s_rx_head == s_rx_tail)
	{
		return -1;
	}

	uint8_t data = s_rx_buf[s_rx_tail];
	s_rx_tail = (uint16_t)((s_rx_tail + 1) % USART_RX_BUF_SIZE);
	return data;
}

/**
 * @brief 发送一个字节（阻塞式，等待发送完成）
 */
void USART_SendByte(uint8_t data)
{
	/* 等待发送寄存器为空（可以写入新数据） */
	while (USART_GetFlagStatus(USART1, USART_FLAG_TXE) == RESET)
	{
		/* 等待 */
	}
	
	/* 发送数据 */
	USART_SendData(USART1, data);
	
	/* 等待传输完成（数据已完全发送到总线上） */
	while (USART_GetFlagStatus(USART1, USART_FLAG_TC) == RESET)
	{
		/* 等待 */
	}
}

/**
 * @brief 发送字符串（阻塞式）
 */
void USART_SendString(const char *str)
{
	if (str == NULL)
	{
		return;
	}
	
	while (*str != '\0')
	{
		USART_SendByte((uint8_t)(*str));
		str++;
	}
}

/**
 * @brief 发送数据缓冲区（阻塞式）
 */
void USART_SendBuffer(const uint8_t *data, uint16_t len)
{
	if (data == NULL)
	{
		return;
	}
	
	uint16_t i;
	for (i = 0U; i < len; i++)
	{
		USART_SendByte(data[i]);
	}
}
