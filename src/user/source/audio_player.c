#include "audio_player.h"
#include "pwm_init.h"
#include "flash_storage.h"
#include "ft32f0xx.h"
#include "ft32f0xx_tim.h"
#include "ft32f0xx_rcc.h"

/* 音频播放状态 */
typedef enum
{
	AUDIO_STATE_IDLE,      /* 空闲 */
	AUDIO_STATE_PLAYING    /* 播放中 */
} AudioState_t;

/* 音频缓冲区大小（流式播放，使用小缓冲区节省RAM） */
#define AUDIO_BUFFER_SIZE  64U  /* 64字节缓冲区，足够播放约8ms的8kHz音频 */

/* 音频播放控制结构 */
static struct
{
	AudioState_t state;
	const uint8_t *pcm_data;      /* PCM数据指针（用于直接播放） */
	uint8_t audio_buffer[AUDIO_BUFFER_SIZE];  /* 小缓冲区（用于Flash流式播放） */
	uint32_t flash_base_addr;     /* Flash音频数据基地址 */
	uint16_t total_size;          /* 总数据大小 */
	uint16_t current_index;       /* 当前播放位置（全局索引） */
	uint16_t buffer_fill_index;  /* 缓冲区填充位置 */
	uint16_t sample_rate;         /* 采样率 */
	uint8_t from_flash;           /* 是否从Flash播放 */
	AudioCompleteCallback_t complete_callback;  /* 完成回调 */
} s_audio_ctrl;

/* 定时器配置：使用 TIM14 控制音频播放速率（FT32F030K6ATx没有TIM2） */
#define AUDIO_TIMER			TIM14
#define AUDIO_TIMER_IRQn	TIM14_IRQn
#define AUDIO_TIMER_CLK		RCC_APB1Periph_TIM14

/* PWM配置：使用TIM3，PB4输出 */
/* 注意：PWM频率需要远高于音频采样率，用于输出PCM值 */
#define PWM_BASE_FREQ_HZ  20000U  /* 20kHz基础频率，用于PCM输出 */
#define AUDIO_PWM_TIMER			TIM3
#define AUDIO_PWM_TIMER_CLK		RCC_APB1Periph_TIM3

/* PWM周期值（在初始化时计算） */
static uint16_t s_pwm_period = 0U;

/**
 * @brief 初始化PWM用于音频播放（固定频率，可变占空比）
 */
static void Audio_PWM_Init(void)
{
	RCC_APB1PeriphClockCmd(AUDIO_PWM_TIMER_CLK, ENABLE);

	uint32_t timer_clk = SystemCoreClock;
	uint32_t base_freq = PWM_BASE_FREQ_HZ;
	
	uint32_t prescaler = 0;
	uint32_t period = 0;
	
	/* 计算预分频和周期 */
	for (prescaler = 0; prescaler < 0xFFFF; prescaler++)
	{
		uint32_t div = prescaler + 1;
		uint32_t tentative = timer_clk / (div * base_freq);
		if (tentative > 0 && tentative <= 0xFFFF)
		{
			period = tentative - 1;
			break;
		}
	}
	
	if (period == 0)
	{
		return;
	}
	
	s_pwm_period = (uint16_t)period;
	
	/* 配置TIM3为固定频率PWM */
	TIM_TimeBaseInitTypeDef tb;
	TIM_OCInitTypeDef oc;
	
	TIM_TimeBaseStructInit(&tb);
	tb.TIM_Prescaler = (uint16_t)prescaler;
	tb.TIM_CounterMode = TIM_CounterMode_Up;
	tb.TIM_Period = s_pwm_period;
	tb.TIM_ClockDivision = TIM_CKD_DIV1;
	TIM_TimeBaseInit(AUDIO_PWM_TIMER, &tb);
	
	TIM_OCStructInit(&oc);
	oc.TIM_OCMode = TIM_OCMode_PWM1;
	oc.TIM_OutputState = TIM_OutputState_Enable;
	oc.TIM_Pulse = 0;  /* 初始占空比为0 */
	oc.TIM_OCPolarity = TIM_OCPolarity_High;
	TIM_OC1Init(AUDIO_PWM_TIMER, &oc);
	TIM_OC1PreloadConfig(AUDIO_PWM_TIMER, TIM_OCPreload_Enable);
	
	TIM_ARRPreloadConfig(AUDIO_PWM_TIMER, ENABLE);
}

