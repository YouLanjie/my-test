/*
 *   Copyright (C) 2025 Chglish
 *
 *   文件名称：ALSA.c
 *   创 建 者：Chglish
 *   创建日期：2025年04月05日
 *   描    述：用于测试ALSA，后发展出部分已分离
 *
 */


#include "core.h"
#include <alsa/asoundlib.h>

#define FLG_PLAY (1 << 0)
#define FLG_SAVE (1 << 1)
/*#define FLG_INPUT (1 << 2)*/
#define FLG_PLYARG (1 << 3)    /* 播放终端传入参数 */
#define FLG_FADE (1 << 4)	/* 淡入淡出 */
#define FLG_HARMONICS (1<<5)	/* 泛音 */
#define FLG_SMOOTH_PORTAMENTO (1<<6)
#define FLG_PRINT (1<<7)
#define FLG_PRINT_DEBUG (1<<9)
#define FLG_PRINT_PHA (1<<10)

/*uint16_t status = FLG_FADE|FLG_HARMONICS;*/

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

int play_wav(snd_pcm_t *pcm_handle, int16_t *wave, int size)
{
	if (!pcm_handle)
		return 1;
	// 写入音频数据
	int frames_written = snd_pcm_writei(pcm_handle, wave, size);
	if (frames_written < 0) frames_written = snd_pcm_recover(pcm_handle, frames_written, 0);
	if (frames_written < 0) {
		fprintf(stderr, "写入错误: %s\n", snd_strerror(frames_written));
		return 1;
	}
	return 0;
}

int read_play_wave(char *filename)
{
	if (!filename) return -1;
	FILE *fp = fopen(filename, "rb");
	if (!fp) {
		fprintf(stderr, "打开文件失败:%s\n", filename);
		return 1;
	}

	char *content = NULL, *p = NULL;
	char RIFF[5] = {0}, WAVE[5] = {0}, data[5] = {0};
	fseek(fp, 0L, SEEK_END);
	int size = ftell(fp);
	fseek(fp, 0L, SEEK_SET);
	content = malloc(sizeof(int)*size);
	fread(content, size, 1, fp);
	fclose(fp);

	WavHeader_t header;
	memcpy(&header, content, sizeof(WavHeader_t));
	memcpy(RIFF, header.RIFF, 4);
	memcpy(WAVE, header.WAVE, 4);
	memcpy(data, header.data, 4);
	if (strcmp(RIFF, "RIFF") || strcmp(WAVE, "WAVE")) {
		fprintf(stderr, "这看起来不像是一个wav文件[思考]\n");
		return 3;
	}
	SAMPLE_RATE = header.sample_rate;

	/* 寻找data段 */
	p = content+sizeof(WavHeader_t) - 8;
	while((strcmp(data, "data") || *(p-1) != 0) && p != content+size) {
		p++;
		memcpy(data, p, 4);
	}
	memcpy(&header.data_size, p+4, 4);

	printf("channels:%d\nsample_rate:%d\nbyte_rate:%d\nblock_align:%d\n"
	       "bits_per_sample:%d\ndata_size:%d\nfile_size:%d\n",
	       header.channels, header.sample_rate, header.byte_rate, header.block_align,
	       header.bits_per_sample, header.data_size/header.block_align, header.file_size);

	snd_pcm_t *pcm_handle = init();
	if (! pcm_handle) return 2;

	play_wav(pcm_handle, (int16_t*)(p+8), header.data_size/header.block_align);

	snd_pcm_drain(pcm_handle);
	snd_pcm_close(pcm_handle);

	free(content);
	return 0;
}

int streamer_file(void *p)
{
	if (!p) return EOF;
	return getc(p);
}

int main(int argc, char *argv[])
{
	int ch = 0;
	double amplitude = 1.0;
	char input[PATH_MAX] = "",
	     wavfile[PATH_MAX] = "";
	struct Melody_opts_t args = {.no_fade=0};
	while ((ch = getopt(argc, argv, "hi:f:nmHPxA:")) != -1) {	/* 获取参数 */
		switch (ch) {
		case '?':
		case 'h':
			printf("Usage: alsa_play [Option]\n"
			       "Option:\n"
			       "    -i <FILE>   输入文件(曲谱)\n"
			       "    -f <FILE>   输入文件(wav)\n"
			       "    -n          取消音符淡入淡出,可能产生杂音\n"
			       "    -m          平滑滑音，滑音频率匀速增长\n"
			       "    -H          取消泛音\n"
			       "    -P          打印音符(格式化)\n"
			       "    -x          打印调试信息\n"
			       "    -A <NUM>    音量系数(默认1.0)\n"
			       "    -h          显示帮助\n"
			       );
			return ch == '?' ? -1 : 0;
			break;
		case 'A':
			amplitude = strtof(optarg, NULL);
			if (amplitude > 100 || amplitude < 0) amplitude = 1.0;
			break;
		case 'i': strlcpy(input, optarg, sizeof(input)); break;
		case 'f': strlcpy(wavfile, optarg, sizeof(input)); break;
		case 'n': args.no_fade = true; break;
		case 'm': args.smooth = true; break;
		case 'H': args.no_har = true; break;
		case 'x': args.print_debug_info = true; break;
		case 'P': args.print_formated_note = true; break;
		default:
			break;
		}
	}
	if (!*input && *wavfile) {
		read_play_wave(wavfile);
		return 0;
	} else if (!*input) return 0;
	FILE *fp = fopen(input, "r");
	if (!fp) return 1;
	MusicCtx_t *ctx = music_ctx_create(1024*50);  /*SAMPLE_RATE*/
	ctx->notes = note_parser(streamer_file, fp, amplitude);
	fclose(fp);

	snd_pcm_t *pcm = init();
	if (! pcm) {
		music_ctx_free(ctx);
		return 1;
	}
	size_t total_size = 0, size = 0;
	while ((size = music_ctx_gen_pcm(ctx))) {
		music_ctx_pcm_to_i16(ctx, 3);
		play_wav(pcm, ctx->pcmf16_buffer, size);
		total_size += size;
		fprintf(stderr, "TS: %.2lfs, CS: %lu, pos: %.2lfs \r",
			(double)total_size/SAMPLE_RATE, size,
			(double)ctx->position/SAMPLE_RATE);
	}
	fprintf(stderr, "\n[Quit]");
	music_ctx_free(ctx);
	// 等待播放完成并关闭设备
	snd_pcm_drain(pcm);
	snd_pcm_close(pcm);
	return 0;
}
