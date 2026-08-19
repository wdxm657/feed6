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
static void JumpToAddr(uint32_t addr);

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
		JumpToAddr(NMI_Handler_Adderes);
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
		JumpToAddr(HardFault_Handler_Adderes);
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
		JumpToAddr(SVC_Handler_Adderes);
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
		JumpToAddr(PendSV_Handler_Adderes);
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
		JumpToAddr(SysTick_Handler_Adderes);
	}
}

/******************************************************************************/
/*                 FT32F0xx Peripherals Interrupt Handlers                   */
/*  Add here the Interrupt Handler for the used peripheral(s) (PPP), for the  */
/*  available peripheral interrupt handler's name please refer to the startup */
/*  file (startup_ft32f0xx.s).                                               */
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
  * @brief  This function handles WWDG Handler.
  * @param  None
  * @note  
  * @retval None
  *****************************************************************************
*/
void WWDG_IRQHandler(void)
{
	if(READ_FLASH(ApplicaflagAddress)==0x66)
	{	
		JumpToAddr(WWDG_IRQHandler_Adderes);
	}
}

/******************************************************************************
  * @brief  This function handles PVD Handler.
  * @param  None
  * @note  
  * @retval None
  *****************************************************************************
*/
void PVD_VDDIO_IRQHandler(void)
{
	if(READ_FLASH(ApplicaflagAddress)==0x66)
	{	
		JumpToAddr(PVD_VDDIO_IRQHandler_Adderes);
	}
}

/******************************************************************************
  * @brief  This function handles RTC Handler.
  * @param  None
  * @note  
  * @retval None
  *****************************************************************************
*/
void RTC_IRQHandler(void)
{
	if(READ_FLASH(ApplicaflagAddress)==0x66)
	{	
		JumpToAddr(RTC_IRQHandler_Adderes);
	}
}

/******************************************************************************
  * @brief  This function handles FLASH Handler.
  * @param  None
  * @note  
  * @retval None
  *****************************************************************************
*/
void FLASH_IRQHandler(void)
{
	if(READ_FLASH(ApplicaflagAddress)==0x66)
	{	
		JumpToAddr(FLASH_IRQHandler_Adderes);
	}
}

/******************************************************************************
  * @brief  This function handles RCC Handler.
  * @param  None
  * @note  
  * @retval None
  *****************************************************************************
*/
void RCC_IRQHandler(void)
{
	if(READ_FLASH(ApplicaflagAddress)==0x66)
	{	
		JumpToAddr(RCC_IRQHandler_Adderes);
	}
}

/******************************************************************************
  * @brief  This function handles EXTI0_1 Handler.
  * @param  None
  * @note  
  * @retval None
  *****************************************************************************
*/
void EXTI0_1_IRQHandler(void)
{
	if(READ_FLASH(ApplicaflagAddress)==0x66)
	{	
		JumpToAddr(EXTI0_1_IRQHandler_Adderes);
	}
}

/******************************************************************************
  * @brief  This function handles EXTI2_3 Handler.
  * @param  None
  * @note  
  * @retval None
  *****************************************************************************
*/
void EXTI2_3_IRQHandler(void)
{
	if(READ_FLASH(ApplicaflagAddress)==0x66)
	{	
		JumpToAddr(EXTI2_3_IRQHandler_Adderes);
	}
}

/******************************************************************************
  * @brief  This function handles EXTI4_15 Handler.
  * @param  None
  * @note  
  * @retval None
  *****************************************************************************
*/
void EXTI4_15_IRQHandler(void)
{
	if(READ_FLASH(ApplicaflagAddress)==0x66)
	{	
		JumpToAddr(EXTI4_15_IRQHandler_Adderes);
	}
}

/******************************************************************************
  * @brief  This function handles DMA1_Channel1 Handler.
  * @param  None
  * @note  
  * @retval None
  *****************************************************************************
*/
void DMA1_Channel1_IRQHandler(void)
{
	if(READ_FLASH(ApplicaflagAddress)==0x66)
	{	
		JumpToAddr(DMA1_Channel1_IRQHandler_Adderes);
	}
}

/******************************************************************************
  * @brief  This function handles DMA1_Channel2_3 Handler.
  * @param  None
  * @note  
  * @retval None
  *****************************************************************************
*/
void DMA1_Channel2_3_IRQHandler(void)
{
	if(READ_FLASH(ApplicaflagAddress)==0x66)
	{	
		JumpToAddr(DMA1_Channel2_3_IRQHandler_Adderes);
	}
}

/******************************************************************************
  * @brief  This function handles DMA1_Channel4_5 Handler.
  * @param  None
  * @note  
  * @retval None
  *****************************************************************************
*/
void DMA1_Channel4_5_IRQHandler(void)
{
	if(READ_FLASH(ApplicaflagAddress)==0x66)
	{	
		JumpToAddr(DMA1_Channel4_5_IRQHandler_Adderes);
	}
}

/******************************************************************************
  * @brief  This function handles ADC Handler.
  * @param  None
  * @note  
  * @retval None
  *****************************************************************************
*/
void ADC1_IRQHandler(void)
{
	if(READ_FLASH(ApplicaflagAddress)==0x66)
	{	
		JumpToAddr(ADC1_IRQHandler_Adderes);
	}
}

