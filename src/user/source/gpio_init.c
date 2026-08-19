#include "gpio_init.h"
#include "ft32f0xx.h"
#include "ft32f0xx_gpio.h"
#include "ft32f0xx_rcc.h"
#include "led.h"
#include "log.h"

static void GPIO_ConfigAnalog(void);
static void GPIO_ConfigInputs(void);
static void GPIO_ConfigOutputs(void);
static uint8_t ChargeSignal_Filter(uint8_t raw, uint8_t *stable_state, uint8_t *counter);
static void ChargeSignals_Update(uint8_t *stby, uint8_t *chrg);

/**
 * @brief 初始化所有GPIO（不包括按键和LED，它们由各自模块初始化）
 */
void GPIO_Init_All(void)
{
    RCC_AHBPeriphClockCmd(RCC_AHBPeriph_GPIOA | RCC_AHBPeriph_GPIOB, ENABLE);

    GPIO_ConfigAnalog();
    GPIO_ConfigInputs();
    GPIO_ConfigOutputs();
}

static void GPIO_ConfigAnalog(void)
{
    GPIO_InitTypeDef gpio_init;

    GPIO_StructInit(&gpio_init);

    gpio_init.GPIO_Mode = GPIO_Mode_AN;
    gpio_init.GPIO_PuPd = GPIO_PuPd_NOPULL;

    /* PA0: 电池电压采样 -> 模拟输入 */
    gpio_init.GPIO_Pin = GPIO_Pin_0;
    GPIO_Init(GPIOA, &gpio_init);

    /* PA3: 电机驱动电压采样 -> 模拟输入 */
    gpio_init.GPIO_Pin = GPIO_Pin_3;
    GPIO_Init(GPIOA, &gpio_init);
}

static void GPIO_ConfigInputs(void)
{
    GPIO_InitTypeDef gpio_init;

    GPIO_StructInit(&gpio_init);
    gpio_init.GPIO_Mode = GPIO_Mode_IN;
    gpio_init.GPIO_PuPd = GPIO_PuPd_NOPULL;

    /* PA4: STDBY 输入 -> 上拉 */
    gpio_init.GPIO_Pin = GPIO_Pin_4;
    gpio_init.GPIO_PuPd = GPIO_PuPd_UP;
    GPIO_Init(GPIOA, &gpio_init);

    /* PA5: CHRG 输入 -> 上拉 */
    gpio_init.GPIO_Pin = GPIO_Pin_5;
    gpio_init.GPIO_PuPd = GPIO_PuPd_UP;
    GPIO_Init(GPIOA, &gpio_init);

    /* PB0: USB 检测 -> 浮空输入 */
    gpio_init.GPIO_Pin = GPIO_Pin_0;
    gpio_init.GPIO_PuPd = GPIO_PuPd_NOPULL;
    GPIO_Init(GPIOB, &gpio_init);

    /* PB3: 限位开关 -> 上拉输入 */
    gpio_init.GPIO_Pin = GPIO_Pin_3;
    gpio_init.GPIO_PuPd = GPIO_PuPd_UP;
    GPIO_Init(GPIOB, &gpio_init);

    /* 注意：PA7（按键2）和PB6（按键1）的GPIO初始化已移至key.c模块 */
}

static void GPIO_ConfigOutputs(void)
{
    GPIO_InitTypeDef gpio_init;

    GPIO_StructInit(&gpio_init);
    gpio_init.GPIO_Mode = GPIO_Mode_OUT;
    gpio_init.GPIO_OType = GPIO_OType_PP;
    gpio_init.GPIO_Speed = GPIO_Speed_2MHz;
    gpio_init.GPIO_PuPd = GPIO_PuPd_NOPULL;

    /* PA1: 电池电压AD读取控制引脚 -> 默认低 读取电池电压AD数据 */
    gpio_init.GPIO_Pin = GPIO_Pin_1;
    GPIO_Init(GPIOA, &gpio_init);
    GPIO_ResetBits(GPIOA, GPIO_Pin_1);

    /* PA6: TP4056 使能 -> 默认高 控制电池充电 */
    gpio_init.GPIO_Pin = GPIO_Pin_6;
    GPIO_Init(GPIOA, &gpio_init);
    GPIO_SetBits(GPIOA, GPIO_Pin_6);

    /* PA12/PA15: 电机驱动 -> 默认低 */
    gpio_init.GPIO_Pin = GPIO_Pin_12 | GPIO_Pin_15;
    GPIO_Init(GPIOA, &gpio_init);
    GPIO_ResetBits(GPIOA, GPIO_Pin_12 | GPIO_Pin_15);

    /* PB1: LC2202 电源开关 -> 默认低 控制WIFI模块电源 */
    gpio_init.GPIO_Pin = GPIO_Pin_1;
    GPIO_Init(GPIOB, &gpio_init);
    GPIO_ResetBits(GPIOB, GPIO_Pin_1);

    /* PB5: LM4871 开关 -> 默认低 控制麦克风电源 */
    gpio_init.GPIO_Pin = GPIO_Pin_5;
    GPIO_Init(GPIOB, &gpio_init);
    GPIO_ResetBits(GPIOB, GPIO_Pin_5);
}

