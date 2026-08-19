/**
	******************************************************************************
	* @file 		ft32f0xx_it.c
	* @author 	    FMD AE
    * @brief   	    Main Interrupt Service Routines.
    *          	    This file provides template for all exceptions handler and 
    *          	    peripherals interrupt service routine.
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
	
/* Includes ------------------------------------------------------------------*/
#include "ft32f0xx_it.h"


/* Private Constant --------------------------------------------------------------*/
/* The last six bits correspond to the character FMDIAP */
const uint8_t Iap_Start_Head[8] = {0x55,0xAA,0x46,0x4D,0x44,0x49,0x41,0x50}; 

/* Public Constant ---------------------------------------------------------------*/
/* Private typedef ---------------------------------------------------------------*/
/* Private define ----------------------------------------------------------------*/
/* Private variables -------------------------------------------------------------*/
uint8_t USART1_RX[519];
uint8_t AutoBaudBate_flag = 0;
uint8_t USART1_COUNT;
uint16_t USART1_RX_num = 0;

/* Public variables --------------------------------------------------------------*/
/* Private function prototypes ---------------------------------------------------*/
/* Public function ------ --------------------------------------------------------*/


/******************************************************************************/
/*            Cortex-M0 Processor Exceptions Handlers                         */
/******************************************************************************/

/******************************************************************************
  * @brief  This function handles NMI exception.
  * @param  None
  * @note  
  * @retval None
  *****************************************************************************
*/
void NMI_Handler(void)
{
	if(READ_FLASH(ApplicaflagAddress)==0x66)
	{
		JumpAddress = *(__IO uint32_t*) NMI_Handler_Adderes;
		Jump_To_Application = (pFunction) JumpAddress;
//		__set_MSP(*(__IO uint32_t*) NMI_Handler_Adderes);
		Jump_To_Application();
	}
}

/******************************************************************************
  * @brief  This function handles HardFault exception.
  * @param  None
  * @note  
  * @retval None
  *****************************************************************************
*/
void HardFault_Handler(void)
{
	if(READ_FLASH(ApplicaflagAddress)==0x66)
	{
		JumpAddress = *(__IO uint32_t*) HardFault_Handler_Adderes;
		Jump_To_Application = (pFunction) JumpAddress;
		Jump_To_Application();
	}
}

/******************************************************************************
  * @brief  This function handles SVC exception.
  * @param  None
  * @note  
  * @retval None
  *****************************************************************************
*/
void SVC_Handler(void)
{
	if(READ_FLASH(ApplicaflagAddress)==0x66)
	{
		JumpAddress = *(__IO uint32_t*) SVC_Handler_Adderes;
		Jump_To_Application = (pFunction) JumpAddress;
		Jump_To_Application();
	}
}

/******************************************************************************
  * @brief  This function handles PendSV exception.
  * @param  None
  * @note  
  * @retval None
  *****************************************************************************
*/
void PendSV_Handler(void)
{
	if(READ_FLASH(ApplicaflagAddress)==0x66)
	{
		JumpAddress = *(__IO uint32_t*) PendSV_Handler_Adderes;
		Jump_To_Application = (pFunction) JumpAddress;
		Jump_To_Application();
	}
}

/******************************************************************************
  * @brief  This function handles SysTick exception.
  * @param  None
  * @note  
  * @retval None
  *****************************************************************************
*/
void SysTick_Handler(void)
{
	if(READ_FLASH(ApplicaflagAddress)==0x66)
	{
		JumpAddress = *(__IO uint32_t*) SysTick_Handler_Adderes;
		Jump_To_Application = (pFunction) JumpAddress;
		Jump_To_Application();
	}
}

/******************************************************************************/
/*                 FT32F072X8 Peripherals Interrupt Handlers                   */
/*  Add here the Interrupt Handler for the used peripheral(s) (PPP), for the  */
/*  available peripheral interrupt handler's name please refer to the startup */
/*  file (startup_ft32f072x8.s).                                            */
/******************************************************************************/

/******************************************************************************
  * @brief  This function handles PPP Handler.
  * @param  None
  * @note  
  * @retval None
  *****************************************************************************
*/
/*void PPP_IRQHandler(void)
{
}*/

