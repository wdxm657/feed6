#include "log.h"
#include <stdio.h>
#include <string.h>
#include "usart_init.h"

#if LOG_ENABLE
#include "mcu_api.h"
#include "protocol.h"
#include "wifi.h"

/* 日志配置 */
static LogLevel_t s_log_level = LOG_LEVEL;
static uint8_t s_log_output_mode = LOG_OUTPUT_MODE;

/**
 * @brief 初始化日志模块
 */
void Log_Init(void)
{
    s_log_level = LOG_LEVEL;
    s_log_output_mode = LOG_OUTPUT_MODE;
}

/**
 * @brief 设置日志级别
 * @param level 日志级别
 */
void Log_SetLevel(LogLevel_t level)
{
    s_log_level = level;
}

/**
 * @brief 设置日志输出方式
 * @param mode 输出方式（LOG_OUTPUT_UART、LOG_OUTPUT_DP或LOG_OUTPUT_ALL）
 */
void Log_SetOutputMode(uint8_t mode)
{
    s_log_output_mode = mode;
}

/**
 * @brief 日志输出函数（内部使用）
 * @param level 日志级别
 * @param format 格式化字符串
 * @param ... 可变参数
 */
void Log_Print(LogLevel_t level, const char *format, ...)
{
    /* 检查日志级别 */
    if (level < s_log_level)
    {
        return;  /* 低于当前日志级别，不输出 */
    }
    
    char log_buffer[LOG_BUFFER_SIZE];
    va_list args;
    
    /* 格式化字符串 */
    va_start(args, format);
    vsnprintf(log_buffer, sizeof(log_buffer), format, args);
    va_end(args);
    
    /* 确保字符串以\0结尾 */
    log_buffer[LOG_BUFFER_SIZE - 1] = '\0';
    
    /* 串口输出 */
    if (s_log_output_mode & LOG_OUTPUT_UART)
    {
        printf("USART: %s\n", log_buffer);
    }
    
    /* DP上报输出（涂鸦） */
    uint8_t wifi_state = mcu_get_wifi_work_state();
    if (s_log_output_mode & LOG_OUTPUT_DP && (wifi_state == WIFI_CONNECTED || wifi_state == WIFI_CONN_CLOUD))
    {
        /* 使用DPID_A作为日志上报通道（可以根据需要修改） */
        mcu_dp_string_update(DPID_A, (unsigned char *)log_buffer, strlen(log_buffer));
    }
}

#else
/* 日志功能禁用时的空实现 */
void Log_Init(void) { }
void Log_SetLevel(LogLevel_t level) { (void)level; }
void Log_SetOutputMode(uint8_t mode) { (void)mode; }
void Log_Print(LogLevel_t level, const char *format, ...) { (void)level; (void)format; }
#endif

