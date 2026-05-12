#ifndef __LOG_H__
#define __LOG_H__

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stdarg.h>

/* 日志级别定义 */
typedef enum
{
    LOG_LEVEL_DEBUG = 0,  /* 调试信息 */
    LOG_LEVEL_INFO,       /* 一般信息 */
    LOG_LEVEL_WARN,       /* 警告信息 */
    LOG_LEVEL_ERROR       /* 错误信息 */
} LogLevel_t;

/* 日志输出方式 */
#define LOG_OUTPUT_UART   0x01  /* 串口输出 */
#define LOG_OUTPUT_DP     0x02  /* DP上报（涂鸦） */
#define LOG_OUTPUT_ALL    (LOG_OUTPUT_UART | LOG_OUTPUT_DP)

/* 日志配置 */
#define LOG_ENABLE         1    /* 是否启用日志：1-启用，0-禁用 */
#define LOG_LEVEL          LOG_LEVEL_DEBUG  /* 当前日志级别，低于此级别的日志不输出 */
#define LOG_OUTPUT_MODE    LOG_OUTPUT_UART  /* 输出方式：串口、DP或两者 */

/* 日志缓冲区大小 */
#define LOG_BUFFER_SIZE    256U

/**
 * @brief 初始化日志模块
 */
void Log_Init(void);

/**
 * @brief 设置日志级别
 * @param level 日志级别
 */
void Log_SetLevel(LogLevel_t level);

/**
 * @brief 设置日志输出方式
 * @param mode 输出方式（LOG_OUTPUT_UART、LOG_OUTPUT_DP或LOG_OUTPUT_ALL）
 */
void Log_SetOutputMode(uint8_t mode);

/**
 * @brief 日志输出函数（内部使用）
 * @param level 日志级别
 * @param format 格式化字符串
 * @param ... 可变参数
 */
void Log_Print(LogLevel_t level, const char *format, ...);

/* 便捷宏定义 */
#if LOG_ENABLE
    #define LOG_DEBUG(fmt, ...)  Log_Print(LOG_LEVEL_DEBUG, "[DEBUG] " fmt "\r\n", ##__VA_ARGS__)
    #define LOG_INFO(fmt, ...)   Log_Print(LOG_LEVEL_INFO,  "[INFO]  " fmt "\r\n", ##__VA_ARGS__)
    #define LOG_WARN(fmt, ...)   Log_Print(LOG_LEVEL_WARN,  "[WARN]  " fmt "\r\n", ##__VA_ARGS__)
    #define LOG_ERROR(fmt, ...)  Log_Print(LOG_LEVEL_ERROR, "[ERROR] " fmt "\r\n", ##__VA_ARGS__)
#else
    #define LOG_DEBUG(fmt, ...)  ((void)0)
    #define LOG_INFO(fmt, ...)   ((void)0)
    #define LOG_WARN(fmt, ...)   ((void)0)
    #define LOG_ERROR(fmt, ...)  ((void)0)
#endif

#ifdef __cplusplus
}
#endif

#endif /* __LOG_H__ */

