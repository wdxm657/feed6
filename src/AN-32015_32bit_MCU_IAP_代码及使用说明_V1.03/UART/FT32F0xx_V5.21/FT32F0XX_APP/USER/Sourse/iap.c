/**
	******************************************************************************
	* @file 		iap.c
	* @author 	    FMD AE
	* @brief 		iap body
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
#include "iap.h"

/* Private Constant --------------------------------------------------------------*/
/* Public Constant ---------------------------------------------------------------*/
/* Private typedef ---------------------------------------------------------------*/
/* Private define ----------------------------------------------------------------*/
/* Private variables -------------------------------------------------------------*/
/* Public variables --------------------------------------------------------------*/
/* Private function prototypes ---------------------------------------------------*/

/* Public function ------ --------------------------------------------------------*/
/******************************************************************************
  * @brief  FLASH_Erase_OnePage program.
  * @param  None
  * @note 
  * @retval None
  *****************************************************************************
*/
void FLASH_Erase_OnePage(uint32_t Addr)      
{
	/*Unlock FLASH*/
	FLASH_Unlock(); 
    
	/* Clear All pending flags */
	FLASH_ClearFlag(FLASH_FLAG_BSY | FLASH_FLAG_EOP | FLASH_FLAG_PGERR | FLASH_FLAG_WRPERR);  
	FLASH_ErasePage(Addr);
	FLASH_Lock();
}
/******************************************************************************
  * @brief  FLASH_WriteWord program.
  * @param  addr: The page address in program memory to be erased.
  * @param  dataBuf:    data buff
  * @param  Byte_Num:   the num byte of the data
  * @note   
  * @retval None
  *****************************************************************************
*/
void FLASH_WriteWord(uint32_t addr , uint8_t *dataBuf , uint32_t Byte_Num)
{
	uint32_t i,OneWord;
    
	/*Unlock FLASH*/
	FLASH_Unlock(); 
    
	/* Clear All pending flags */
	FLASH_ClearFlag(FLASH_FLAG_BSY | FLASH_FLAG_EOP | FLASH_FLAG_PGERR | FLASH_FLAG_WRPERR);  
	for(i=0;i<Byte_Num;i=i+4)
	{
		OneWord=(dataBuf[3+i] << 24)+(dataBuf[2+i] << 16)+(dataBuf[1+i] << 8)+ dataBuf[i];
		IWDG_ReloadCounter();
		FLASH_ProgramWord(addr, OneWord);
		addr += 4;
	}
	FLASH_Lock();
}
/******************************************************************************
  * @brief  Flash_Read program.
  * @param  iAddress: 
  * @param  buf:    
  * @param  iNbrToRead:   
  * @note   
  * @retval None
  *****************************************************************************
*/
void Flash_Read(uint32_t iAddress, uint8_t *buf, int32_t iNbrToRead)
{
	int32_t i = 0;
	while(i < iNbrToRead ) 
	{
	   *(buf + i) = *(__IO uint8_t*) iAddress++;
	   i++;
	}
}
/* Private function ------ -------------------------------------------------------*/

/************************ (C) COPYRIGHT FMD *****END OF FILE ****/




