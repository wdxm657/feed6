/**
	******************************************************************************
	* @file 		main.h
	* @author 	    FMD AE
	* @brief		Header for main.c module
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
#ifndef __MAIN_H
#define __MAIN_H

/* Includes ------------------------------------------------------------------*/
#include "ft32f0xx.h"
#include "uart.h"
#include "iap.h"
#include "stdint.h"

/* Public Constant prototypes----------------------------------------------------*/
/* Public typedef ---------------------------------------------------------------*/
/* Public define ----------------------------------------------------------------*/
#define ApplicationAddress    ((uint32_t)0x08002000)    //APP Program start address
#define IaplicationAddress    ((uint32_t)0x08000000)    //IAP Program start address
#define ApplicaflagAddress    ((uint32_t)0x08001E00)    //APP Program start flag

/*CMD define*/

#define  CMD_W_ADDR       0X01
#define  CMD_W_DATA       0X02
#define  CMD_R_DATA       0X03
#define  CMD_E_FLASH      0X04
#define  CMD_W_FLASH      0X05
#define  CMD_S_USR        0X06

#define  CMD_WAIT          0XFF
/*NOP define*/
#define  NOP_DATA          0X00

/* Public variables prototypes --------------------------------------------------*/
extern uint8_t LED_Blink_Timer;

/* Public function prototypes----------------------------------------------------*/

#endif /* __MAIN_H */

/************************ (C) COPYRIGHT FMD *****END OF FILE****/
