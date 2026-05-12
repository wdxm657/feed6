#ifndef __LED_H__
#define __LED_H__

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

/* LED编号定义 */
typedef enum
{
	LED_CHARGE_RED = 0,    /* 充电指示红色LED：PA2 */
	LED_CHARGE_GREEN = 1,  /* 充电指示绿色LED：PB7 */
	LED_BLUE = 2,          /* 普通蓝色LED：PA8 */
	LED_RED = 3            /* 普通红色LED：PA11 */
} LedNum_t;

#define LOWER_POWER_THRESHOLD 10U
#define UPPER_POWER_THRESHOLD 60U

/**
 * @brief 初始化LED GPIO
 */
void LED_Init(void);

/**
 * @brief 设置LED状态
 * @param led_num LED编号
 * @param on 1-点亮，0-熄灭
 */
void LED_Set(LedNum_t led_num, uint8_t on);

/**
 * @brief 点亮LED
 * @param led_num LED编号
 */
void LED_On(LedNum_t led_num);

/**
 * @brief 熄灭LED
 * @param led_num LED编号
 */
void LED_Off(LedNum_t led_num);

/**
 * @brief 初始化所有LED为熄灭状态
 */
void LED_InitAllOff(void);

/**
 * @brief WIFI状态LED控制任务
 * @note 根据WIFI状态控制普通蓝灯：
 *       - SMART_CONFIG_STATE: LED快闪（200ms间隔）
 *       - AP_STATE: LED慢闪（1000ms间隔）
 *       - WIFI_NOT_CONNECTED: LED常暗（熄灭）
 *       - WIFI_CONNECTED: LED常亮
 */
void LED_WifiStatusControl(void);

/**
 * @brief 电源/电量指示灯控制任务
 *
 * 逻辑说明：
 * - 充电中（USB 插入且 CHRG=0）：红灯闪烁，绿灯关闭
 * - 充满电（USB 插入且 STDBY=0）：绿灯常亮，红灯关闭
 * - USB 拔出且电量 >= 50%：红灯与绿灯交替闪烁，周期 500ms（视觉为黄灯闪烁）
 * - USB 拔出且 30% <= 电量 < 50%：红灯常亮，绿灯关闭
 * - 电量 < 30%：红灯与绿灯都关闭
 */
void LED_PowerStatusControl(void);

#ifdef __cplusplus
}
#endif

#endif /* __LED_H__ */

