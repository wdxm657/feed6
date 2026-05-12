#ifndef __AUDIO_PLAYER_H__
#define __AUDIO_PLAYER_H__

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

/* 音频播放完成回调函数类型 */
typedef void (*AudioCompleteCallback_t)(void);

/**
 * @brief 初始化音频播放模块
 */
void Audio_Init(void);

/**
 * @brief 播放PCM音频数据（非阻塞）
 * @param pcm_data PCM数据指针
 * @param data_size 数据大小（字节）
 * @param sample_rate 采样率（Hz），建议8000
 * @return 0-失败，1-成功
 */
uint8_t Audio_PlayPCM(const uint8_t *pcm_data, uint16_t data_size, uint16_t sample_rate);

/**
 * @brief 从Flash播放音频（非阻塞）
 * @param sample_rate 采样率（Hz），建议8000
 * @return 0-失败，1-成功
 */
uint8_t Audio_PlayFromFlash(uint16_t sample_rate);

/**
 * @brief 停止播放
 */
void Audio_Stop(void);

/**
 * @brief 检查是否正在播放
 * @return 1-正在播放，0-未播放
 */
uint8_t Audio_IsPlaying(void);

/**
 * @brief 设置播放完成回调函数
 */
void Audio_SetCompleteCallback(AudioCompleteCallback_t callback);

/**
 * @brief 更新音频播放状态机（需要在主循环中定期调用）
 */
void Audio_Update(void);

#ifdef __cplusplus
}
#endif

#endif /* __AUDIO_PLAYER_H__ */

