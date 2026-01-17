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

#define ENABLE_ALSA

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <math.h>
#ifdef ENABLE_ALSA
#include <alsa/asoundlib.h>
#endif

#define FLG_PLAY (1 << 0)
#define FLG_SAVE (1 << 1)
/*#define FLG_INPUT (1 << 2)*/
#define FLG_PLYARG (1 << 3)    /* 播放终端传入参数 */
#define FLG_FADE (1 << 4)	/* 淡入淡出 */
#define FLG_HARMONICS (1<<5)	/* 泛音 */
#define FLG_SMMOTH_GLIDE (1<<6)
#define FLG_PRINT (1<<7)
#define FLG_PRINT_CSTYLE (1<<8)
#define FLG_PLYARG_WITH_CONTROL (1<<9)

int status = FLG_FADE|FLG_HARMONICS;
/* 采样率（Hz） */
unsigned int SAMPLE_RATE = 44100;

#ifdef ENABLE_ALSA
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
#endif

// WAV文件头结构
typedef struct {
	char RIFF[4];		// "RIFF"
	uint32_t file_size;	// 文件总大小-8
	char WAVE[4];		// "WAVE"
	char fmt[4];		// "fmt "
	uint32_t fmt_size;	// fmt块大小（16）
	uint16_t audio_format;	// 1=PCM
	uint16_t channels;	// 声道数
	uint32_t sample_rate;
	uint32_t byte_rate;	// 每秒字节数
	uint16_t block_align;	// 每个样本的字节数
	uint16_t bits_per_sample;
	char data[4];		// "data"
	uint32_t data_size;	// 音频数据大小
} WavHeader;

void create_wav_header(WavHeader *header, float duration)
{
	memcpy(header->RIFF, "RIFF", 4);
	memcpy(header->WAVE, "WAVE", 4);
	memcpy(header->fmt, "fmt ", 4);
	memcpy(header->data, "data", 4);

	header->fmt_size = 16;
	header->audio_format = 1;	// PCM
	header->channels = 2;
	header->sample_rate = SAMPLE_RATE;
	header->bits_per_sample = 16;
	header->block_align = header->channels * header->bits_per_sample / 8;
	header->byte_rate = SAMPLE_RATE * header->block_align;

	int data_size = SAMPLE_RATE * duration * header->block_align;
	header->data_size = data_size;
	header->file_size = data_size + sizeof(WavHeader) - 8;
	/*printf("DATA_SIZE:%d\nFILE_SIZE:%d\nDELTER:%d\n",
	 * data_size,header->file_size,header->file_size-data_size);*/
}

/* 
 * 生成固定声波
 * volume: 0% ~ 100%
 * 数学模型: f(x) = A*sin(ω*x)
 * x: f(x)的x值
 * f: 频率ω
 * */
double wave(double x, double volume, double f)
{
	if (f < 0) return 0;
	static const double harmonicsp[15] = {
		1,0.340,0.102,0.085,0.070,0.065,0.028,0.085,0.011,0.030,0.010,0.014,0.012,0.013,0.004
	};
	const short max_vol = (unsigned short)(-1) / 2;
	double value = 0.0;
	// 基频 + 14个泛音（振幅递减）
	for (int i = 0; i < 15; i++) {
		value += harmonicsp[i] * sin(2 * M_PI * (i+1) * f * x);	// 基频
		if (!(status&FLG_HARMONICS))
			break;
	}
	return volume * max_vol * value;
}

double adsr_envelope(int current, int total)
{
	if (current < 0 || current > total)
		return 0;
	if (!(status&FLG_FADE)) return 1.0;
	double t = (double)current / total;
	double attack = 0.01, decay = 0.1, sustain = 0.7, release = 0.2;
	if (t < attack)
		return t / attack;	// 起振（线性增长）
	else if (t < attack + decay)
		return 1.0 - (1.0 - sustain) * (t - attack) / decay;	// 衰减
	else if (t < 1.0 - release)
		return sustain;	// 持续
	else
		return sustain * (1.0 - (t - (1.0 - release)) / release);	// 释放
}

