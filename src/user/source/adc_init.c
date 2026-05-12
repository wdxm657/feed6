#include "main.h"
#include "adc_init.h"
#include <stdint.h>
#include "ft32f0xx_rcc.h"
#include "ft32f0xx_adc.h"
#include "ft32f0xx_dma.h"
#include "ft32f0xx_misc.h"

/* ADC DMA配置 */
#define ADC1_DR_Address 0x40012440U /* ADC1数据寄存器地址 */
#define ADC_DMA_BUFFER_SIZE 2U       /* 三通道：电池、电机、内部基准电压 */

/* 内部基准电压校准值（从Flash读取） */
#define VREFINT_CAL ((*(uint16_t *)0x1FFFF7BA) & 0x0FFF)

/* DMA缓冲区：存储三个通道的ADC值
 * 注意：扫描方向为Backward（从高通道到低通道），所以：
 * [0]=Channel17 (内部基准电压 Vrefint)
 * [1]=Channel3 (电机 PA3)
 * [2]=Channel0 (电池 PA0)
 */
static volatile uint16_t s_adc_dma_buffer[ADC_DMA_BUFFER_SIZE] = {0U, 0U};

/* 当前ADC值（由DMA中断更新） */
static volatile uint16_t s_battery_adc_raw = 0U;
static volatile uint16_t s_motor_adc_raw = 0U;
static volatile uint16_t s_vrefint_adc_raw = 0U;

/* 当前VDDA电压值（mV），由内部基准电压计算得出 */
static volatile uint16_t s_vdda_voltage_mv = 3300U; /* 默认值3.3V */

void ADC_Init_All(void)
{
	ADC_InitTypeDef adc_init;
	DMA_InitTypeDef dma_init;
	NVIC_InitTypeDef nvic_init;

	/* 使能时钟 */
	RCC_AHBPeriphClockCmd(RCC_AHBPeriph_DMA1, ENABLE);
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_ADC1, ENABLE);
	ADC->CR2 = 1;
	/* 复位ADC和DMA */
	ADC_DeInit(ADC1);
	DMA_DeInit(DMA1_Channel1);

	/* 配置ADC为连续转换模式，扫描模式 */
	ADC_StructInit(&adc_init);
	adc_init.ADC_Resolution = ADC_Resolution_12b;
	adc_init.ADC_ContinuousConvMode = ENABLE; /* 连续转换模式 */
	adc_init.ADC_ExternalTrigConvEdge = ADC_ExternalTrigConvEdge_None;
	adc_init.ADC_DataAlign = ADC_DataAlign_Right;
	adc_init.ADC_ScanDirection = ADC_ScanDirection_Backward; /* 从高通道到低通道扫描 */
	ADC_Init(ADC1, &adc_init);

	/* 配置VDDA作为ADC参考电压 */
	ADC_VrefselConfig(ADC_Vrefsel_2_5V);

	/* 配置三个通道：PA0(Channel0)=电池, PA3(Channel3)=电机, Channel17=内部基准电压 */
	/* 注意：扫描方向为Backward，所以先配置高通道，后配置低通道 */
	/* 使能内部基准电压 */
	// ADC_VrefintCmd(ENABLE);
	
	// ADC_ChannelConfig(ADC1, ADC_Channel_17, ADC_SampleTime_239_5Cycles); /* 内部基准电压 Vrefint */
	ADC_ChannelConfig(ADC1, ADC_Channel_3, ADC_SampleTime_239_5Cycles);   /* 电机：PA3 */
	ADC_ChannelConfig(ADC1, ADC_Channel_0, ADC_SampleTime_239_5Cycles);    /* 电池：PA0 */

	/* ADC校准 */
	ADC_GetCalibrationFactor(ADC1);

	/* 配置ADC DMA请求为循环模式 */
	ADC_DMARequestModeConfig(ADC1, ADC_DMAMode_Circular);

	/* 使能ADC DMA */
	ADC_DMACmd(ADC1, ENABLE);

	/* 使能ADC并等待就绪 */
	ADC_Cmd(ADC1, ENABLE);
	while (ADC_GetFlagStatus(ADC1, ADC_FLAG_ADRDY) == RESET)
		;

	/* 配置DMA */
	dma_init.DMA_PeripheralBaseAddr = (uint32_t)ADC1_DR_Address;
	dma_init.DMA_MemoryBaseAddr = (uint32_t)s_adc_dma_buffer;
	dma_init.DMA_DIR = DMA_DIR_PeripheralSRC;
	dma_init.DMA_BufferSize = ADC_DMA_BUFFER_SIZE;
	dma_init.DMA_PeripheralInc = DMA_PeripheralInc_Disable;
	dma_init.DMA_MemoryInc = DMA_MemoryInc_Enable;
	dma_init.DMA_PeripheralDataSize = DMA_PeripheralDataSize_HalfWord;
	dma_init.DMA_MemoryDataSize = DMA_MemoryDataSize_HalfWord;
	dma_init.DMA_Mode = DMA_Mode_Circular; /* 循环模式 */
	dma_init.DMA_Priority = DMA_Priority_High;
	dma_init.DMA_M2M = DMA_M2M_Disable;
	DMA_Init(DMA1_Channel1, &dma_init);

	/* 使能DMA传输完成中断 */
	DMA_ITConfig(DMA1_Channel1, DMA_IT_TC, ENABLE);

	/* 配置NVIC */
	nvic_init.NVIC_IRQChannel = DMA1_Channel1_IRQn;
	nvic_init.NVIC_IRQChannelPriority = 1;
	nvic_init.NVIC_IRQChannelCmd = ENABLE;
	NVIC_Init(&nvic_init);

	/* 使能DMA */
	DMA_Cmd(DMA1_Channel1, ENABLE);

	/* 启动ADC转换 */
	ADC_StartOfConversion(ADC1);
}