/******************************************************************************
  * @brief  This function handles TIM1_BRK Handler.
  * @param  None
  * @note  
  * @retval None
  *****************************************************************************
*/
void TIM1_BRK_UP_TRG_COM_IRQHandler(void)
{
	if(READ_FLASH(ApplicaflagAddress)==0x66)
	{	
		JumpToAddr(TIM1_BRK_UP_TRG_COM_IRQHandler_Adderes);
	}
}

/******************************************************************************
  * @brief  This function handles TIM1_CC Handler.
  * @param  None
  * @note  
  * @retval None
  *****************************************************************************
*/
void TIM1_CC_IRQHandler(void)
{
	if(READ_FLASH(ApplicaflagAddress)==0x66)
	{	
		JumpToAddr(TIM1_CC_IRQHandler_Adderes);
	}
}

/******************************************************************************
  * @brief  This function handles TIM3 Handler.
  * @param  None
  * @note  
  * @retval None
  *****************************************************************************
*/
void TIM3_IRQHandler(void)
{
	if(READ_FLASH(ApplicaflagAddress)==0x66)
	{	
		JumpToAddr(TIM3_IRQHandler_Adderes);
	}
}
/******************************************************************************
  * @brief  This function handles TIM6 Handler.
  * @param  None
  * @note  
  * @retval None
  *****************************************************************************
*/
void TIM6_IRQHandler(void)
{
	if(READ_FLASH(ApplicaflagAddress)==0x66)
	{	
		JumpToAddr(TIM6_IRQHandler_Adderes);
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
		JumpToAddr(TIM14_IRQHandler_Adderes);
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
  * @brief  This function handles TIM15 Handler.
  * @param  None
  * @note  
  * @retval None
  *****************************************************************************
*/
void TIM15_IRQHandler(void)
{
	if(READ_FLASH(ApplicaflagAddress)==0x66)
	{	
		JumpToAddr(TIM15_IRQHandler_Adderes);
	}
}

/******************************************************************************
  * @brief  This function handles TIM16 Handler.
  * @param  None
  * @note  
  * @retval None
  *****************************************************************************
*/
void TIM16_IRQHandler(void)
{
	if(READ_FLASH(ApplicaflagAddress)==0x66)
	{	
		JumpToAddr(TIM16_IRQHandler_Adderes);
	}
}

/******************************************************************************
  * @brief  This function handles TIM17 Handler.
  * @param  None
  * @note  
  * @retval None
  *****************************************************************************
*/
void TIM17_IRQHandler(void)
{
	if(READ_FLASH(ApplicaflagAddress)==0x66)
	{	
		JumpToAddr(TIM17_IRQHandler_Adderes);
	}
}

/******************************************************************************
  * @brief  This function handles I2C1 Handler.
  * @param  None
  * @note  
  * @retval None
  *****************************************************************************
*/
void I2C1_IRQHandler(void)
{
	if(READ_FLASH(ApplicaflagAddress)==0x66)
	{	
		JumpToAddr(I2C1_IRQHandler_Adderes);
	}
}

/******************************************************************************
  * @brief  This function handles I2C2 Handler.
  * @param  None
  * @note  
  * @retval None
  *****************************************************************************
*/
void I2C2_IRQHandler(void)
{
	if(READ_FLASH(ApplicaflagAddress)==0x66)
	{	
		JumpToAddr(I2C2_IRQHandler_Adderes);
	}
}

/******************************************************************************
  * @brief  This function handles SPI1 Handler.
  * @param  None
  * @note  
  * @retval None
  *****************************************************************************
*/
void SPI1_IRQHandler(void)
{
	if(READ_FLASH(ApplicaflagAddress)==0x66)
	{	
		JumpToAddr(SPI1_IRQHandler_Adderes);
	}
}

/******************************************************************************
  * @brief  This function handles SPI2 Handler.
  * @param  None
  * @note  
  * @retval None
  *****************************************************************************
*/
void SPI2_IRQHandler(void)
{
	if(READ_FLASH(ApplicaflagAddress)==0x66)
	{	
		JumpToAddr(SPI2_IRQHandler_Adderes);
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
		JumpToAddr(USART1_IRQHandler_Adderes);
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
  * @brief  This function handles USART2 Handler.
  * @param  None
  * @note  
  * @retval None
  *****************************************************************************
*/
void USART2_IRQHandler(void)
{
	if(READ_FLASH(ApplicaflagAddress)==0x66)
	{	
		JumpToAddr(USART2_IRQHandler_Adderes);
	}
}

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
		JumpToAddr(USB_IRQHandler_Adderes);
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

/* Private function ------ -------------------------------------------------------*/
/******************************************************************************
  * @brief  JumpToAddr.
  * @param  addr
  * @note  
  * @retval None
  *****************************************************************************
*/
static void JumpToAddr(uint32_t addr)
{
	JumpAddress = *(__IO uint32_t*) addr;
	Jump_To_Application = (pFunction) JumpAddress;
	Jump_To_Application();	
}

/************************ (C) COPYRIGHT FMD *****END OF FILE ****/