/**
 * @brief 设置PWM占空比（只更新CCR寄存器，不重新配置定时器）
 * @param pcm_value PCM值（0-255）
 */
static void Audio_PWM_SetDuty(uint8_t pcm_value)
{
	if (s_pwm_period == 0)
	{
		return;
	}
	
	/* 将PCM值（0-255）映射到PWM占空比 */
	uint32_t pulse = ((uint32_t)pcm_value * (s_pwm_period + 1)) / 255U;
	
	/* 更新CCR1寄存器（占空比） */
	TIM_SetCompare1(AUDIO_PWM_TIMER, (uint16_t)pulse);
	
	/* 确保PWM输出使能 */
	if ((AUDIO_PWM_TIMER->CR1 & TIM_CR1_CEN) == 0)
	{
		TIM_Cmd(AUDIO_PWM_TIMER, ENABLE);
	}
}

/**
 * @brief 初始化音频播放模块
 */
void Audio_Init(void)
{
	/* 初始化PWM用于音频播放 */
	Audio_PWM_Init();
	
	/* 使能定时器时钟 */
	RCC_APB1PeriphClockCmd(AUDIO_TIMER_CLK, ENABLE);
	
	/* 配置定时器：用于控制音频播放速率 */
	/* 默认配置为8kHz采样率 */
	uint32_t timer_clk = SystemCoreClock;
	uint16_t sample_rate = 8000U;
	uint16_t prescaler = (uint16_t)((timer_clk / 1000000U) - 1U);
	uint16_t period = (1000000U / sample_rate) - 1U;
	
	TIM_TimeBaseInitTypeDef tb;
	TIM_TimeBaseStructInit(&tb);
	tb.TIM_Prescaler = prescaler;
	tb.TIM_CounterMode = TIM_CounterMode_Up;
	tb.TIM_Period = period;
	tb.TIM_ClockDivision = TIM_CKD_DIV1;
	TIM_TimeBaseInit(AUDIO_TIMER, &tb);
	
	TIM_ClearITPendingBit(AUDIO_TIMER, TIM_IT_Update);
	TIM_ITConfig(AUDIO_TIMER, TIM_IT_Update, ENABLE);
	
	/* 配置NVIC */
	NVIC_InitTypeDef nvic;
	nvic.NVIC_IRQChannel = AUDIO_TIMER_IRQn;
	nvic.NVIC_IRQChannelPriority = 3;  /* 优先级低于电机定时器 */
	nvic.NVIC_IRQChannelCmd = ENABLE;
	NVIC_Init(&nvic);
	
	/* 初始状态 */
	s_audio_ctrl.state = AUDIO_STATE_IDLE;
	s_audio_ctrl.pcm_data = NULL;
	s_audio_ctrl.total_size = 0U;
	s_audio_ctrl.current_index = 0U;
	s_audio_ctrl.buffer_fill_index = 0U;
	s_audio_ctrl.from_flash = 0U;
	s_audio_ctrl.complete_callback = NULL;
	
	/* 停止定时器 */
	TIM_Cmd(AUDIO_TIMER, DISABLE);
}

/**
 * @brief 从Flash读取数据到缓冲区（填充整个缓冲区）
 */