double get_glide_freq(double start, double end, double x)
{
	double s = 1/(1+exp(-20*(x-0.5)));
	if (status&FLG_SMMOTH_GLIDE)
		s = x;
	double ret = log(start) + (log(end)-log(start))*s;
	return exp(ret);
}

struct Note {
	double freq;
	double amplitude;
	int duration;    /* base on sample point */
	int enable_glide;    /* 连音符 */
	int enable_slide;    /* 滑动频率 */
	int use_right_side;
};

/* 
 * 解读字符串形式的音符并产生对应声波
 * note: C D E F G A B
 * type: 默认为n分音符(或者在note后缀的控制符中控制)
 * base: 以n分音符为一拍
 * speed: 速度，n拍/分钟
 * */
short *create_note_wave(const char *note, double type, int base, double speed, double *amplitude)
{
	if (note == NULL) return NULL;
	struct Note note_list[20] = {};
	struct Note *current_note = note_list;
	const double note_freq[256] = { 0,
		['c'] = 130.8, ['d'] = 146.8, ['e'] = 164.8, ['f'] = 174.6,
		['g'] = 196.0, ['a'] = 220.0, ['b'] = 246.9,
		['C'] = 261.6, ['D'] = 293.6, ['E'] = 329.6, ['F'] = 349.2,
		['G'] = 392.0, ['A'] = 440.0, ['B'] = 493.9,
		['1'] = 523.2, ['2'] = 587.3, ['3'] = 659.2, ['4'] = 698.5,
		['5'] = 784.0, ['6'] = 880.0, ['7'] = 987.8, ['0'] = -1,
		['{'] = 20, ['}'] = 20000};
	int counter = -1;
	static double fadeout = 0.0;
	*amplitude *= (1+fadeout);
	if (*amplitude > 1 || *amplitude < 0) {
		int istty = isatty(STDERR_FILENO);
		char *colors[2][2] = {{"", ""}, {"\e[31m", "\e[0m"}};
		fprintf(stderr, "[%sWARN%s amplitude out of range:%lf]\n",
			colors[istty][0], colors[istty][1], *amplitude);
		*amplitude /= (1+fadeout);
	}

	for (const char *p = note; *p != '\0' && counter < 20; p++) {
		if (!note_freq[(int)*p] && !current_note->freq)
			continue;
		if (note_freq[(int)*p]) {
			if (current_note->freq)
				current_note++;
			current_note->freq = note_freq[(int)*p];
			current_note->duration = SAMPLE_RATE*(base/type)*(60/speed);
			current_note->amplitude = *amplitude;
			counter++;
		} else if (*p == '/')
			current_note->duration /= 2;
		else if (*p == '*')
			current_note->duration *= 2;
		else if (*p == '.')
			current_note->duration *= (3.0/2.0);
		else if (*p == '~')
			current_note->enable_glide = 1;
		else if (*p == 's')
			current_note->enable_slide = 1;
		else if (*p == '+')
			current_note->amplitude += current_note->amplitude + 0.1 <= 1 ? 0.1 : 0;
		else if (*p == '-')
			current_note->amplitude -= current_note->amplitude - 0.1 >= 0 ? 0.1 : 0;
		else if (*p == 'l')
			current_note->freq-=10;
		else if (*p == 'L')
			current_note->freq/=2;
		else if (*p == 'u')
			current_note->freq+=10;
		else if (*p == 'U')
			current_note->freq*=2;
		else if (*p == 'r')
			current_note->use_right_side = 1;
		else if (*p == '<')
			fadeout += 0.005;
		else if (*p == '>')
			fadeout -= 0.005;
		else if (*p == '=')
			fadeout = 0;
		else
			continue;
		if (status&FLG_PRINT) printf("%c", *p);
	}
	current_note = NULL;
	unsigned int buffer_frames = 0;
	int enable_glide = 0;
	for (int i = 0; i < 20 && note_list[i].amplitude != 0; i++) {
		if (note_list[i].duration > buffer_frames)
			buffer_frames = note_list[i].duration;
		if (note_list[i].enable_glide) {
			enable_glide += buffer_frames;
			buffer_frames = 0;
		}
	}
	buffer_frames += enable_glide;
	enable_glide = enable_glide ? 1 : 0;

	if (!(status&(FLG_PLAY|FLG_SAVE))) {
		int *buffer = malloc(sizeof(int)*1);
		*buffer = buffer_frames;
		return (short*)buffer;
	}

	/* 缓冲区大小，同时控制声音片段大小
	 * 缓冲区越大声音持续越久，也越慢 */
	short *buffer = malloc(sizeof(short)*(buffer_frames * 2 + 2));    /* 双声道 */
	memset(buffer, 0, sizeof(short)*(buffer_frames * 2 + 2));
	*(int*)buffer = buffer_frames;
	struct Note *p = note_list;
	int use_right_side = 0;
	int flag_slide = 0;
	for (int i = 0; i < 20 && p->amplitude != 0; i++,p++) {
		short num = 0;
		if (p->enable_slide) {
			flag_slide = p->freq;
			continue;
		}
		for (int i = 0; i < buffer_frames; i++) {    /* 生成每个采样点的声波 */
			double env = adsr_envelope(i - (p->enable_glide || enable_glide ? 0 : enable_glide),
						   (p->enable_glide || enable_glide) ? buffer_frames : p->duration)*p->amplitude;
			double freq = flag_slide ? get_glide_freq(flag_slide, p->freq, (double)i/p->duration) : p->freq;
			num = (short)wave((double)i/SAMPLE_RATE, env, freq);
			if (p->enable_glide)
				num *= (1 - 1/(1+exp((double)(p->duration+enable_glide-i)/800)));
			if (enable_glide)
				num *= (1/(1+exp((double)(enable_glide-i)/800)));
			if (use_right_side == 1)
				buffer[i * 2 + 3] = num;    /* 右声道 */
			else if (use_right_side == 2)
				buffer[i * 2 + 3] += num;    /* 右声道 */
			else {
				buffer[i * 2 + 2] += num;    /* 左声道 */
				buffer[i * 2 + 3] += num;    /* 右声道 */
			}
		}
		if (use_right_side == 1)
			use_right_side = 2;
		if (p->use_right_side)
			use_right_side = 1;
		if (p->enable_glide) {
			use_right_side = 0;
			enable_glide += p->duration;
		}
	}
	return buffer;
}

