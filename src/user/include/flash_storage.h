#ifndef __FLASH_STORAGE_H__
#define __FLASH_STORAGE_H__

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

/* Flash存储区域定义（使用最后一页，2KB） */
#define FLASH_STORAGE_BASE      0x08007800U
#define FLASH_STORAGE_SIZE      2048U
#define FLASH_STORAGE_MAGIC     0x46454544U  /* "FEED" 魔数 */

/* 音频数据最大大小（预留一些空间给其他数据） */
#define AUDIO_DATA_MAX_SIZE     1800U

/**
 * @brief 保存音频PCM数据到Flash
 * @param data PCM数据指针
 * @param size 数据大小（字节）
 * @return 0-失败，1-成功
 */
uint8_t Flash_SaveAudio(const uint8_t *data, uint16_t size);

/**
 * @brief 从Flash读取音频PCM数据
 * @param buffer 缓冲区指针（需要足够大）
 * @param size 输入：缓冲区大小，输出：实际读取的数据大小
 * @return 0-失败或没有数据，1-成功
 */
uint8_t Flash_LoadAudio(uint8_t *buffer, uint16_t *size);

/**
 * @brief 清除Flash中的音频数据
 * @return 0-失败，1-成功
 */
uint8_t Flash_EraseAudio(void);

/**
 * @brief 获取Flash中音频数据大小
 * @return 音频数据大小（字节），0表示没有数据
 */
uint16_t Flash_GetAudioSize(void);

#ifdef __cplusplus
}
#endif

#endif /* __FLASH_STORAGE_H__ */