static uint8_t usb_flag = 0;
static uint8_t stby_stable_state = 1U;
static uint8_t chrg_stable_state = 1U;
static uint8_t stby_filter_counter = 0U;
static uint8_t chrg_filter_counter = 0U;
#define CHARGE_SIGNAL_FILTER_COUNT 3U

static uint8_t ChargeSignal_Filter(uint8_t raw, uint8_t *stable_state, uint8_t *counter)
{
    if (raw == *stable_state)
    {
        *counter = 0U;
    }
    else
    {
        if (*counter < CHARGE_SIGNAL_FILTER_COUNT)
        {
            (*counter)++;
        }
        if (*counter >= CHARGE_SIGNAL_FILTER_COUNT)
        {
            *stable_state = raw;
            *counter = 0U;
        }
    }
    return *stable_state;
}

static void ChargeSignals_Update(uint8_t *stby, uint8_t *chrg)
{
    uint8_t raw_stby = GPIO_ReadInputDataBit(GPIOA, GPIO_Pin_4);
    uint8_t raw_chrg = GPIO_ReadInputDataBit(GPIOA, GPIO_Pin_5);

    stby_stable_state = ChargeSignal_Filter(raw_stby, &stby_stable_state, &stby_filter_counter);
    chrg_stable_state = ChargeSignal_Filter(raw_chrg, &chrg_stable_state, &chrg_filter_counter);

    if (stby != 0)
    {
        *stby = stby_stable_state;
    }
    if (chrg != 0)
    {
        *chrg = chrg_stable_state;
    }
}

void ChargeSignals_ReadFiltered(uint8_t *stby, uint8_t *chrg)
{
    ChargeSignals_Update(stby, chrg);
}

uint8_t Get_USB_Flag(void)
{
    return usb_flag;
}
/**
 * @brief USB检测处理
 */
void USB_Detect_Process(void)
{
    static uint8_t last_usb_state = 0;
    static uint8_t current_usb_state = 0;
    // 读取GPIOA的PA4和PA5的值（带滤波）
    uint8_t stby = 1U;
    uint8_t chrg = 1U;
    ChargeSignals_Update(&stby, &chrg);
    // 0 0 1
    // 0 1 1
    // 1 0 1
    // 1 1 0
    current_usb_state = (stby == 0 || chrg == 0) ? 1 : 0;
    // LOG_DEBUG("USB_Detect_Process: stby: %d, chrg: %d, current_usb_state: %d", stby, chrg, current_usb_state);
    if (current_usb_state == last_usb_state)
    {
        return;
    }
    if (current_usb_state)
    {
        // 有一个为低电平，则代表USB插入
        // LED_On(LED_CHARGE_GREEN);
        // LED_Off(LED_CHARGE_RED);
        mcu_dp_bool_update(DPID_CHARGE_STATE, 1);
        Battery_SetClosePowerMode(0U);
        usb_flag = 1;
        GPIO_SetBits(GPIOB, GPIO_Pin_1);
    }
    else
    {
        // USB拔出
        mcu_dp_bool_update(DPID_CHARGE_STATE, 0);
        // LED_On(LED_CHARGE_RED);
        // LED_Off(LED_CHARGE_GREEN);
        usb_flag = 0;
    }
    last_usb_state = current_usb_state;
}

/**
 * @brief 单片机DP上传
 */
uint8_t last_usb_flag = 0;
uint8_t last_battery_percentage = 0U;
void mcu_Dp_Update(void)
{
    uint8_t wifi_state = mcu_get_wifi_work_state();
    if (wifi_state == WIFI_CONNECTED || wifi_state == WIFI_CONN_CLOUD)
    {
        /* 上报电量百分比，但是只上报20的倍数, +20是为了80-99电量的时候 显示满格 以此类推 否则满格显示的机会不大了就*/
        int8_t s_battery_percentage = Battery_GetPercentage() / 20 * 20 + 20;
        if (Get_USB_Flag())
        {
            mcu_dp_bool_update(DPID_CHARGE_STATE, 1);
            last_usb_flag = 0;
        }
        else
        {
            mcu_dp_bool_update(DPID_CHARGE_STATE, 0);
            // 拔出USB时直接上报一次当前电量，防止电量一直没变动的时候不更新到APP
            if (!last_usb_flag)
            {
                mcu_dp_value_update(DPID_BATTERY_PERCENTAGE, s_battery_percentage);
                last_usb_flag = 1;
            }
            // LOG_DEBUG("mcu_Dp_Update: battery_percentage: %d", s_battery_percentage);
            // 上报一次电量百分比，但是只上报20%的倍数
            if (last_battery_percentage != s_battery_percentage)
            {
                mcu_dp_value_update(DPID_BATTERY_PERCENTAGE, s_battery_percentage);
                last_battery_percentage = s_battery_percentage;
            }
        }
    }
}
