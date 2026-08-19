/**
	******************************************************************************
	* @file 		iap.h
	* @author 	    FMD AE
	* @brief		Header for iap.c module
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
#ifndef __IAP_H
#define __IAP_H

/* Includes ------------------------------------------------------------------*/
#include "ft32f0xx.h"

/* Public Constant prototypes----------------------------------------------------*/
/* Public typedef ---------------------------------------------------------------*/
/* Public define ----------------------------------------------------------------*/
/* Public variables prototypes --------------------------------------------------*/


/* Public function prototypes----------------------------------------------------*/ 
void FLASH_Erase_OnePage(uint32_t Address);  
void FLASH_WriteWord(uint32_t addr , uint8_t *dataBuf , uint32_t Byte_Num);
void Flash_Read(uint32_t iAddress, uint8_t *buf, int32_t iNbrToRead);


#endif  /* __IAP_H */

/************************ (C) COPYRIGHT FMD *****END OF FILE****/




