/*
 *   Copyright (C) 2025 Chglish
 *
 *   文件名称：ALSA.c
 *   创 建 者：Chglish
 *   创建日期：2025年04月05日
 *   描    述：用于测试ALSA，内谱有《小星星》一首
 *             资料借助deepseek，代码最初由其生成，由我修改
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <alsa/asoundlib.h>
#include <string.h>

/* 采样率（Hz） */
unsigned int SAMPLE_RATE = 44100;

snd_pcm_t *init()
{
	snd_pcm_t *pcm_handle = NULL;
	int err;

	// 打开PCM设备("default")
	if ((err = snd_pcm_open(&pcm_handle, "default", SND_PCM_STREAM_PLAYBACK, 0))) {
		fprintf(stderr, "无法打开PCM设备: %s\n", snd_strerror(err));
		return NULL;
	}

	// 设置硬件参数
	snd_pcm_hw_params_t *hw_params;
	snd_pcm_hw_params_alloca(&hw_params);
	snd_pcm_hw_params_any(pcm_handle, hw_params);

	// 交错模式、16位有符号小端格式、双声道、44100Hz
	if ((err = snd_pcm_hw_params_set_access(pcm_handle, hw_params, SND_PCM_ACCESS_RW_INTERLEAVED))
	    || (err = snd_pcm_hw_params_set_format(pcm_handle, hw_params, SND_PCM_FORMAT_S16_LE))
	    || (err = snd_pcm_hw_params_set_channels(pcm_handle, hw_params, 2))
	    || (err = snd_pcm_hw_params_set_rate_near(pcm_handle, hw_params, &SAMPLE_RATE, 0))) {
		fprintf(stderr, "参数设置失败: %s\n", snd_strerror(err));
		snd_pcm_close(pcm_handle);
		return NULL;
	}

	// 应用参数
	if ((err = snd_pcm_hw_params(pcm_handle, hw_params))) {
		fprintf(stderr, "无法应用硬件参数: %s\n", snd_strerror(err));
		snd_pcm_close(pcm_handle);
		return NULL;
	}
	return pcm_handle;
}

/* 
 * 生成固定声波
 * volume: 0% ~ 100%
 * 数学模型: f(x) = A*sin(ω*x)
 * */
double wave(double x, double volume, double f)
{
	const short max_vol = (unsigned short)(-1) / 2;
	return (volume * max_vol) * sin(f * (2*M_PI) * x);
}

/* 
 * note: 1~7
 * type: n分音符
 * base: 以n分音符为一拍
 * speed: 速度，n拍/分钟
 * */
int play_note(snd_pcm_t *pcm_handle, int note, int type, int base, int speed, int is_stop)
{
	/* C D E F G A B */
	const double note_freq[] = {261.63, 293.66, 329.63, 349.23, 392.00, 440.00, 493.88};
	const int buffer_frames = 512;
	short buffer[buffer_frames * 2];    // 双声道

	note = (note - 1) % 7;
	type = type <= 0 ? 4 : type;
	base = base <= 0 ? 4 : base;
	speed = speed <= 0 ? 60 : speed;

	for (int j = SAMPLE_RATE*((float)base/type)*(60/(float)speed), count = 0; j > 0 ; j -= buffer_frames) {    /* 以1024字节为块生成每次采样的音频 */
		for (int i = 0; i < buffer_frames; i++, count++) {
			// 双声道（左右相同）(2i -> left; 2i+1 -> right)
			buffer[i * 2 + 1] = buffer[i * 2] = (short)wave((float)count / SAMPLE_RATE, 0.5, note_freq[note]);
		}

		if (is_stop && (int)(j - SAMPLE_RATE/32) < 0) {
			memset(buffer, 0, sizeof(short) * buffer_frames * 2);
		}
		// 写入音频数据
		int frames_written = snd_pcm_writei(pcm_handle, buffer, buffer_frames);
		if (frames_written < 0) frames_written = snd_pcm_recover(pcm_handle, frames_written, 0);
		if (frames_written < 0) {
			fprintf(stderr, "写入错误: %s\n", snd_strerror(frames_written));
			return 1;
		}
	}

	return 0;
}

int main()
{
	// ALSA PCM设备配置
	snd_pcm_t *pcm_handle = init();
	if (! pcm_handle) return 1;

	int note_table[42] = {
		1, 1, 5, 5, 6, 6, 5,  4, 4, 3, 3, 2, 2, 1,    /* 一闪一闪亮晶晶 */ /* 满天都是小星星 */
		5, 5, 4, 4, 3, 3, 2,  5, 5, 4, 4, 3, 3, 2,    /* 挂在天上放光明 */ /* 好像许多小眼睛 */
		1, 1, 5, 5, 6, 6, 5,  4, 4, 3, 3, 2, 2, 1,    /* 一闪一闪亮晶晶 */ /* 满天都是小星星 */
	};
	for (int i = 0; i < 42; i++)
		play_note(pcm_handle, note_table[i], i % 7 == 6 ? 2 : 4, 4, 120, 1);

	// 等待播放完成并关闭设备
	snd_pcm_drain(pcm_handle);
	snd_pcm_close(pcm_handle);
	return EXIT_SUCCESS;
}
