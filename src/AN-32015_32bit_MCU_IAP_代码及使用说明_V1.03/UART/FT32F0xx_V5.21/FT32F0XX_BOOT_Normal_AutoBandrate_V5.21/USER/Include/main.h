/**
	******************************************************************************
	* @file 		main.h
	* @author 	FMD AE
	* @brief		Header for main.c module
	* @version 	V1.0.0
	* @data 		2021-09-14
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
typedef  void (*pFunction)(void);
/* Public define ----------------------------------------------------------------*/
#define	READ_FLASH(X)		  (*(uint8_t *)(X))
#define ApplicationAddress    ((uint32_t)0x08002000)    //APP Program start address
#define IaplicationAddress    ((uint32_t)0x08000000)    //IAP Program start address
#define ApplicaflagAddress    ((uint32_t)0x08001E00)    //APP Program start flag

/*intrupt address*/
#define Reset_Handler_Adderes                   (ApplicationAddress+0x04)
#define NMI_Handler_Adderes    					(ApplicationAddress+0x08)
#define HardFault_Handler_Adderes  				(ApplicationAddress+0x0c)
#define SVC_Handler_Adderes  					(ApplicationAddress+0x2c)
#define PendSV_Handler_Adderes  				(ApplicationAddress+0x38)
#define SysTick_Handler_Adderes  				(ApplicationAddress+0x3c)
#define WWDG_IRQHandler_Adderes  				(ApplicationAddress+0x40)
#define PVD_VDDIO_IRQHandler_Adderes  			(ApplicationAddress+0x44)
#define RTC_IRQHandler_Adderes  				(ApplicationAddress+0x48)
#define FLASH_IRQHandler_Adderes  				(ApplicationAddress+0x4c)
#define RCC_IRQHandler_Adderes  				(ApplicationAddress+0x50)
#define EXTI0_1_IRQHandler_Adderes  			(ApplicationAddress+0x54)
#define EXTI2_3_IRQHandler_Adderes  			(ApplicationAddress+0x58)
#define EXTI4_15_IRQHandler_Adderes  			(ApplicationAddress+0x5c)
#define DMA1_Channel1_IRQHandler_Adderes  		(ApplicationAddress+0x64)
#define DMA1_Channel2_3_IRQHandler_Adderes  	(ApplicationAddress+0x68)
#define DMA1_Channel4_5_IRQHandler_Adderes      (ApplicationAddress+0x6c)
#define ADC1_IRQHandler_Adderes                 (ApplicationAddress+0x70)
#define TIM1_BRK_UP_TRG_COM_IRQHandler_Adderes  (ApplicationAddress+0x74)
#define TIM1_CC_IRQHandler_Adderes 				(ApplicationAddress+0x78)
#define TIM3_IRQHandler_Adderes  				(ApplicationAddress+0x80)
#define TIM6_IRQHandler_Adderes  				(ApplicationAddress+0x84)
#define TIM14_IRQHandler_Adderes  				(ApplicationAddress+0x8c)
#define TIM15_IRQHandler_Adderes  				(ApplicationAddress+0x90)
#define TIM16_IRQHandler_Adderes  				(ApplicationAddress+0x94)
#define TIM17_IRQHandler_Adderes  				(ApplicationAddress+0x98)
#define I2C1_IRQHandler_Adderes  				(ApplicationAddress+0x9c)
#define I2C2_IRQHandler_Adderes  				(ApplicationAddress+0xa0)
#define SPI1_IRQHandler_Adderes  				(ApplicationAddress+0xa4)
#define SPI2_IRQHandler_Adderes  				(ApplicationAddress+0xa8)
#define USART1_IRQHandler_Adderes  				(ApplicationAddress+0xac)
#define USART2_IRQHandler_Adderes  				(ApplicationAddress+0xb0)
#define USB_IRQHandler_Adderes  				(ApplicationAddress+0xbc)
                                                

/*CMD define*/

#define  CMD_W_ADDR     0X01
#define  CMD_W_DATA     0X02
#define  CMD_R_DATA     0X03
#define  CMD_E_FLASH    0X04
#define  CMD_W_FLASH    0X05
#define  CMD_S_USR      0X06

#define  CMD_WAIT       0XFF
/*NOP define*/
#define  NOP_DATA       0X00

#if (defined(FT32F072xB) || defined(FT32F030xB) )
    #define FLASH_END_ADDR  ((uint32_t)0x0801FFFF)
    #define FLASH_MAX_SIZE  ((uint32_t)0x0001FFFF)
    #define FLASH_PAGE_SIZE ((uint32_t)1024)
#else
    #define FLASH_END_ADDR  ((uint32_t)0x0800FFFF)
    #define FLASH_MAX_SIZE  ((uint32_t)0x0000FFFF)
    #define FLASH_PAGE_SIZE ((uint32_t)512)
#endif

/* Public variables prototypes --------------------------------------------------*/
extern pFunction Jump_To_Application;
extern uint32_t JumpAddress;

extern uint8_t updata_flag[];
extern uint8_t LED_Blink_Timer;
extern uint8_t flash_data[519];

/* Public function prototypes----------------------------------------------------*/
uint16_t CRC16_CCITT(uint8_t *data, uint16_t datalen);


#endif /* __MAIN_H */

/************************ (C) COPYRIGHT FMD *****END OF FILE****/