static void Audio_FillBufferFromFlash(void)
{
	/* 计算还需要读取的总数据量 */
	uint16_t remaining = s_audio_ctrl.total_size - s_audio_ctrl.buffer_fill_index;
	
	if (remaining == 0)
	{
		return;
	}
	
	/* 读取一个缓冲区大小的数据 */
	uint16_t to_read = (remaining > AUDIO_BUFFER_SIZE) ? AUDIO_BUFFER_SIZE : remaining;
	
	/* 从Flash直接读取数据 */
	uint32_t flash_addr = s_audio_ctrl.flash_base_addr + s_audio_ctrl.buffer_fill_index;
	uint8_t *buffer = s_audio_ctrl.audio_buffer;
	
	for (uint16_t i = 0; i < to_read; i++)
	{
		/* 从Flash地址读取字节 */
		buffer[i] = *((uint8_t *)flash_addr + i);
	}
	
	/* 如果数据不足一个缓冲区，剩余部分填充0（静音） */
	for (uint16_t i = to_read; i < AUDIO_BUFFER_SIZE; i++)
	{
		buffer[i] = 128;  /* 填充中间值（静音） */
	}
	
	s_audio_ctrl.buffer_fill_index += to_read;
}

/**
 * @brief 播放PCM音频数据（非阻塞）
 */
uint8_t Audio_PlayPCM(const uint8_t *pcm_data, uint16_t data_size, uint16_t sample_rate)
{
	if (pcm_data == NULL || data_size == 0 || sample_rate == 0)
	{
		return 0U;
	}
	
	/* 如果正在播放，先停止 */
	if (s_audio_ctrl.state == AUDIO_STATE_PLAYING)
	{
		Audio_Stop();
	}
	
	/* 设置播放参数（直接播放模式） */
	s_audio_ctrl.pcm_data = pcm_data;
	s_audio_ctrl.total_size = data_size;
	s_audio_ctrl.sample_rate = sample_rate;
	s_audio_ctrl.current_index = 0U;
	s_audio_ctrl.from_flash = 0U;
	s_audio_ctrl.state = AUDIO_STATE_PLAYING;
	
	/* 重新配置定时器以匹配采样率 */
	uint32_t timer_clk = SystemCoreClock;
	uint16_t prescaler = (uint16_t)((timer_clk / 1000000U) - 1U);
	uint16_t period = (1000000U / sample_rate) - 1U;
	
	TIM_TimeBaseInitTypeDef tb;
	TIM_TimeBaseStructInit(&tb);
	tb.TIM_Prescaler = prescaler;
	tb.TIM_CounterMode = TIM_CounterMode_Up;
	tb.TIM_Period = period;
	tb.TIM_ClockDivision = TIM_CKD_DIV1;
	TIM_TimeBaseInit(AUDIO_TIMER, &tb);
	
	/* 启动定时器 */
	TIM_Cmd(AUDIO_TIMER, ENABLE);
	
	/* 播放第一个样本 */
	if (s_audio_ctrl.current_index < s_audio_ctrl.total_size)
	{
		uint8_t pcm_value = s_audio_ctrl.pcm_data[s_audio_ctrl.current_index];
		Audio_PWM_SetDuty(pcm_value);
	}
	
	return 1U;
}

/**
 * @brief 从Flash播放音频（非阻塞，流式播放）
 */
uint8_t Audio_PlayFromFlash(uint16_t sample_rate)
{
	/* 获取Flash中音频数据大小 */
	uint16_t audio_size = Flash_GetAudioSize();
	if (audio_size == 0)
	{
		return 0U;
	}
	
	/* 如果正在播放，先停止 */
	if (s_audio_ctrl.state == AUDIO_STATE_PLAYING)
	{
		Audio_Stop();
	}
	
	/* 设置流式播放参数 */
	s_audio_ctrl.flash_base_addr = FLASH_STORAGE_BASE + 8;  /* 跳过魔数和大小字段 */
	s_audio_ctrl.total_size = audio_size;
	s_audio_ctrl.sample_rate = sample_rate;
	s_audio_ctrl.current_index = 0U;
	s_audio_ctrl.buffer_fill_index = 0U;
	s_audio_ctrl.from_flash = 1U;
	s_audio_ctrl.pcm_data = NULL;
	s_audio_ctrl.state = AUDIO_STATE_PLAYING;
	
	/* 填充第一个缓冲区 */
	Audio_FillBufferFromFlash();
	
	/* 重新配置定时器以匹配采样率 */
	uint32_t timer_clk = SystemCoreClock;
	uint16_t prescaler = (uint16_t)((timer_clk / 1000000U) - 1U);
	uint16_t period = (1000000U / sample_rate) - 1U;
	
	TIM_TimeBaseInitTypeDef tb;
	TIM_TimeBaseStructInit(&tb);
	tb.TIM_Prescaler = prescaler;
	tb.TIM_CounterMode = TIM_CounterMode_Up;
	tb.TIM_Period = period;
	tb.TIM_ClockDivision = TIM_CKD_DIV1;
	TIM_TimeBaseInit(AUDIO_TIMER, &tb);
	
	/* 启动定时器 */
	TIM_Cmd(AUDIO_TIMER, ENABLE);
	
	/* 播放第一个样本 */
	if (s_audio_ctrl.current_index < s_audio_ctrl.total_size)
	{
		uint8_t pcm_value = s_audio_ctrl.audio_buffer[0];
		Audio_PWM_SetDuty(pcm_value);
	}
	
	return 1U;
}