/******************************************************************************
  * @brief  This function handles USB Handler.
  * @param  None
  * @note  
  * @retval None
  *****************************************************************************
*/
void USB_IRQHandler(void)
{
	if(READ_FLASH(ApplicaflagAddress)==0x66)
	{	
		JumpAddress = *(__IO uint32_t*) USB_IRQHandler_Adderes;
		Jump_To_Application = (pFunction) JumpAddress;
		Jump_To_Application();
	}
}
/******************************************************************************
  * @brief  This function handles TIM14 Handler.
  * @param  None
  * @note  
  * @retval None
  *****************************************************************************
*/ 
void TIM14_IRQHandler(void)   
{
    //10ms
	if(READ_FLASH(ApplicaflagAddress)==0x66)
	{
		JumpAddress = *(__IO uint32_t*) TIM14_IRQHandler_Adderes;
		Jump_To_Application = (pFunction) JumpAddress;
		Jump_To_Application();
	}
	else
	{
		if(TIM_GetITStatus(TIM14 , TIM_IT_Update) == SET)
		{
			TIM_ClearITPendingBit(TIM14,TIM_FLAG_Update);
			LED_Blink_Timer ++;
			if(cmd_updata_flag == 1)
			{
				USART1_COUNT++;
				if(USART1_COUNT>15)
				{
					USART1_COUNT = 0;
					cmd_updata_flag = 0;
					USART1_RX_num = 0;
				}
			}
		}

	
	}
}


/******************************************************************************
  * @brief  This function handles USART1 Handler.
  * @param  None
  * @note  
  * @retval None
  *****************************************************************************
*/

void USART1_IRQHandler(void)
{
	uint16_t i;
	uint16_t CRC_16;
	if(READ_FLASH(ApplicaflagAddress)==0x66)
	{
		JumpAddress = *(__IO uint32_t*) USART1_IRQHandler_Adderes;
		Jump_To_Application = (pFunction) JumpAddress;
		Jump_To_Application();
	}
	else
	{
	
		if (USART_GetITStatus(USART1, USART_IT_RXNE) == SET)
		{
			USART_ClearITPendingBit(USART1,USART_IT_RXNE);
			USART1_RX[USART1_RX_num] = USART_ReceiveData(USART1);
			if(AutoBaudBate_flag == 0)
			{
				if(USART1_RX[0]==0x55)
				{
					AutoBaudBate_flag = 1;
				}
				else
				{
					if(USART_GetFlagStatus(USART1,USART_FLAG_ABRF) != RESET)
					{
						USART_RequestCmd(USART1,USART_Request_ABRRQ,ENABLE);
					}
				}
			}
			if(cmd_updata_flag == 0)
			{
				if(USART1_RX[USART1_RX_num]==Iap_Start_Head[USART1_RX_num])
				{
					USART1_RX_num++;
					if(USART1_RX_num > 7)
					{
						cmd_updata_flag = 1;
						Iap_cmd = 0;
						FLASH_Erase_OnePage(ApplicaflagAddress);
						UART1_send_byte(0xaa);
						USART1_RX_num = 0;
					}
				}
				else
				{
					USART1_RX_num = 0;
				}
			}
			else
			{
				USART1_RX_num++;
				USART1_COUNT = 0;
				switch(USART1_RX[0])
				{
					case CMD_W_FLASH:   //Write flash command
						if(USART1_RX_num > 518)
						{
							USART1_RX_num = 0;
							CRC_16 = CRC16_CCITT(USART1_RX,517);

							if(CRC_16 !=((USART1_RX[517] << 8)+ USART1_RX[518]))
							{
								UART1_send_byte(0x44);//Error
							}
							else
							{
								for(i=0;i<519;i++)  
								{
									flash_data[i] = USART1_RX[i];
								}
								Iap_cmd = CMD_W_FLASH;
							}
							
						}
					  break;
					case CMD_S_USR:  //Wait until the 0x00 byte is sent to complete a soft reset
						Iap_cmd = CMD_S_USR;
						USART1_RX_num = 0;
					  break;
													
					default:
						USART1_RX_num = 0;
					  break;
				}
				
			}
		}
	}
}

/******************************************************************************
  * @brief  This function handles PPP Handler.
  * @param  None
  * @note  
  * @retval None
  *****************************************************************************
*/

/******************************************************************************
  * @brief  This function handles PPP Handler.
  * @param  None
  * @note  
  * @retval None
  *****************************************************************************
*/

/************************ (C) COPYRIGHT FMD *****END OF FILE ****/
