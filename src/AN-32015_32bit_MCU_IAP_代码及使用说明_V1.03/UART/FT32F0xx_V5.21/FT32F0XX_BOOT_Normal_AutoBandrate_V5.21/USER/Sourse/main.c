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
uint8_t LED_Sta __attribute__((at(0x20000104)));
/* Public variables --------------------------------------------------------------*/
uint8_t LED_Blink_Timer __attribute__((at(0x20000102)));
pFunction Jump_To_Application __attribute__((at(0x20000108)));
uint32_t JumpAddress __attribute__((at(0x2000010C)));
uint8_t flash_data[519];

/* Private function prototypes ---------------------------------------------------*/
static void SystemCoreClockSetHSI(void);
static void TIM14Config(void);
static void POWER_LED_Init(void);
static void POWER_LED_Blink(void);
static uint8_t data_check_erase_or_not(uint32_t addr, uint8_t mode);

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

    if(READ_FLASH(ApplicaflagAddress)!=0x66)
    {
        uint32_t Address;
        
        /* USART1 for APP Updata */	
        USART1_Configuration();  
        AutoBauRate_StartBitMethod();
        POWER_LED_Init();	

        TIM14Config();	  
        
        while(1)
        {
            POWER_LED_Blink();
            if(cmd_updata_flag == 1)
            {
                switch(Iap_cmd)
                {
                    case CMD_W_FLASH:   //Write flash command
                        Iap_cmd = 0;
                        Address=(flash_data[1] << 24)+(flash_data[2] << 16)+(flash_data[3] << 8)+ flash_data[4];
                        if( (Address < ApplicationAddress) || (Address > FLASH_END_ADDR))
                        {
                            UART1_send_byte(0x44);//Error
                            break;
                        }
                        if ( ((Address & FLASH_MAX_SIZE) % FLASH_PAGE_SIZE ) == 0)   //page addr 
                        {
                            FLASH_Erase_OnePage(Address); 
                        }

                        FLASH_WriteWord(Address, flash_data+5 ,0x200);

                        if(data_check_erase_or_not(Address,1))
                        {
                            UART1_send_byte(NOP_DATA);
                        }
                        else
                        {
                            UART1_send_byte(0x44);//Error
                        }  
                       break;
                    
                    case CMD_S_USR:              //Wait until the 0x00 byte is sent to complete a soft reset
                        Iap_cmd = 0;
                        uint8_t updata_flag[1];
                        updata_flag[0]=0x66; 
                        FLASH_WriteWord(ApplicaflagAddress,updata_flag,1);
                        UART1_send_byte(NOP_DATA);
                        while(!((USART1->ISR)&(1<<6)));
                        __disable_irq();
                        NVIC_SystemReset();//The reset function
                        break;
                    default:   

                        break;
                }
            
            }
        }
    }
    else
    {
        if (((*(__IO uint32_t*)ApplicationAddress) & 0x2FFE0000 ) == 0x20000000)
        {
            __disable_irq();	
        //	NVIC->ICER[0] = 0xFFFFFFFF;	
            /* Jump to user application */
            JumpAddress = *(__IO uint32_t*) (ApplicationAddress + 4);
            Jump_To_Application = (pFunction) JumpAddress;
            /* Initialize user application's Stack Pointer */
            __set_MSP(*(__IO uint32_t*) ApplicationAddress);
            Jump_To_Application();
        }
    }
}
/******************************************************************************
  * @brief  CRC16_CCITT program.
  * @param  data: data buff
  * @param  datalen: data length
  * @note   x16+x12+x5+1
  *         Width:	16 
  *         Poly:    0x1021   //  0001 0000 0010 0001  Reverse the byte: 1000 0100 0000 1000     0x8408
  *         Init:    0x0000 
  *         Refin:   True 
  *         Refout:  True 
  *         Xorout:  0x0000 
  *         Alias:   DOW-CRC,CRC-8/IBUTTON 
  * @retval None
  *****************************************************************************
*/
uint16_t CRC16_CCITT(uint8_t *data, uint16_t datalen)
{
	uint16_t wCRCin = 0x0000;
	uint16_t wCPoly = 0x8408;
	uint8_t i;
	while (datalen--) 	
	{
		wCRCin ^= *(data++);
		for(i = 0;i < 8;i++)
		{
			if(wCRCin & 0x01)
				wCRCin = (wCRCin >> 1) ^ wCPoly;
			else
				wCRCin = wCRCin >> 1;
		}
	}
	return (wCRCin);
}
/* Private function ------ -------------------------------------------------------*/

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
	
	GPIO_SetBits(GPIOC,GPIO_Pin_13);
}
/******************************************************************************
  * @brief  data_check_erase_or_not program.
  * @param  None
  * @note   Check whether the chip data is consistent with the data to be written and whether it needs to be erased
  * @retval None
  *****************************************************************************
*/ 
static uint8_t data_check_erase_or_not(uint32_t addr, uint8_t mode)
{
    uint16_t i;
    uint8_t read_data[512];
	
	Flash_Read(addr,read_data,512);
	
	for(i=0;i<0x200;i++)                         
	{  
		if(mode == 0)
		{
			if((read_data[i] & flash_data[i+5]) != flash_data[i+5])
			{
				return 0;
			}  
		}
		else
		{
			if(read_data[i] != flash_data[i+5]) 
			{
				return 0;
			} 
		}
	 }
	return 1;
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
  if(LED_Blink_Timer>30)
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
/************************ (C) COPYRIGHT FMD *****END OF FILE ****/