#ifdef ENABLE_ALSA
int play_wav(snd_pcm_t *pcm_handle, short *wave, int size) {
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
#endif

char *str_in_group(char *p, char *splitors, int id)
{
	if (!p || !splitors) return NULL;

	int splitor_table[256] = {0};
	for (char *sp = splitors;sp && *sp; sp++) splitor_table[(int)*sp] = 1;

	int i = 0, il = 0, count = 0, flag = 2;
	for (; p[i] && flag; i++) {
		if (! splitor_table[(int)p[i]]) {
			flag = 1;
			continue;
		}
		if (flag == 2) {
			il = i+1;
			continue;
		}
		if (count >= id) {
			flag = 0;
			break;
		}
		il = i+1;
		count++;
		flag = 2;
	}
	if (id > count) return NULL;
	if (!p[i]) return p+il;
	static char *ret = NULL;
	if (ret) free(ret);
	ret = malloc(i-il+1);
	memset(ret, 0, i-il+1);
	memcpy(ret, p+il, i-il);
	return ret;
}

int melody(int id, char *filename, int speed, double amplitude, char *input)
{
	id %= 7;
	const char *note[7][514] = {
		{
			"4 4",
			"C","C","G","G","A","A","G*", "F","F","E","E","D","D","C*",    /* 一闪一闪亮晶晶 */ /* 满天都是小星星 */
			"G","G","F","F","E","E","D*", "G","G","F","F","E","E","D*",    /* 挂在天上放光明 */ /* 好像许多小眼睛 */
			"C","C","G","G","A","A","G*", "F","F","E","E","D","D","C*",    /* 一闪一闪亮晶晶 */ /* 满天都是小星星 */
		}, {
			"2 4",
			"C/","C/","C",       "g/","a/","g",     "E/.","D//","C/","a/", "D*",
			"E/","E/","E",       "G/","G/","G",     "A/.","G//","C/","E/", "D*",
			"E*",                "G*",              "E/.","D//","C/","D/", "a*",
			"As1*",              "GsE*",            "C","E",               "G*",
			"A/","0/","G/","0/", "E/","0/","G/","0/",
			"1.++","A/","G/","0/","E/","0/",	/* 打败美帝 */
			"G/","0/","D/","0/","C","0",		/* 野心狼 */
			"C/","C/","C",       "g/","a/","g",     "E/.","D//","C/","a/", "D*",
			"E/","E/","E",       "G/","G/","G",     "A/.","G//","C/","E/", "D*",
			"E*",                "G*",              "E/.","D//","C/","D/", "a*",
			"As1*",              "GsE*",            "C","E",               "G*",
			"A/","0/","G/","0/", "E/","0/","G/","0/",
			"1.++","A/","G/","0/","E/","0/",	/* 打败美帝 */
			"1/","0/","G/","0/", "1","0"		/* 野心狼 */
		}, {
			"4 4",
			"C","C","C","C","C","C","C","C","C","C",
			"C","C","C","C","C","C","C","C","C","C",    /* 10s音频测试 */
		}, {
			"4 4",
			"c","d","e","f","g","a","b",
			"C","D","E","F","G","A","B",
			"1","2","3","4","5","6","7",    /* 10.5s音频测试 */
		}, {
			"2 4",
			"3/.","2//","1/","A/", "G/","A//","G//","E/","G/", "1/","1//","1//","1/","1/",
			"1","C//","D//","E//","F//",    /* 前奏 */
			"G","E/.","D//", "C/","E/","G/","1/", "3","2/.","3//", "1*",
			"2","2/.","3//", "2/","1/","B/","A/", "G","A/.","1//", "G*", 
			"E/","E//","E//","D/","E/", "G/","E//","G//","A/","G/", "1","2", "3*",
			"2/.","1//","B/","A/", "G/","A//","G//","E/","G/", "1","1/","1/", "1",
			"G/.","A//", "E", "G", "1/","1/","2/","1/", "G*",    /* 为了五大洲的友谊 */
			"G","A/.","G//", "E","G", "1/","1/","2/.","1//", "3*",    /* 为了全人类的理想 */
			"3","3/","5/", "4.","3/", "2/","B/","A/","G//","G//", "3.","2/", "1",    /* 为了发扬奥林匹克的精神 */
			"B/","A/", "G.","E/", "G/","G//","A//","B/","1/", "2*", "2",    /* 我们竞技在那运动场 */
			"G/.","A//", "E", "G", "1/","1/","2/","1/", "G*",    /* 为了五大洲的友谊 */
			"G","A/.","G//", "E","G", "1/","1/","2/.","1//", "3*",    /* 为了全人类的理想 */
			"3","3/","5/", "4.","3/", "2/","B/","A/","G//","G//", "3.","2/", "1",    /* 为了发扬奥林匹克的精神 */
			"B/","A/", "G.","E/", "G/","A/","B/","2/", "1*",    /* 我们竞技在那运动场 */
			"1","C//","D//","E//","F//",
			/* REPEAT NEXT */
			"G","E/.","D//", "C/","E/","G/","1/", "3","2/.","3//", "1*",
			"2","2/.","3//", "2/","1/","B/","A/", "G","A/.","1//", "G*", 
			"E/","E//","E//","D/","E/", "G/","E//","G//","A/","G/", "1","2", "3*",
			"2/.","1//","B/","A/", "G/","A//","G//","E/","G/", "1","1/","1/", "1",
			"G/.","A//", "E", "G", "1/","1/","2/","1/", "G*",    /* 为了五大洲的友谊 */
			"G","A/.","G//", "E","G", "1/","1/","2/.","1//", "3*",    /* 为了全人类的理想 */
			"3","3/","5/", "4.","3/", "2/","B/","A/","G//","G//", "3.","2/", "1",    /* 为了发扬奥林匹克的精神 */
			"B/","A/", "G.","E/", "G/","G//","A//","B/","1/", "2*", "2",    /* 我们竞技在那运动场 */
			"G/.","A//", "E", "G", "1/","1/","2/","1/", "G*",    /* 为了五大洲的友谊 */
			"G","A/.","G//", "E","G", "1/","1/","2/.","1//", "3*",    /* 为了全人类的理想 */
			"3","3/","5/", "4.","3/", "2/","B/","A/","G//","G//", "3.","2/", "1",    /* 为了发扬奥林匹克的精神 */
			"B/","A/", "G.","E/", "G/","A/","B/","2/", "1*",    /* 我们竞技在那运动场 */
			"1", "0",    /* 运动员进行曲 */
		}, {
			"6 8 #如果嫌慢可设置bpm:180,\
			由于本曲有八六八九两种拍子，但不支持多拍，\
			报WARN说有9拍的属正常现象",

			"1/", "2/", "3/", "2/", "1/", "A/", "B/", "A./", "E//", "G.~G.",
			"1/", "2/", "3/", "2/", "1/", "A/", "B/", "G./", "E//", "A.~A.",
			"G/", "F/", "E/", "D.", "b/", "a/", "g/", "E.", "F.", "D", "C/", "C.~C.",    /* 前奏 */

			"G/~A/", "G/", "F/~E/", "D/", "C.", "g.",		/* 我和~我~的~祖~国~ */
			"C/", "E/", "1/", "B/", "A./", "E//", "G.~G.",		/* 一刻也不能分割 */
			"A/", "B/", "A/", "G/~F/", "E/", "D.", "a.",		/* 无论我走到哪里 */
			"b/", "a/", "g/", "G/", "C./", "D//", "E.~E.",		/* 都流出一首赞歌 */
			"G/", "A/", "G/", "F/", "E/", "D/", "C.", "g.",		/* 我歌唱每一座高山 */
			"C/", "E/", "1/", "B/", "2./", "1//", "A.~A.",		/* 我歌唱每一条河 */
			"1/", "B/", "A/", "G.",			/* 袅袅炊烟 */
			"A/", "G/", "F/", "E.",			/* 小小村落 */
			"b", "a/", "g/~g/", "D/", "C.~C.",	/* 路下一道辙 */
			"1/", "2/~3/", "2/", "1/", "A/", "B/~A./~E//", "G.~G.",			/* 我最亲爱的祖~国~ */
			"1/", "2/", "3/", "2/", "1/", "A/", "B/", "G./", "E//", "A.~A.",	/* 我永远紧依着你的心窝 */
			"G/", "F/", "E/", "D.",			/* 你用你那 */
			"b/", "a/", "g/", "E.",			/* 母亲的脉搏 */
			"F.", "D", "C/", "C.~C", "0/",		/* 和我诉说 */
			/* 重复上一段 */
			"G/~A/", "G/", "F/~E/", "D/", "C.", "g.",		/* 我的~祖~国~和~我~ */
			"C/", "E/", "1/", "B/", "A./", "E//", "G.~G.",		/* 像海和浪花一朵 */
			"A/", "B/", "A/", "G/~F/", "E/", "D.", "a.",		/* 浪是那海的赤子 */
			"b/", "a/", "g/", "G/", "C./", "D//", "E.~E.",		/* 海是那浪的依托 */
			"G/", "A/", "G/", "F/", "E/", "D/", "C.", "g.",		/* 每当大海在微笑 */
			"C/", "E/", "1/", "B/", "2./", "1//", "A.~A.",		/* 我就是笑的酒窝 */
			"1/", "B/", "A/", "G.",			/* 我分担着 */
			"A/", "G/", "F/", "E.",			/* 海的忧愁 */
			"b", "a/", "g/~g/", "D/", "C.~C.",	/* 分享海的欢乐 */
			"1/", "2/~3/", "2/", "1/", "A/", "B/~A./~E//", "G.~G.",			/* 我最亲爱的祖~国~ */
			"1/", "2/", "3/", "2/", "1/", "A/", "B/", "G./", "E//", "A.~A.",	/* 你是大海永不干涸 */
			"G/", "F/", "E/", "D.",			/* 永远给我 */
			"b/", "a/", "g/", "E.",			/* 碧浪清波 */
			"F.", "D", "C/", "C.~C", "0/",		/* 心中的歌 */
			/* 结束句 */
			"1/", "2/", "3/", "2/", "1/", "A/", "B/~A./~E//", "G.~G.",		/* 我最亲爱的祖~国~ */
			"1/", "2/", "3/", "2/", "1/", "A/", "B/", "G./", "E//", "A.~A.",	/* 你是大海永不干涸 */
			"G/", "F/", "E/", "D.",			/* 永远给我 */
			"b/", "a/", "g/", "E.",			/* 碧浪清波 */
			"G.", "2", "1/","1.~1.", "0/"	/* 心中的歌 */
			/* 我和我的祖国 */
		}, {
			"2 4",
			"C./", "E//", "G/", "G/", "A", "G", "E./", "C//", "G/", "G//", "G//",
			"E", "C", "g/", "g//", "g//", "g/", "g//", "g//", "C", "0/",
			"g/", "C.", "C/", "C./", "C//", "g/", "a//", "b//", "C", "C","0/",
			"E/", "C/", "D//", "E//", "G", "G", "E./", "E//", "C./", "E//", "G./", "E//", "D", "D*",
			"A", "G", "D", "E", "G/", "E/", "0/", "G/", "E/", "D//", "E//", "C", "E", "0",
			"g./", "a//", "C/", "C/", "E./", "E//", "G/", "G/", "D/", "D//", "D//", "a", "D.",
			"g/","C.",
			"C/", "E.",
			"E/", "G*",
			"C./", "E//", "G/", "G/", "A", "G",
			"E./", "C//", "G/", "G//", "G//", "E/", "0/","C/", "0/",
			"g", "C", "E./", "C//", "G/", "G//", "G//", "E/", "0/", "C/", "0/",
			"g", "C", "g", "C", "g", "C", "C", "0",
			/* 义勇军进行曲 */
		}
	};
	/*
	 * c d e f g a b
	 * C D E F G A B
	 * 1 2 3 4 5 6 7
	 * */

	FILE *wav_file = NULL;
	WavHeader wav_header;
	double size = 0;
	if (status & FLG_SAVE) {
		wav_file =  fopen(filename, "wb");
		 /* 创建并写入WAV文件头 */
		if (!wav_file)
			fprintf(stderr, "打开文件不成: %s\n", filename);
		create_wav_header(&wav_header, size);
		fwrite(&wav_header, 1, sizeof(wav_header), wav_file);
	}

#ifdef ENABLE_ALSA
	snd_pcm_t *pcm_handle = NULL;
	if (status & FLG_PLAY) {
		// ALSA PCM设备配置
		pcm_handle = init();
		if (! pcm_handle) return 1;
	}
#endif

	int i = 0;
	int type = 4, base = 4;
	if (!(status&FLG_PLYARG))
		sscanf(note[id][0], "%d %d", &type, &base);
	else if (status&FLG_PLYARG_WITH_CONTROL) {
		/* base分音符为一拍, 每小节type拍 */
		sscanf(input, "%d %d", &type, &base);
		i+=2;
	}
	char *p = status&FLG_PLYARG ? input : (char*)note[id][i+1];
	double tempo_counter = 0;
	for (; (status&FLG_PLYARG && p && p[0]) || (!(status&FLG_PLYARG) &&note[id][i+1] != NULL); i++) {
		p = status&FLG_PLYARG ? \
		    (input) : \
		    (char*)note[id][i+1];
		p = str_in_group(p, "| \r\n", status&FLG_PLYARG ?i:0);
		/*printf(" -> ret:%s\n", p);*/
		if (!p || !*p) continue;
		if (status&FLG_PRINT)
			printf(status&FLG_PRINT_CSTYLE?"\"":"");
		short *wave = create_note_wave(p, 4, base, speed, &amplitude);
		if (!wave || *(int*)wave <= 10) {
			fprintf(stderr,"波形为空,Code:[%d], ind:%d, str:'%s'\n",
			       wave ? *(int*)wave : -1, i, p);
			free(wave);
			continue;
		}
		if (status&FLG_PRINT) {
			printf(status&FLG_PRINT_CSTYLE?"\", ":" ");
			tempo_counter += round(((double)*(int*)wave / SAMPLE_RATE * speed/60)*100)/100;
			char *colors[2][2] = {{"", ""}, {"\e[31m", "\e[0m"}};
			int istty = isatty(STDERR_FILENO);
			int istty2 = isatty(STDOUT_FILENO);
			if (tempo_counter >= type && !(status&FLG_PRINT_CSTYLE)) {
				if (tempo_counter > type) {
					printf("%s%s<<%s", istty2 ? "\e[7m" : "", colors[istty2][0], colors[istty2][1]);
					fprintf(stderr, "[%sWARN%s 不符合的拍子(%d:'%s':%f)，可能有计算错误]\n",
						colors[istty][0], colors[istty][1], i, p, tempo_counter);
				}
				/*printf("%s(%f)| %s", colors[istty][0], tempo_counter, colors[istty][1]);*/
				printf("%s| %s", colors[istty2][0], colors[istty2][1]);
				tempo_counter = 0;
			}
		}
		size+=(double)*(int*)wave/SAMPLE_RATE;
		if (status & FLG_SAVE) fwrite(wave+2, sizeof(int), *(int*)wave, wav_file);
#ifdef ENABLE_ALSA
		if (status & FLG_PLAY) play_wav(pcm_handle, wave+2, *(int*)wave);
#endif
		free(wave);
	}
	if (status&FLG_PRINT)
		printf("\n");
	if (status & FLG_SAVE) {
		fseek(wav_file, 0, SEEK_SET);
		create_wav_header(&wav_header, size);	// 重新生成准确的头信息
		fwrite(&wav_header, 1, sizeof(wav_header), wav_file);
		fclose(wav_file);
	}

	if (status & FLG_PLAY) {
		// 等待播放完成并关闭设备
#ifdef ENABLE_ALSA
		snd_pcm_drain(pcm_handle);
		snd_pcm_close(pcm_handle);
#endif
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

	WavHeader header;
	memcpy(&header, content, sizeof(WavHeader));
	memcpy(RIFF, header.RIFF, 4);
	memcpy(WAVE, header.WAVE, 4);
	memcpy(data, header.data, 4);
	if (strcmp(RIFF, "RIFF") || strcmp(WAVE, "WAVE")) {
		fprintf(stderr, "这看起来不像是一个wav文件[思考]\n");
		return 3;
	}
	SAMPLE_RATE = header.sample_rate;

	/* 寻找data段 */
	p = content+sizeof(WavHeader) - 8;
	while((strcmp(data, "data") || *(p-1) != 0) && p != content+size) {
		p++;
		memcpy(data, p, 4);
	}
	memcpy(&header.data_size, p+4, 4);

	printf("channels:%d\nsample_rate:%d\nbyte_rate:%d\nblock_align:%d\n"
	       "bits_per_sample:%d\ndata_size:%d\nfile_size:%d\n",
	       header.channels, header.sample_rate, header.byte_rate, header.block_align,
	       header.bits_per_sample, header.data_size, header.file_size);

#ifdef ENABLE_ALSA
	snd_pcm_t *pcm_handle = init();
	if (! pcm_handle) return 2;

	play_wav(pcm_handle, (short*)(p+8), header.data_size);

	snd_pcm_drain(pcm_handle);
	snd_pcm_close(pcm_handle);
#endif

	free(content);
	return 0;
}

int main(int argc, char *argv[])
{
	int ch = 0, id = 0, speed = 120;
	double amplitude = 0.2;
	char filename[125] = "output.wav",
	     *notes = NULL;
	while ((ch = getopt(argc, argv, "hi:psnmHo:S:r:C:PcdA:")) != -1) {	/* 获取参数 */
		switch (ch) {
		case '?':
		case 'h':
			printf("Usage: ALSA [Option]\n"
			       "Option:\n"
			       "    -i <NUM>  选择曲子\n"
			       "    -p        播放曲子(使用ALSA)\n"
			       "    -s        保存曲子(单设-o无用)\n"
			       "    -n        取消音符淡入淡出,可能产生杂音\n"
			       "    -m        平滑滑音，滑音频率匀速增长\n"
			       "    -H        取消泛音\n"
			       "    -o <FILE> 输出文件(output.wav)\n"
			       "    -S <NUM>  设置曲速(120)\n"
			       "    -r <FILE> 输入文件（启用后其他选项无效）\n"
			       "    -C <STR>  额外指定音符(用`|`或` `分割)\n"
			       "    -d        指定额外音符时读取头两个数字作拍号\n"
			       "    -P        打印音符(格式化)\n"
			       "    -c        打印音符时采用c语言样式\n"
			       "    -A <NUM>  基本音量(0~1,默认0.2)\n"
			       "    -h        显示帮助\n"
			       "  NUM: 0: 小星星\n"
			       "       1: 中国人民志愿军战歌\n"
			       "       2: 20s音频测试\n"
			       "       3: 10.5升调音频测试\n"
			       "       4: 运动员进行曲\n"
			       "       5: 我和我的祖国\n"
			       "       6: 义勇军进行曲\n"
#ifndef ENABLE_ALSA
			       "[!] 本程序在编译时关闭了`ENABLE_ALSA`\n"
			       "    意味着无法提供任何ALSA方面的支持\n"
#endif
			       );
			return ch == '?' ? -1 : 0;
			break;
		case 'i':
			id = strtod(optarg, NULL);
			break;
		case 'p':
			status |= FLG_PLAY;
			break;
		case 's':
			status |= FLG_SAVE;
			break;
		case 'n':
			status &= ~FLG_FADE;
			break;
		case 'm':
			status |= FLG_SMMOTH_GLIDE;
			break;
		case 'H':
			status &= ~FLG_HARMONICS;
			break;
		case 'o':
			strcpy(filename, optarg);
			break;
		case 'S':
			speed = strtod(optarg, NULL);
			break;
		case 'A':
			amplitude = strtof(optarg, NULL);
			break;
		case 'r':
			strcpy(filename, optarg);
			return read_play_wave(filename);
			break;
		case 'C':
			if (notes)
				break;
			status |= FLG_PLYARG;
			notes = malloc(sizeof(char)*(strlen(optarg)+1));
			memset(notes, 0, sizeof(char)*(strlen(optarg)+1));
			strcpy(notes, optarg);
			break;
		case 'P':
			status |= FLG_PRINT;
			break;
		case 'c':
			status |= FLG_PRINT_CSTYLE;
			break;
		case 'd':
			status |= FLG_PLYARG_WITH_CONTROL;
			break;
		default:
			break;
		}
	}
	speed = speed <= 2 ? 120 : speed;
	melody(id, filename, speed, amplitude, notes);
	if (notes)
		free(notes);
	return 0;
}
