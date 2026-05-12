#ifndef __ADC_INIT_H__
#define __ADC_INIT_H__

#ifdef __cplusplus
extern "C" {
#endif

#include "main.h"

void ADC_Init_All(void);
uint16_t ADC_ReadBatteryRaw(void);   /* PA0 */
uint16_t ADC_ReadMotorRaw(void);     /* PA3 */
void ADC_ReadBatteryRaw_enable(uint8_t enable);

/**
 * @brief 获取当前VDDA电压（mV）
 * @note 基于内部基准电压计算得出
 * @return VDDA电压值（mV），默认3300mV
 */
uint16_t ADC_GetVDDA(void);

/**
 * @brief 将电机ADC原始值转换为电压（mV）
 * @note 使用实际VDDA电压进行计算，而不是固定3.3V
 * @param adc_raw 电机ADC原始值
 * @return 电机电压（mV）
 */
uint16_t ADC_MotorRawToVoltage(uint16_t adc_raw);

/* ADC停止和恢复（用于低功耗模式） */
void ADC_Stop(void);  /* 停止ADC和DMA */
void ADC_Resume(void); /* 恢复ADC和DMA */

/* DMA中断处理函数（需要在中断文件中实现） */
void DMA1_Channel1_IRQHandler(void);
#ifdef __cplusplus
}
#endif

#endif /* __ADC_INIT_H__ */

