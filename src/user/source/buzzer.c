#include "buzzer.h"
#include "ft32f0xx_gpio.h"
#include "ft32f0xx_rcc.h"
#include "ft32f0xx_tim.h"
#include "systick_tick.h"
#include "log.h"
#include "pwm_init.h"
#include "rtc_init.h"
#include "battery_monitor.h"

#define BUZZER_GPIO_PORT GPIOB
#define BUZZER_GPIO_PIN GPIO_Pin_4
#define BUZZER_PWM_FREQ_HZ 4000U

typedef struct
{
    uint8_t active;
    uint8_t is_on;
    uint8_t repeat_total;
    uint8_t repeat_done;
    uint32_t on_ms;
    uint32_t off_ms;
    uint32_t last_toggle_ms;
} BuzzerContext_t;

static BuzzerContext_t s_buzzer_ctx = {0};

/* 判断是否处于夜间静音时段（20:00 - 08:00），或RTC未有效同步 */
static uint8_t Buzzer_IsQuietHours(void)
{
    uint8_t hour = 0U, minute = 0U, second = 0U;
    uint16_t year = 0U;
    uint8_t month = 0U, day = 0U;

    RTC_User_GetTime(&hour, &minute, &second);
    RTC_User_GetDate(&year, &month, &day);

    /* 简单判定是否已获取有效网络时间：默认上电时间为 2024-01-01 00:00:00 */
    // if ((year <= 2024U) && (month <= 1U) && (day <= 1U))
    // {
    //     return 1U; /* 视为未同步，静音 */
    // }

    /* 夜间静音：20:00-23:59 或 00:00-07:59 */
    if (hour >= 20U || hour < 8U)
    {
        return 1U;
    }

    return 0U;
}

static void Buzzer_SetState(uint8_t on)
{
    if (on)
    {
        GPIO_SetBits(BUZZER_GPIO_PORT, BUZZER_GPIO_PIN);
    }
    else
    {
        GPIO_ResetBits(BUZZER_GPIO_PORT, BUZZER_GPIO_PIN);
    }
}

static void Buzzer_UpdateWaveState(uint8_t enable)
{
    if (enable)
    {
        PWM_SetFrequency(BUZZER_PWM_FREQ_HZ);
    }
    else
    {
        /* 先停止PWM（会将GPIO切换回普通输出模式并设置为低电平） */
        PWM_Stop();
        /* Buzzer_SetState(0U) 不再需要，因为PWM_Stop已经设置了低电平 */
    }
}

void Buzzer_Init(void)
{
    RCC_AHBPeriphClockCmd(RCC_AHBPeriph_GPIOB, ENABLE);
    Buzzer_SetState(0U);
    Buzzer_Stop();
}

void Buzzer_Stop(void)
{
    Buzzer_SetState(0U);
    Buzzer_UpdateWaveState(0U);
    s_buzzer_ctx.active = 0U;
    s_buzzer_ctx.is_on = 0U;
    s_buzzer_ctx.repeat_total = 0U;
    s_buzzer_ctx.repeat_done = 0U;
    s_buzzer_ctx.on_ms = 0U;
    s_buzzer_ctx.off_ms = 0U;
    s_buzzer_ctx.last_toggle_ms = 0U;
}

void Buzzer_Play(uint8_t repeat, uint32_t on_ms, uint32_t off_ms)
{
    if (repeat == 0U || on_ms == 0U || Battery_GetClosePowerMode())
    {
        return;
    }

    /* 普通播放不检查静音时段，允许所有时间播放 */
    s_buzzer_ctx.repeat_total = repeat;
    s_buzzer_ctx.repeat_done = 0U;
    s_buzzer_ctx.on_ms = on_ms;
    s_buzzer_ctx.off_ms = off_ms;
    s_buzzer_ctx.active = 1U;
    s_buzzer_ctx.is_on = 1U;
    s_buzzer_ctx.last_toggle_ms = Systick_Tick_GetMs();
    Buzzer_UpdateWaveState(1U);
}

/**
 * @brief 带静音时段检查的蜂鸣器播放（用于关机前提示等需要夜间静音的场景）
 * @param repeat 重复次数
 * @param on_ms 每次响的时间（毫秒）
 * @param off_ms 每次间隔的时间（毫秒）
 */
void Buzzer_PlayWithQuietHours(uint8_t repeat, uint32_t on_ms, uint32_t off_ms)
{
    if (repeat == 0U || on_ms == 0U || Battery_GetClosePowerMode())
    {
        return;
    }

    /* 夜间静音或RTC未同步时，不响蜂鸣器 */
    if (Buzzer_IsQuietHours())
    {
        return;
    }

    s_buzzer_ctx.repeat_total = repeat;
    s_buzzer_ctx.repeat_done = 0U;
    s_buzzer_ctx.on_ms = on_ms;
    s_buzzer_ctx.off_ms = off_ms;
    s_buzzer_ctx.active = 1U;
    s_buzzer_ctx.is_on = 1U;
    s_buzzer_ctx.last_toggle_ms = Systick_Tick_GetMs();
    Buzzer_UpdateWaveState(1U);
}

uint8_t Buzzer_IsActive(void)
{
    return s_buzzer_ctx.active;
}

void Buzzer_Task(void)
{
    uint32_t now;

    if (!s_buzzer_ctx.active)
    {
        Buzzer_UpdateWaveState(0U);
        return;
    }

    now = Systick_Tick_GetMs();

    if (s_buzzer_ctx.is_on)
    {
        if (Systick_Tick_IsTimeout(s_buzzer_ctx.last_toggle_ms, s_buzzer_ctx.on_ms))
        {
            Buzzer_UpdateWaveState(0U);
            s_buzzer_ctx.is_on = 0U;
            s_buzzer_ctx.last_toggle_ms = now;
            s_buzzer_ctx.repeat_done++;

            if (s_buzzer_ctx.repeat_done >= s_buzzer_ctx.repeat_total)
            {
                s_buzzer_ctx.active = 0U;
            }
        }
    }
    else
    {
        if (s_buzzer_ctx.repeat_done >= s_buzzer_ctx.repeat_total)
        {
            s_buzzer_ctx.active = 0U;
            return;
        }

        if (s_buzzer_ctx.off_ms == 0U)
        {
            s_buzzer_ctx.active = 0U;
            return;
        }

        if (Systick_Tick_IsTimeout(s_buzzer_ctx.last_toggle_ms, s_buzzer_ctx.off_ms))
        {
            Buzzer_UpdateWaveState(1U);
            s_buzzer_ctx.is_on = 1U;
            s_buzzer_ctx.last_toggle_ms = now;
        }
    }
}
