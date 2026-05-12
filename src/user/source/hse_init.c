#include "hse_init.h"
#include "ft32f0xx_rcc.h"
#include "ft32f0xx_flash.h"
#include "ft32f0xx.h"

/**
 * @brief 初始化HSE（外部8MHz晶振）并配置为系统时钟
 * @note  参考HSE_Project例程实现
 *        系统时钟配置：HSE(8MHz) -> PLL(x9) -> 72MHz
 */
void HSE_Init(void)
{
    __IO uint32_t StartUpCounter = 0, HSEStatus = 0;

    /* 使能Flash预取和设置Flash延迟（72MHz需要2个等待周期） */
    FLASH->ACR = FLASH_ACR_PRFTBE | ((uint32_t)0x00000002);

    /* 使能HSE */
    RCC_HSEConfig(RCC_HSE_ON);

    /* 等待HSE就绪 */
    do
    {
        HSEStatus = RCC_GetFlagStatus(RCC_FLAG_HSERDY);
        StartUpCounter++;
    } while ((HSEStatus == RESET) && (StartUpCounter != HSE_STARTUP_TIMEOUT));

    if (RCC_GetFlagStatus(RCC_FLAG_HSERDY) != RESET)
    {
        HSEStatus = (uint32_t)0x01;
    }
    else
    {
        HSEStatus = (uint32_t)0x00;
    }

    if (HSEStatus == (uint32_t)0x01)
    {
        /* HCLK = SYSCLK */
        RCC_HCLKConfig(RCC_SYSCLK_Div1);

        /* PCLK = HCLK */
        RCC_PCLKConfig(RCC_HCLK_Div1);

        /* 配置PLL：HSE作为PLL源，倍频9倍 */
        RCC_PLLConfig(RCC_PLLSource_HSE, RCC_PLLMul_9);

        /* 使能PLL */
        RCC_PLLCmd(ENABLE);

        /* 等待PLL就绪 */
        while (RCC_GetFlagStatus(RCC_FLAG_PLLRDY) == RESET)
            ;

        /* 选择PLL作为系统时钟源 */
        RCC_SYSCLKConfig(RCC_SYSCLKSource_PLLCLK);

        /* 等待PLL被用作系统时钟源 */
        while (RCC_GetSYSCLKSource() != 0x08)
            ;

        /* 更新SystemCoreClock变量 */
        SystemCoreClockUpdate();
    }
    else
    {
        /* HSE启动失败，可以在这里添加错误处理代码 */
        /* 注意：如果HSE启动失败，系统可能无法正常工作 */
    }
}