void ADC_ReadBatteryRaw_enable(uint8_t enable)
{
	if (enable)
	{
		GPIO_ResetBits(GPIOA, GPIO_Pin_1);
	}
	else
	{
		GPIO_SetBits(GPIOA, GPIO_Pin_1);
	}
}

uint16_t ADC_ReadBatteryRaw(void)
{
	/* 返回DMA中断更新的最新值 */
	return s_battery_adc_raw;
}

uint16_t ADC_ReadMotorRaw(void)
{
	/* 返回DMA中断更新的最新值 */
	return s_motor_adc_raw;
}

/**
 * @brief 获取当前VDDA电压（mV）
 * @note 基于内部基准电压计算得出
 * @return VDDA电压值（mV），默认3300mV
 */
uint16_t ADC_GetVDDA(void)
{
	return s_vdda_voltage_mv;
}

/**
 * @brief 将电机ADC原始值转换为电压（mV）
 * @note 使用实际VDDA电压进行计算，而不是固定3.3V
 * @param adc_raw 电机ADC原始值
 * @return 电机电压（mV）
 */
uint16_t ADC_MotorRawToVoltage(uint16_t adc_raw)
{
	/* 使用实际VDDA计算：电压 = (ADC值 * VDDA) / 4095 */
	uint32_t voltage_mv = ((uint32_t)adc_raw * 2500U) / 0xFFFU;
	return (uint16_t)voltage_mv;
}

/**
 * @brief 停止ADC和DMA（用于低功耗模式）
 */
void ADC_Stop(void)
{
	/* 停止ADC转换 */
	ADC_Cmd(ADC1, DISABLE);
	
	/* 停止DMA */
	DMA_Cmd(DMA1_Channel1, DISABLE);
	
	/* 禁用ADC DMA */
	ADC_DMACmd(ADC1, DISABLE);
}

/**
 * @brief 恢复ADC和DMA（从低功耗模式唤醒后）
 */
