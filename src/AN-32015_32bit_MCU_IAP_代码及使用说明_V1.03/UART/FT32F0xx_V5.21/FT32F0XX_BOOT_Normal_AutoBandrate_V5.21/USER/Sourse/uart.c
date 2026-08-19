/**
	******************************************************************************
	* @file 		uart.c
	* @author 	    FMD AE
	* @brief 		uart body
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
#include "uart.h"

/* Private Constant --------------------------------------------------------------*/
/* Public Constant ---------------------------------------------------------------*/
/* Private typedef ---------------------------------------------------------------*/
/* Private define ----------------------------------------------------------------*/
/* Private variables -------------------------------------------------------------*/
/* Public variables --------------------------------------------------------------*/
/* Private function prototypes ---------------------------------------------------*/
/* Public function ------ --------------------------------------------------------*/
/******************************************************************************
  * @brief  USART1_Configuration program.
  * @param  None   
  * @note   USART1 initialization function
  * @retval None
  *****************************************************************************
*/
void USART1_Configuration(void)
{  
    GPIO_InitTypeDef  GPIO_InitStructure;
    USART_InitTypeDef USART_InitStructure;
          
    RCC_AHBPeriphClockCmd( RCC_AHBPeriph_GPIOA, ENABLE);
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_USART1, ENABLE );
          
    /*
    *  USART1_TX -> PA9 , USART1_RX ->  PA10
    */                                
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_9|GPIO_Pin_10;                 
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF; 
    GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
    GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_UP;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz; 
    GPIO_Init(GPIOA, &GPIO_InitStructure);        

    GPIO_PinAFConfig(GPIOA,GPIO_PinSource9,GPIO_AF_1);
    GPIO_PinAFConfig(GPIOA,GPIO_PinSource10,GPIO_AF_1); 
      
      
    USART_InitStructure.USART_BaudRate = 115200;
    USART_InitStructure.USART_WordLength = USART_WordLength_8b;//8
    USART_InitStructure.USART_StopBits = USART_StopBits_1;//1
    USART_InitStructure.USART_Parity = USART_Parity_No;//n
    USART_InitStructure.USART_HardwareFlowControl = USART_HardwareFlowControl_None;
    USART_InitStructure.USART_Mode = USART_Mode_Rx | USART_Mode_Tx;
    USART_Init(USART1, &USART_InitStructure); 
    
}			
/******************************************************************************
  * @brief  AutoBauRate_StartBitMethod program.
  * @param  None   
  * @note   
  * @retval None
  *****************************************************************************
*/
void AutoBauRate_StartBitMethod(void)
{   
	
    NVIC_InitTypeDef NVIC_InitStructure;
    USART_AutoBaudRateConfig(USART1, USART_AutoBaudRate_StartBit);  

    USART_AutoBaudRateCmd(USART1, ENABLE);  

        
    NVIC_InitStructure.NVIC_IRQChannel = USART1_IRQn; 
    NVIC_InitStructure.NVIC_IRQChannelPriority = 2;    
    NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;    
    NVIC_Init(&NVIC_InitStructure);

    USART_ITConfig(USART1,USART_IT_RXNE,ENABLE); 

    USART_Cmd(USART1, ENABLE);
}


/******************************************************************************
  * @brief  UART1_send_byte program.
  * @param  None   
  * @note   
  * @retval None
  *****************************************************************************
*/
void UART1_send_byte(uint8_t byte) 
{
	while(!((USART1->ISR)&(1<<7)));
	USART1->TDR=byte;	
}		
/******************************************************************************
  * @brief  UART1_Send program.
  * @param  Buffer: data buff
  * @param  Length: data Length
  * @note   
  * @retval None
  *****************************************************************************
*/
void UART1_Send(uint8_t *Buffer, uint8_t Length)
{
	while(Length != 0)
	{
		while(!((USART1->ISR)&(1<<7)));
		USART1->TDR= *Buffer;
		Buffer++;
		Length--;
	}
}

/* Private function ------ -------------------------------------------------------*/

/************************ (C) COPYRIGHT FMD *****END OF FILE ****/
		
