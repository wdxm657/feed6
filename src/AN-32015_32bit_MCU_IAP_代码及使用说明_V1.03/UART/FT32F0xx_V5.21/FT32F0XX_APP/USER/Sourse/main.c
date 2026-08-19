/**
	******************************************************************************
	* @file 		main.c
	* @author 	    FMD AE
	* @brief 		Main program body
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
#include "main.h"

/* Private Constant --------------------------------------------------------------*/
/* Public Constant ---------------------------------------------------------------*/
/* Private typedef ---------------------------------------------------------------*/
/* Private define ----------------------------------------------------------------*/
/* Private variables -------------------------------------------------------------*/
uint8_t LED_Sta;

/* Public variables --------------------------------------------------------------*/
uint8_t LED_Blink_Timer;

/* Private function prototypes ---------------------------------------------------*/
static void SystemCoreClockSetHSI(void);
static void TIM14Config(void);
static void POWER_LED_Init(void);
static void POWER_LED_Blink(void);

/* Public function ------ --------------------------------------------------------*/
/******************************************************************************
  * @brief  main program.
  * @param  None
  * @note 
  * @retval None
  *****************************************************************************
*/
int main(void)
{
    SystemCoreClockSetHSI();
    __enable_irq();

    /* USART1 for APP Updata */
    USART1_Configuration();  
    AutoBauRate_StartBitMethod();

    POWER_LED_Init();
    TIM14Config();

    while(1)
    {
        POWER_LED_Blink();
    }
}

/******************************************************************************
  * @brief  SystemCoreClockSetHSI program.
  * @param  None
  * @note   Set HSI48 as sysclk
  * @retval None
  *****************************************************************************
*/
static void SystemCoreClockSetHSI(void) 
{
    /* HSI48 */
    FLASH->ACR = FLASH_ACR_PRFTBE | FLASH_ACR_LATENCY;
	
	RCC_HSI48Cmd(ENABLE);
	while(RCC_GetFlagStatus(RCC_FLAG_HSI48RDY) != SET);
	
	RCC_SYSCLKConfig(RCC_SYSCLKSource_HSI48);
	
	//HSI48 as sysclk
	while(RCC_GetSYSCLKSource() != 0x0C);
	
	RCC_HCLKConfig(RCC_SYSCLK_Div1);
	
	RCC_PCLKConfig(RCC_HCLK_Div1);
	
	/* Update SystemCoreClock */
	SystemCoreClockUpdate();
}
/******************************************************************************
  * @brief  TIM14Config program.
  * @param  None
  * @note   
  * @retval None
  *****************************************************************************
*/
static void TIM14Config(void)
{
	NVIC_InitTypeDef NVIC_InitStructure;
	TIM_TimeBaseInitTypeDef  TIM_TimeBaseStructure;
	RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM14,ENABLE);		
	TIM_DeInit(TIM14);
    TIM_TimeBaseStructInit(&TIM_TimeBaseStructure);
	TIM_TimeBaseStructure.TIM_Period=10000;     //ARR value
	TIM_TimeBaseStructure.TIM_Prescaler=48-1;   //10ms
	TIM_TimeBaseStructure.TIM_CounterMode=TIM_CounterMode_Up;  
	TIM_TimeBaseInit(TIM14, &TIM_TimeBaseStructure);
	TIM_ITConfig(TIM14,TIM_IT_Update,ENABLE);
	TIM_Cmd(TIM14, ENABLE);

	NVIC_InitStructure.NVIC_IRQChannel = TIM14_IRQn;  
	NVIC_InitStructure.NVIC_IRQChannelPriority = 3;    
	NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;   
	NVIC_Init(&NVIC_InitStructure);

}
/******************************************************************************
  * @brief  POWER_LED_Init program.
  * @param  None
  * @note   
  * @retval None
  *****************************************************************************
*/
static void POWER_LED_Init(void)
{
	GPIO_InitTypeDef GPIO_InitStructure;

	RCC_AHBPeriphClockCmd(RCC_AHBPeriph_GPIOC, ENABLE);	
	/* Configure all the GPIOA in Input Floating mode */
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_13;            //
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_10MHz;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_OUT;
	GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
	GPIO_Init(GPIOC, &GPIO_InitStructure);
}


/******************************************************************************
  * @brief  POWER_LED_Blink program.
  * @param  None
  * @note   
  * @retval None
  *****************************************************************************
*/
static void POWER_LED_Blink(void)
{
  if(LED_Blink_Timer>200)
  {
    LED_Blink_Timer = 0;
    if(LED_Sta==0)
    {
      GPIO_SetBits(GPIOC,GPIO_Pin_13);
      LED_Sta = 1;
    }
    else
    {
      GPIO_ResetBits(GPIOC,GPIO_Pin_13);
      LED_Sta = 0;
    }
  }
}

/************************ (C) COPYRIGHT Fremont Micro Devices *****END OF FILE****/