void ADC_Resume(void)
{
	ADC_InitTypeDef adc_init;
	DMA_InitTypeDef dma_init;
	NVIC_InitTypeDef nvic_init;

	/* 使能时钟（确保时钟已使能） */
	RCC_AHBPeriphClockCmd(RCC_AHBPeriph_DMA1, ENABLE);
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_ADC1, ENABLE);
	ADC->CR2 = 1;
	/* 复位ADC和DMA（确保干净的状态） */
	ADC_DeInit(ADC1);
	DMA_DeInit(DMA1_Channel1);

	/* 重新配置ADC为连续转换模式，扫描模式 */
	ADC_StructInit(&adc_init);
	adc_init.ADC_Resolution = ADC_Resolution_12b;
	adc_init.ADC_ContinuousConvMode = ENABLE; /* 连续转换模式 */
	adc_init.ADC_ExternalTrigConvEdge = ADC_ExternalTrigConvEdge_None;
	adc_init.ADC_DataAlign = ADC_DataAlign_Right;
	adc_init.ADC_ScanDirection = ADC_ScanDirection_Backward; /* 从高通道到低通道扫描 */
	ADC_Init(ADC1, &adc_init);

	/* 配置VDDA作为ADC参考电压 */
	ADC_VrefselConfig(ADC_Vrefsel_2_5V);

	/* 重新配置ADC通道 */
	/* 使能内部基准电压 */
	// ADC_VrefintCmd(ENABLE);
	
	// ADC_ChannelConfig(ADC1, ADC_Channel_17, ADC_SampleTime_239_5Cycles); /* 内部基准电压 Vrefint */
	ADC_ChannelConfig(ADC1, ADC_Channel_3, ADC_SampleTime_239_5Cycles);   /* 电机：PA3 */
	ADC_ChannelConfig(ADC1, ADC_Channel_0, ADC_SampleTime_239_5Cycles);   /* 电池：PA0 */

	/* 重新校准ADC（Stop模式可能会影响校准） */
	ADC_GetCalibrationFactor(ADC1);

	/* 重新配置ADC DMA请求为循环模式 */
	ADC_DMARequestModeConfig(ADC1, ADC_DMAMode_Circular);

	/* 使能ADC DMA */
	ADC_DMACmd(ADC1, ENABLE);

	/* 使能ADC并等待就绪 */
	ADC_Cmd(ADC1, ENABLE);
	while (ADC_GetFlagStatus(ADC1, ADC_FLAG_ADRDY) == RESET)
		;

	/* 重新配置DMA（Stop模式可能会丢失DMA配置） */
	dma_init.DMA_PeripheralBaseAddr = (uint32_t)ADC1_DR_Address;
	dma_init.DMA_MemoryBaseAddr = (uint32_t)s_adc_dma_buffer;
	dma_init.DMA_DIR = DMA_DIR_PeripheralSRC;
	dma_init.DMA_BufferSize = ADC_DMA_BUFFER_SIZE;
	dma_init.DMA_PeripheralInc = DMA_PeripheralInc_Disable;
	dma_init.DMA_MemoryInc = DMA_MemoryInc_Enable;
	dma_init.DMA_PeripheralDataSize = DMA_PeripheralDataSize_HalfWord;
	dma_init.DMA_MemoryDataSize = DMA_MemoryDataSize_HalfWord;
	dma_init.DMA_Mode = DMA_Mode_Circular; /* 循环模式 */
	dma_init.DMA_Priority = DMA_Priority_High;
	dma_init.DMA_M2M = DMA_M2M_Disable;
	DMA_Init(DMA1_Channel1, &dma_init);

	/* 使能DMA传输完成中断 */
	DMA_ITConfig(DMA1_Channel1, DMA_IT_TC, ENABLE);

	/* 重新配置NVIC（确保中断配置正确） */
	nvic_init.NVIC_IRQChannel = DMA1_Channel1_IRQn;
	nvic_init.NVIC_IRQChannelPriority = 1;
	nvic_init.NVIC_IRQChannelCmd = ENABLE;
	NVIC_Init(&nvic_init);

	/* 清空DMA缓冲区和ADC值（避免读取到旧数据） */
	s_adc_dma_buffer[0] = 0U;
	s_adc_dma_buffer[1] = 0U;
	// s_adc_dma_buffer[2] = 0U;
	s_motor_adc_raw = 0U;
	s_battery_adc_raw = 0U;
	// s_vrefint_adc_raw = 0U;

	/* 使能DMA */
	DMA_Cmd(DMA1_Channel1, ENABLE);

	/* 启动ADC转换 */
	ADC_StartOfConversion(ADC1);
}

/**
 * @brief 计算VDDA电压（基于内部基准电压）
 * @note 使用内部基准电压校准值计算实际VDDA电压
 */
static void ADC_CalculateVDDA(void)
{
	if (s_vrefint_adc_raw == 0U)
	{
		return; /* 内部基准电压未采样，使用默认值 */
	}

	/* 计算VDDA：VDDA = (VREFINT_CAL * 3300) / Vref_Int_Value */
	/* VREFINT_CAL是工厂校准值，单位为ADC计数，在3.3V VDDA下测量 */
	uint32_t vdda_mv = ((uint32_t)VREFINT_CAL * 3300U) / s_vrefint_adc_raw;
	
	/* 限制VDDA范围（合理范围：2.0V - 4.0V） */
	// if (vdda_mv < 2000U)
	// {
	// 	vdda_mv = 2000U;
	// }
	// else if (vdda_mv > 4000U)
	// {
	// 	vdda_mv = 4000U;
	// }
	
	s_vdda_voltage_mv = (uint16_t)vdda_mv;
}

/**
 * @brief DMA1 Channel1中断处理函数（ADC DMA传输完成）
 * @note 在DMA传输完成中断中更新电池、电机和内部基准电压的ADC值，并计算VDDA
 */
void DMA1_Channel1_IRQHandler(void)
{
	if (DMA_GetITStatus(DMA1_IT_TC1) != RESET)
	{
		/* 清除DMA传输完成标志 */
		DMA_ClearITPendingBit(DMA1_IT_TC1);

		/* 更新ADC值
		 * 注意：扫描方向为Backward，所以：
		 * s_adc_dma_buffer[0] = Channel17 (内部基准电压 Vrefint)
		 * s_adc_dma_buffer[1] = Channel3 (电机 PA3)
		 * s_adc_dma_buffer[2] = Channel0 (电池 PA0)
		 */
		// s_vrefint_adc_raw = s_adc_dma_buffer[0];
		s_motor_adc_raw = s_adc_dma_buffer[0];
		s_battery_adc_raw = s_adc_dma_buffer[1];
		
		/* 计算VDDA电压（基于内部基准电压） */
		// ADC_CalculateVDDA();
	}
}

