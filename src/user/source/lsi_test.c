#include "lsi_test.h"
#include "ft32f0xx_gpio.h"
#include "ft32f0xx_rcc.h"

/**
 * @brief 初始化HSE时钟输出测试模块
 * @note  使用MCO功能将HSE时钟输出到PA8引脚
 *        方法：配置PA8为MCO功能，选择HSE作为时钟源
 *        输出频率：HSE频率（8MHz），可通过预分频器调整
 *        
 *        使用方法：
 *        1. 调用 LSI_Test_Init() 初始化（函数名保持兼容）
 *        2. 用示波器测量PA8引脚的频率
 *        3. 测量到的频率即为实际HSE频率
 *        4. 根据测量结果调整RTC预分频器配置
 */
void LSI_Test_Init(void)
{
    /* 使能HSE时钟（外部8MHz晶振） */
    RCC_HSEConfig(RCC_HSE_ON);
    
    /* 等待HSE就绪 */
    while (RCC_GetFlagStatus(RCC_FLAG_HSERDY) == RESET)
        ;
    
    /* 使能GPIOA时钟 */
    RCC_AHBPeriphClockCmd(RCC_AHBPeriph_GPIOA, ENABLE);
    
    /* 配置PA8为AF模式（MCO功能） */
    GPIO_InitTypeDef gpio_init;
    GPIO_StructInit(&gpio_init);
    gpio_init.GPIO_Pin = GPIO_Pin_8;
    gpio_init.GPIO_Mode = GPIO_Mode_AF;
    gpio_init.GPIO_Speed = GPIO_Speed_50MHz;
    gpio_init.GPIO_OType = GPIO_OType_PP;
    gpio_init.GPIO_PuPd = GPIO_PuPd_NOPULL;
    GPIO_Init(GPIOA, &gpio_init);
    
    /* 配置PA8为AF0（MCO功能） */
    GPIO_PinAFConfig(GPIOA, GPIO_PinSource8, GPIO_AF_0);
    
    /* 配置MCO输出HSE时钟，预分频器设为64，输出频率 = 8MHz / 64 = 125kHz，便于示波器测量 */
    /* 注意：如果需要直接输出8MHz，可以改为RCC_MCOPrescaler_1 */
    RCC_MCOConfig(RCC_MCOSource_HSE, RCC_CFGR_MCO_PRE_32);
}

/**
 * @brief 停止LSI时钟输出测试
 */
void LSI_Test_Stop(void)
{
    /* 禁用MCO输出 */
    RCC_MCOConfig(RCC_MCOSource_NoClock, RCC_MCOPrescaler_1);
    
    /* 将PA8恢复为普通输出模式 */
    GPIO_InitTypeDef gpio_init;
    GPIO_StructInit(&gpio_init);
    gpio_init.GPIO_Pin = GPIO_Pin_8;
    gpio_init.GPIO_Mode = GPIO_Mode_OUT;
    gpio_init.GPIO_Speed = GPIO_Speed_2MHz;
    gpio_init.GPIO_OType = GPIO_OType_PP;
    gpio_init.GPIO_PuPd = GPIO_PuPd_NOPULL;
    GPIO_Init(GPIOA, &gpio_init);
    GPIO_ResetBits(GPIOA, GPIO_Pin_8);
}