/**
 * @brief 停止播放
 */
void Audio_Stop(void)
{
	if (s_audio_ctrl.state == AUDIO_STATE_PLAYING)
	{
		/* 停止定时器 */
		TIM_Cmd(AUDIO_TIMER, DISABLE);
		
		/* 停止PWM */
		PWM_Stop();
		
		/* 复位状态 */
		s_audio_ctrl.state = AUDIO_STATE_IDLE;
		s_audio_ctrl.pcm_data = NULL;
		s_audio_ctrl.total_size = 0U;
		s_audio_ctrl.current_index = 0U;
		s_audio_ctrl.buffer_fill_index = 0U;
		s_audio_ctrl.from_flash = 0U;
	}
}

/**
 * @brief 检查是否正在播放
 */
uint8_t Audio_IsPlaying(void)
{
	return (s_audio_ctrl.state == AUDIO_STATE_PLAYING) ? 1U : 0U;
}

/**
 * @brief 设置播放完成回调函数
 */
void Audio_SetCompleteCallback(AudioCompleteCallback_t callback)
{
	s_audio_ctrl.complete_callback = callback;
}

/**
 * @brief 更新音频播放状态机（需要在主循环中定期调用）
 */
void Audio_Update(void)
{
	/* 状态机更新在中断中完成，这里可以添加其他逻辑 */
}

/* TIM14 中断处理函数：按采样率更新PCM数据 */
void TIM14_IRQHandler(void)
{
	if (TIM_GetITStatus(AUDIO_TIMER, TIM_IT_Update) != RESET)
	{
		TIM_ClearITPendingBit(AUDIO_TIMER, TIM_IT_Update);
		
		if (s_audio_ctrl.state == AUDIO_STATE_PLAYING)
		{
			/* 移动到下一个样本 */
			s_audio_ctrl.current_index++;
			
			/* 检查是否播放完成 */
			if (s_audio_ctrl.current_index >= s_audio_ctrl.total_size)
			{
				/* 播放完成 */
				Audio_Stop();
				
				/* 调用完成回调 */
				if (s_audio_ctrl.complete_callback != NULL)
				{
					s_audio_ctrl.complete_callback();
				}
			}
			else
			{
				uint8_t pcm_value;
				
				if (s_audio_ctrl.from_flash)
				{
					/* 流式播放模式：从缓冲区读取 */
					uint16_t buffer_index = s_audio_ctrl.current_index % AUDIO_BUFFER_SIZE;
					
					/* 如果播放到缓冲区末尾，需要填充下一个缓冲区 */
					if (buffer_index == 0 && s_audio_ctrl.current_index > 0)
					{
						/* 检查是否还有数据需要读取 */
						if (s_audio_ctrl.buffer_fill_index < s_audio_ctrl.total_size)
						{
							Audio_FillBufferFromFlash();
						}
					}
					
					/* 从缓冲区读取PCM值 */
					pcm_value = s_audio_ctrl.audio_buffer[buffer_index];
				}
				else
				{
					/* 直接播放模式：从内存读取 */
					pcm_value = s_audio_ctrl.pcm_data[s_audio_ctrl.current_index];
				}
				
				/* 更新PWM占空比 */
				Audio_PWM_SetDuty(pcm_value);
			}
		}
	}
}

