/**
	******************************************************************************
	* @file 		uart.h
	* @author 	    FMD AE
	* @brief		Header for uart.c module
	* @version 	    V1.0.0
	* @data 		2021-11-15
	******************************************************************************
	* @attention
	* COPYRIGHT (C) 2021 Fremont Micro Devices (SZ) Corporation All rights reserved.
	* This software is provided by the copyright holders and contributors,and the
	*	software is believed to be accurate and reliable. However, Fremont Micro
	*	Devices (SZ) Corporation assumes no responsibility for the consequences of
	*	use of such software or for any infringement of patents of other rights
	*	of third parties, which may result from its use. No license is granted by
	*	implication or otherwise under any patent rights of Fremont Micro Devices (SZ)
	*	Corporation.
	******************************************************************************
	*/
	
/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __UART_H
#define	__UART_H

/* Includes ------------------------------------------------------------------*/
#include "ft32f0xx.h"
#include <stdio.h>
#include "stdint.h"

/* Public Constant prototypes----------------------------------------------------*/
/* Public typedef ---------------------------------------------------------------*/
/* Public define ----------------------------------------------------------------*/
/* Public variables prototypes --------------------------------------------------*/
/* Public function prototypes----------------------------------------------------*/
void USART1_Configuration(void);
void AutoBauRate_StartBitMethod(void) ;
void UART1_send_byte(uint8_t byte);
void UART1_Send(uint8_t *Buffer, uint8_t Length);
void IAP_Send_String(uint8_t *data);


#endif /* __UART_H */

/************************ (C) COPYRIGHT FMD *****END OF FILE****/

