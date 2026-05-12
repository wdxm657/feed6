#include "flash_storage.h"
#include "ft32f0xx.h"
#include "ft32f0xx_flash.h"

/* Flash存储结构体 */
typedef struct
{
	uint32_t magic;          /* 魔数，用于验证 */
	uint16_t audio_size;     /* 音频数据大小 */
	uint8_t reserved[2];     /* 保留字段，对齐到4字节 */
	uint8_t audio_data[AUDIO_DATA_MAX_SIZE];  /* 音频数据 */
} FlashStorage_t;

/* Flash存储区域指针 */
#define FLASH_STORAGE_PTR  ((FlashStorage_t *)FLASH_STORAGE_BASE)

/**
 * @brief 擦除Flash存储页
 */
static uint8_t Flash_ErasePage(void)
{
	FLASH_Status status;
	
	/* 解锁Flash */
	FLASH_Unlock();
	
	/* 清除所有Flash标志 */
	FLASH_ClearFlag(FLASH_FLAG_EOP | FLASH_FLAG_PGERR | FLASH_FLAG_WRPERR);
	
	/* 擦除最后一页（页地址 = FLASH_STORAGE_BASE） */
	status = FLASH_ErasePage(FLASH_STORAGE_BASE);
	
	/* 锁定Flash */
	FLASH_Lock();
	
	return (status == FLASH_COMPLETE) ? 1U : 0U;
}

/**
 * @brief 写入一个字（4字节）到Flash
 */
static uint8_t Flash_WriteWord(uint32_t address, uint32_t data)
{
	FLASH_Status status;
	
	status = FLASH_ProgramWord(address, data);
	
	return (status == FLASH_COMPLETE) ? 1U : 0U;
}

/**
 * @brief 保存音频PCM数据到Flash
 */
uint8_t Flash_SaveAudio(const uint8_t *data, uint16_t size)
{
	if (data == NULL || size == 0 || size > AUDIO_DATA_MAX_SIZE)
	{
		return 0U;
	}
	
	/* 解锁Flash */
	FLASH_Unlock();
	
	/* 清除所有Flash标志 */
	FLASH_ClearFlag(FLASH_FLAG_EOP | FLASH_FLAG_PGERR | FLASH_FLAG_WRPERR);
	
	/* 擦除存储页 */
	if (Flash_ErasePage() == 0U)
	{
		FLASH_Lock();
		return 0U;
	}
	
	/* 写入魔数 */
	if (Flash_WriteWord(FLASH_STORAGE_BASE, FLASH_STORAGE_MAGIC) == 0U)
	{
		FLASH_Lock();
		return 0U;
	}
	
	/* 写入音频数据大小（2字节，需要组合成字） */
	uint32_t size_word = (uint32_t)size | (0x0000U << 16);  /* 高16位为0 */
	if (Flash_WriteWord(FLASH_STORAGE_BASE + 4, size_word) == 0U)
	{
		FLASH_Lock();
		return 0U;
	}
	
	/* 写入音频数据（按字对齐写入） */
	uint32_t address = FLASH_STORAGE_BASE + 8;  /* 跳过魔数和大小字段 */
	uint16_t words = (size + 3) / 4;  /* 向上取整到4字节对齐 */
	
	for (uint16_t i = 0; i < words; i++)
	{
		uint32_t word_data = 0xFFFFFFFFU;  /* 默认填充0xFF */
		
		/* 组合4个字节成一个字 */
		for (uint8_t j = 0; j < 4; j++)
		{
			uint16_t byte_index = i * 4 + j;
			if (byte_index < size)
			{
				word_data &= ~(0xFFU << (j * 8));
				word_data |= ((uint32_t)data[byte_index]) << (j * 8);
			}
		}
		
		if (Flash_WriteWord(address, word_data) == 0U)
		{
			FLASH_Lock();
			return 0U;
		}
		
		address += 4;
	}
	
	/* 锁定Flash */
	FLASH_Lock();
	
	return 1U;
}

/**
 * @brief 从Flash读取音频PCM数据
 */
uint8_t Flash_LoadAudio(uint8_t *buffer, uint16_t *size)
{
	if (buffer == NULL || size == NULL)
	{
		return 0U;
	}
	
	FlashStorage_t *storage = FLASH_STORAGE_PTR;
	
	/* 检查魔数 */
	if (storage->magic != FLASH_STORAGE_MAGIC)
	{
		*size = 0U;
		return 0U;
	}
	
	/* 检查数据大小 */
	if (storage->audio_size == 0 || storage->audio_size > AUDIO_DATA_MAX_SIZE)
	{
		*size = 0U;
		return 0U;
	}
	
	/* 检查缓冲区大小 */
	if (*size < storage->audio_size)
	{
		*size = storage->audio_size;
		return 0U;  /* 缓冲区不够大 */
	}
	
	/* 复制数据 */
	for (uint16_t i = 0; i < storage->audio_size; i++)
	{
		buffer[i] = storage->audio_data[i];
	}
	
	*size = storage->audio_size;
	return 1U;
}

/**
 * @brief 清除Flash中的音频数据
 */
uint8_t Flash_EraseAudio(void)
{
	return Flash_ErasePage();
}

/**
 * @brief 获取Flash中音频数据大小
 */
uint16_t Flash_GetAudioSize(void)
{
	FlashStorage_t *storage = FLASH_STORAGE_PTR;
	
	/* 检查魔数 */
	if (storage->magic != FLASH_STORAGE_MAGIC)
	{
		return 0U;
	}
	
	/* 检查数据大小 */
	if (storage->audio_size == 0 || storage->audio_size > AUDIO_DATA_MAX_SIZE)
	{
		return 0U;
	}
	
	return storage->audio_size;
}

