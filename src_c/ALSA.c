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
#include <string.h>
#include <limits.h>
#include <stdint.h>
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
#define FLG_SMOOTH_PORTAMENTO (1<<6)
#define FLG_PRINT (1<<7)

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
 * x: f(x)的x值 (单位:sec)
 * f: 频率ω
 * */
double gen_wave(double x, double volume, double f)
{
	if (f < 0) return 0;
	static const double harmonicsp[15] = {
		1,0.340,0.102,0.085,0.070,0.065,0.028,0.085,0.011,0.030,0.010,0.014,0.012,0.013,0.004
	};
	double value = 0.0;
	// 基频 + 14个泛音（振幅递减）
	for (int i = 0; i < 15; i++) {
		value += harmonicsp[i] * sin(2 * M_PI * (i+1) * f * x);	// 基频
		if (!(status&FLG_HARMONICS))
			break;
	}
	return INT16_MAX * volume * value;
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

#define sigmoid(x) (1 - 1/(1+exp(x)))

/*
 * 获取应有频率
 * start: 初始频率
 * end: 目标频率
 * x: 0-1
 * */
double get_portamento_freq(double start, double end, double x)
{
	/* s:获取前后两个波形的音量占比
	 * x=0.5附近从0平滑跃变为1 || y=kx */
	double s = status&FLG_SMOOTH_PORTAMENTO ? x : sigmoid(20*(x-0.5));
	/* 对数和指数计算让音调“均匀”变化 */
	double ret = log(start) + (log(end)-log(start))*s;
	return exp(ret);
}

#define BUFFSIZE 2048
#define SCR_HEIGHT 4

#define NFLG_LEFT (1 << 0)
#define NFLG_RIGHT (1 << 1)
#define NFLG_LEGATO (1 << 2)
#define NFLG_PORTAMENTO (1 << 3)
#define NFLG_BE_LEGATO (1 << 4)

struct Note {
	double freq;
	double amplitude;
	double   type;		/* type分音符 */
	uint8_t  notes;		/* 以notes分音符为一拍 */
	uint8_t  beates;	/* 每小节beates拍 */
	uint16_t speed;		/* 速度，n拍/分钟 */
	uint64_t duration;
	uint8_t  track;
	uint8_t  flag;		/* NFLG_* flags */
	int16_t *pwav;
	struct Note *pNext;
};

struct Note *create_note(double freq)
{
	struct Note *p = malloc(sizeof(struct Note));
	if (!p) return NULL;
	*p = (struct Note){
		freq, .amplitude=0.5,
		.type=4, .notes=4, .beates=4,
		.speed=120, .duration=0, .track=0,
		.flag=NFLG_LEFT|NFLG_RIGHT,
		.pwav=NULL, NULL
	};
	return p;
}

char *i2b(unsigned long data, int bytes, char *buf, size_t buf_size)
{
	if (!buf || buf_size == 0) return NULL;
	if (bytes <= 0 || bytes > sizeof(data)) bytes = sizeof(data);

	int bits = bytes * 8;
	if (bits >= buf_size) bits = buf_size - 1;  // 留1字节给'\0'

	for (int i = 0; i < bits; ++i) {
		unsigned long mask = 1UL << (bits - 1 - i);  // 1UL 保证至少 32/64 位
		buf[i] = (data & mask) ? '1' : '0';
	}
	buf[bits] = '\0';
	return buf;
}

/* 解读字符串形式的音符并产生解析好的音符串struct */
struct Note *parse_notes(const char *str, double amplitude)
{
	static const double note_freq[256] = { 0,
		['c'] = 130.8, ['d'] = 146.8, ['e'] = 164.8, ['f'] = 174.6,
		['g'] = 196.0, ['a'] = 220.0, ['b'] = 246.9,
		['C'] = 261.6, ['D'] = 293.6, ['E'] = 329.6, ['F'] = 349.2,
		['G'] = 392.0, ['A'] = 440.0, ['B'] = 493.9,
		['1'] = 523.2, ['2'] = 587.3, ['3'] = 659.2, ['4'] = 698.5,
		['5'] = 784.0, ['6'] = 880.0, ['7'] = 987.8, ['0'] = -1,
		['{'] = 20, ['}'] = 20000};
	char scr[BUFSIZ] = "",
	     scr2[BUFSIZ] = "",
	     buf[30] = "",
	     key[40] = "",
	     value[40] = "";
	double fade = 0;
	uint8_t track = 0, setting_mode = 0;
	int beates = 4, /* 每小节type拍(重置计数器用) */
	    notes = 4,  /* 以几分音符为一拍 */
	    type = 4,   /* 默认type分音符 */
	    speed = 120,
	    c = 0;
	struct Note *pH = NULL, *p = NULL;
	while (c != EOF) {
		if (str) {
			c=*str;
			if (!c) break;
			str++;
		} else c=getchar();
		if (c==EOF) break;
		if (c >= 256 || c < 0) continue;

		if (setting_mode == 0) {
		} else if (setting_mode <= 40) {
			if (c != '=') key[setting_mode-1] = c;
			else setting_mode = 40;
			setting_mode++;
			continue;
		} else if (setting_mode <= 40+40) {
			if (c != ';') value[setting_mode-41] = c;
			else {
				setting_mode = 0;
#define ifin(str, fmt, var, check) if (strcmp(key, str) == 0) {sscanf(value, fmt, &var); check;} else
				ifin("track", "%hhd", track, )
				ifin("speed", "%d", speed, if(speed<1) speed=120)
				ifin("beates", "%d", beates, if(beates<1) )
				ifin("notes", "%d", notes, if(notes%4!=0||notes<1) notes=4)
				ifin("type", "%d", type, if(type%4!=0||type<1)type=4);
#undef ifin
				memset(key, 0, 40);
				memset(value, 0, 40);
				continue;
			}
			setting_mode++;
			continue;
		} else {
			setting_mode = 0;
		}

		if (note_freq[c]) {
			struct Note *p2 = p;
			p = create_note(note_freq[c]);
			/*p->duration = SAMPLE_RATE*((double)base/4)*((double)60/speed);*/
			p->amplitude = amplitude;
			p->type = type;
			p->track = track;
			p->speed = speed;
			if (!pH) pH = p;
			if (p2) {
				p2->pNext = p;
				p->flag |= p2->flag&NFLG_LEGATO ? NFLG_BE_LEGATO : 0;
				if (fade) p->amplitude = p2->amplitude*(1.0+fade);
			}
		} else if (c == ':') {
			setting_mode = 1;
		} else if (!p) continue;

		switch (c) {
		case '|':
		case ';':
		case ' ':
		case '\r':
		case '\n':
		case '\t':
			/*if (p) free(p);*/
			/*p = NULL;*/
			c = 0;
			break;
		case '/':
			p->type *= 2;
			break;
		case '*':
			p->type /= 2;
			break;
		case '.':
			p->type /= (3.0/2.0);
			break;
		case '~':
			p->flag ^= NFLG_LEGATO;
			break;
		case 's':
			p->flag ^= NFLG_PORTAMENTO;
			break;
		case '+':
			p->amplitude += p->amplitude + 0.1 <= 1 ? 0.1 : 0;
			break;
		case '-':
			p->amplitude -= p->amplitude - 0.1 >= 0 ? 0.1 : 0;
			break;
		case 'l':
			p->freq-=10;
			break;
		case 'L':
			p->freq/=2;
			break;
		case 'u':
			p->freq+=10;
			break;
		case 'U':
			p->freq*=2;
			break;
		case '[':
			p->flag ^= NFLG_LEFT;
			break;
		case ']':
			p->flag ^= NFLG_RIGHT;
			break;
		case '<':
			fade += 0.005;
			break;
		case '>':
			fade -= 0.005;
			break;
		case '=':
			fade = 0;
			break;
		case ':':
			setting_mode = 1;
			break;
		case 'q':
			c = EOF;
			break;
		}
	}
	return pH;
}

#define pduration(p) (SAMPLE_RATE*((double)p->notes/p->type)*((double)60/p->speed))
/*
 * 解读字符串形式的音符并产生对应声波
 * RET: duration
 * */
int create_note_wave(struct Note **pp)
{
	if (!pp || !*pp) return -1;
	struct Note *p = *pp;
	uint32_t sample_num = pduration(p);
	if (p->flag&(NFLG_LEGATO|NFLG_PORTAMENTO)) {
		uint8_t flag = p->flag&(NFLG_LEGATO|NFLG_PORTAMENTO);
		sample_num = 0;
		for (struct Note *p2 = p;
		     p2 && p2->flag&flag && p2->track == p->track;
		     p2=p2->pNext) {
			sample_num += pduration(p2);
			*pp = p2;
		}
		if ((*pp)->pNext) {
			*pp = (*pp)->pNext;
			sample_num += pduration((*pp));
		}
	}

	if (!(status&(FLG_PLAY|FLG_SAVE))) {
		(**pp).pwav = NULL;
		return -2;
	}

	/* 缓冲区大小，同时控制声音片段大小
	 * 缓冲区越大声音持续越久，也越慢 */
	int16_t *buffer = malloc(sizeof(uint16_t)*(sample_num * 2));    /* 双声道 */
	if (!buffer) return -3;
	memset(buffer, 0, sizeof(uint16_t)*(sample_num * 2));
	int flag_slide = 0;    /* 启用滑音时的初始频率 */
	int duration_offset = 0;
	for (; p ; p = p->pNext) {
		int16_t phase = 0;
		if (p->flag&NFLG_PORTAMENTO) {    /* 滑音 != 连音 */
			flag_slide = p->freq;
			continue;
		}
		uint32_t duration = pduration(p);
		for (int i = 0; i < sample_num; i++) {    /* 生成每个采样点声波的相位 */
			double env = adsr_envelope(i, sample_num)*p->amplitude;
			double freq = flag_slide ? get_portamento_freq(flag_slide, p->freq, (double)i/sample_num) : p->freq;
			phase = (int16_t)gen_wave((double)i/SAMPLE_RATE, env, freq);
			if (p->flag&NFLG_BE_LEGATO)    /* fade in */
				phase *= sigmoid((int32_t)(i-duration_offset)/((double)SAMPLE_RATE/100));
			if (p->flag&NFLG_LEGATO)    /* fade out */
				phase *= sigmoid((int32_t)(duration_offset+duration-i)/((double)SAMPLE_RATE/100));
			if (p->flag&NFLG_LEFT)  buffer[i * 2] += phase;    /* 左声道 */
			if (p->flag&NFLG_RIGHT) buffer[i*2+1] += phase;    /* 左声道 */
		}
		duration_offset += duration;
		if (!p) continue;
		if (p == *pp) break;
	}
	if (!p) return -4;
	p->duration = sample_num;
	p->pwav = buffer;
	/*if (status&FLG_PRINT) {*/
		/*char buf[30] = "";*/
		/*printf("[%d/%d %dbpm:%d] 1/%-3.2lg %5.1lfHz (%0.4lg%%) [%s] [%p(%d)]\n",*/
		       /*p->notes,p->beates,p->speed,p->track,p->type,*/
		       /*p->freq, p->amplitude*100, i2b(p->flag, 1, buf, 30),*/
		       /*p->pwav, sample_num);*/
	/*}*/
	return sample_num;
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

/* 将所有非0轨道合并到轨道0 */
void merge_tracks(struct Note *tracks[UINT8_MAX], int duration, uint64_t offset)
{
	/* 用ai来修bug还是可以的hhh */
	if (!tracks[0] || !tracks[0]->pwav)
		return;

	if (offset%2 != 0) offset++;
	int16_t *pwav = tracks[0]->pwav;	// 主轨道缓冲区，长度 2*duration

	for (uint8_t i = 1; i < UINT8_MAX; i++) {
		if (!tracks[i])
			continue;
		struct Note *p = tracks[i];
		int len = 0;	// 已处理的样本数（时间）
		for (int j = 0; j < duration; j++) { // 找到覆盖当前时间 offset+j 的音符
			while (p) { // 跳过无音频数据的音符，并累加其持续时间
				if (!p->pwav) {
					len += p->duration;
					p = p->pNext;
					continue;
				}
				if (p->track != i) {
					p = NULL;
					break;
				}
				if (len + p->duration > offset + j) break;	// 目标时间在当前音符内
				// 若当前音符结束位置 <= 目标时间，则移到下一个音符
				len += p->duration;
				p = p->pNext;
			}
			if (!p) break;	// 轨道已无音符，剩余样本不叠加
			int note_offset = (offset + j) - len;	// 在当前音符内的偏移（样本数）
			// 叠加左右声道
			pwav[2 * j] += p->pwav[2 * note_offset];
			pwav[2 * j + 1] += p->pwav[2 * note_offset + 1];
		}
	}
}

/* free space of pwav */
void clean_tracks_wav(struct Note *(*tracks)[UINT8_MAX])
{
	if (!tracks || !*tracks) return;
	struct Note *p2 = NULL;
	for(uint8_t i=1; i < UINT8_MAX; i++) {
		if (!*tracks[i]) continue;
		for(struct Note *p = *tracks[i]; p && p->track == i; p2 = p, p = p->pNext) {
			if (!p2 || !p2->pwav) continue;
			free(p2->pwav);
		}
	}
	return;
}

int melody(int id, char *filename, double amplitude, char *input)
{
	id %= 7;
	const char *note[7] = {
		"C C G G A A G*  F F E E D D C*\n"   /* 一闪一闪亮晶晶 */ /* 满天都是小星星 */
		"G G F F E E D*  G G F F E E D*\n"   /* 挂在天上放光明 */ /* 好像许多小眼睛 */
		"C C G G A A G*  F F E E D D C*",    /* 一闪一闪亮晶晶 */ /* 满天都是小星星 */

		":beates=2;"
		"C/ C/ C   g/ a/ g   E/. D// C/ a/  D*"
		"E/ E/ E   G/ G/ G   A/. G// C/ E/  D*"
		"E*        G*        E/. D// C/ D/  a*"
		"As1       G.sE/     C E            G*"
		"A/ 0/     G/ 0/     E/ 0/          G/ 0/"
		"1.++ A/ G/ 0/ E/ 0/"	/* 打败美帝 */
		"G/ 0/ D/ 0/ C 0"	/* 野心狼 */
		"C/ C/ C   g/ a/ g   E/. D// C/ a/  D*"
		"E/ E/ E   G/ G/ G   A/. G// C/ E/  D*"
		"E*        G*        E/. D// C/ D/  a*"
		"As1       G.sE/     C E            G*"
		"A/ 0/     G/ 0/     E/ 0/          G/ 0/"
		"1.++ A/ G/ 0/ E/ 0/"	/* 打败美帝 */
		"1/ 0/ G/ 0/ 1 0",	/* 野心狼 */

		"C C C C C C C C C C "
		"C C C C C C C C C C",    /* 10s音频测试 */

		"c d e f g a b "
		"C D E F G A B "
		"1 2 3 4 5 6 7",    /* 10.5s音频测试 */

		":beates=2;"
		"3/. 2// 1/ A/   G/ A// G// E/ G/   1/ 1// 1// 1/ 1/"
		"1 C// D// E// F//"    /* 前奏 */
		"G E/. D//   C/ E/ G/ 1/   3 2/. 3//   1*"
		"2 2/. 3//   2/ 1/ B/ A/   G A/. 1//   G*"
		"E/ E// E// D/ E/   G/ E// G// A/ G/   1 2   3*"
		"2/. 1// B/ A/   G/ A// G// E/ G/   1 1/ 1/   1"
		"G/. A//   E   G   1/ 1/ 2/ 1/   G*"    /* 为了五大洲的友谊 */
		"G A/. G//   E G   1/ 1/ 2/. 1//   3*"    /* 为了全人类的理想 */
		"3 3/ 5/   4. 3/   2/ B/ A/ G// G//   3. 2/   1"    /* 为了发扬奥林匹克的精神 */
		"B/ A/   G. E/   G/ G// A// B/ 1/   2*   2"    /* 我们竞技在那运动场 */
		"G/. A//   E   G   1/ 1/ 2/ 1/   G*"    /* 为了五大洲的友谊 */
		"G A/. G//   E G   1/ 1/ 2/. 1//   3*"    /* 为了全人类的理想 */
		"3 3/ 5/   4. 3/   2/ B/ A/ G// G//   3. 2/   1"    /* 为了发扬奥林匹克的精神 */
		"B/ A/   G. E/   G/ A/ B/ 2/   1*"    /* 我们竞技在那运动场 */
		"1 C// D// E// F//"
		/* REPEAT NEXT */
		"G E/. D//   C/ E/ G/ 1/   3 2/. 3//   1*"
		"2 2/. 3//   2/ 1/ B/ A/   G A/. 1//   G*"
		"E/ E// E// D/ E/   G/ E// G// A/ G/   1 2   3*"
		"2/. 1// B/ A/   G/ A// G// E/ G/   1 1/ 1/   1"
		"G/. A//   E   G   1/ 1/ 2/ 1/   G*"    /* 为了五大洲的友谊 */
		"G A/. G//   E G   1/ 1/ 2/. 1//   3*"    /* 为了全人类的理想 */
		"3 3/ 5/   4. 3/   2/ B/ A/ G// G//   3. 2/   1"    /* 为了发扬奥林匹克的精神 */
		"B/ A/   G. E/   G/ G// A// B/ 1/   2*   2"    /* 我们竞技在那运动场 */
		"G/. A//   E   G   1/ 1/ 2/ 1/   G*"    /* 为了五大洲的友谊 */
		"G A/. G//   E G   1/ 1/ 2/. 1//   3*"    /* 为了全人类的理想 */
		"3 3/ 5/   4. 3/   2/ B/ A/ G// G//   3. 2/   1"    /* 为了发扬奥林匹克的精神 */
		"B/ A/   G. E/   G/ A/ B/ 2/   1*"    /* 我们竞技在那运动场 */
		"1   0",    /* 运动员进行曲 */

		":beates=6;"
		":notes=8;"
		":speed=180;"
		"1/ 2/ 3/ 2/ 1/ A/ B/ A./ E// G.~G."
		"1/ 2/ 3/ 2/ 1/ A/ B/ G./ E// A.~A."
		"G/ F/ E/ D. b/ a/ g/ E.  F.  D  C/  C.~C."
		/* 前奏over */
		"G/~A/ G/ F/~E/  D/  C. g."	/* 我和~我~的~祖~国~ */
		"C/ E/ 1/ B/ A./ E// G.~G."	/* 一刻也不能分割 */
		"A/ B/ A/ G/~F/  E/  D. a."	/* 无论我走到哪里 */
		"b/ a/ g/ G/ C./ D// E.~E."	/* 都流出一首赞歌 */
		"G/ A/ G/ F/ E/  D/  C. g."	/* 我歌唱每一座高山 */
		"C/ E/ 1/ B/ 2./ 1// A.~A."	/* 我歌唱每一条河 */
		"1/ B/ A/ G."			/* 袅袅炊烟 */
		"A/ G/ F/ E."			/* 小小村落 */
		"b  a/ g/~g/ D/ C.~C."		/* 路下一道辙 */
		"1/ 2/~3/ 2/ 1/ A/ B/~A./~E// G.~G."	/* 我最亲爱的祖~国~ */
		"1/ 2/ 3/ 2/ 1/ A/ B/ G./ E// A.~A."	/* 我永远紧依着你的心窝 */
		"G/ F/ E/ D."			/* 你用你那 */
		"b/ a/ g/ E."			/* 母亲的脉搏 */
		"F. D  C/ C.~C 0/"		/* 和我诉说 */
		/* 重复上一段 */
		"G/~A/ G/ F/~E/  D/  C. g."	/* 我的~祖~国~和~我~ */
		"C/ E/ 1/ B/ A./ E// G.~G."	/* 像海和浪花一朵 */
		"A/ B/ A/ G/~F/  E/  D. a."	/* 浪是那海的赤子 */
		"b/ a/ g/ G/ C./ D// E.~E."	/* 海是那浪的依托 */
		"G/ A/ G/ F/ E/  D/  C. g."	/* 每当大海在微笑 */
		"C/ E/ 1/ B/ 2./ 1// A.~A."	/* 我就是笑的酒窝 */
		"1/ B/ A/ G."			/* 我分担着 */
		"A/ G/ F/ E."			/* 海的忧愁 */
		"b  a/ g/~g/ D/ C.~C."		/* 分享海的欢乐 */
		"1/ 2/~3/ 2/ 1/ A/ B/~A./~E// G.~G."	/* 我最亲爱的祖~国~ */
		"1/ 2/ 3/ 2/ 1/ A/ B/ G./ E// A.~A."	/* 你是大海永不干涸 */
		"G/ F/ E/ D."			/* 永远给我 */
		"b/ a/ g/ E."			/* 碧浪清波 */
		"F. D  C/ C.~C 0/"		/* 心中的歌 */
		/* 结束句 */
		"1/ 2/ 3/ 2/ 1/ A/ B/~A./~E// G.~G."	/* 我最亲爱的祖~国~ */
		"1/ 2/ 3/ 2/ 1/ A/ B/ G./ E// A.~A."	/* 你是大海永不干涸 */
		"G/ F/ E/ D."			/* 永远给我 */
		"b/ a/ g/ E."			/* 碧浪清波 */
		"G. 2  1/ 1.~1. 0/",		/* 心中的歌 */
		/* 我和我的祖国 */

		":beates=2;"
		"C./ E// G/ G/ A G E./ C// G/ G// G//"
		"E C g/ g// g// g/ g// g// C 0/"
		"g/ C. C/ C./ C// g/ a// b// C C  0/"
		"E/ C/ D// E// G G E./ E// C./ E// G./ E// D D*"
		"A G D E G/ E/ 0/ G/ E/ D// E// C E 0"
		"g./ a// C/ C/ E./ E// G/ G/ D/ D// D// a D."
		"g/  C."
		"C/ E."
		"E/ G*"
		"C./ E// G/ G/ A G"
		"E./ C// G/ G// G// E/ 0/  C/ 0/"
		"g C E./ C// G/ G// G// E/ 0/ C/ 0/"
		"g C g C g C C 0",
		/* 义勇军进行曲 */
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

	struct Note *pH = parse_notes(status&FLG_PLYARG ? input : note[id], amplitude),
		    *p = pH, *p2 = NULL,
		    *tracks[UINT8_MAX] = {NULL};
	uint64_t sizes[UINT8_MAX] = {0};
	int count = 0;
	for (; p ; p = p->pNext) {
		if (!p) break;
		count++;
		p2 = p;
		int duration = create_note_wave(&p);
		if (!p) break;
		if (duration <= 10) {
			fprintf(stderr,"波形为空,Code:[%d], ind:%d\n",
			       duration, count);
			continue;
		}

		if (p->track == 0 && sizes[0] == 0) {
			size+=(double)duration/SAMPLE_RATE;
			if (status & FLG_SAVE) fwrite(p->pwav, sizeof(int), duration, wav_file);
#ifdef ENABLE_ALSA
			if (status & FLG_PLAY) play_wav(pcm_handle, p->pwav, duration);
#endif
			free(p->pwav);
			p->pwav = NULL;
		} else if (p->track == 0) {
			if (sizes[0] == -1) {
				sizes[0] = 0;
				for(uint8_t i=1; i < UINT8_MAX; i++) {
					if(sizes[i]>sizes[0]) sizes[0]=sizes[i];
					sizes[i] = 0;
				}
				sizes[1] = size*SAMPLE_RATE;
			}
			tracks[0] = p;
			merge_tracks(tracks, duration, size*SAMPLE_RATE-sizes[1]);
			size+=(double)duration/SAMPLE_RATE;
			if (status & FLG_SAVE) fwrite(p->pwav, sizeof(int), duration, wav_file);
#ifdef ENABLE_ALSA
			if (status & FLG_PLAY) play_wav(pcm_handle, p->pwav, duration);
#endif
			free(p->pwav);
			p->pwav = NULL;
			if (size*SAMPLE_RATE-sizes[1] > sizes[0]) {
				// exit clean up
				clean_tracks_wav(&tracks);
				sizes[0] = 0;
				sizes[1] = 0;
			}
		} else {
			if (!tracks[p->track]) tracks[p->track] = p;
			sizes[p->track] += duration;
			sizes[0] = -1;
		}
	}
	for (struct Note *p2 = pH; pH;) {
		p2 = pH;
		pH = pH->pNext;
		if (p2->pwav) free(p2->pwav);
		free(p2);
	}
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
	int ch = 0, id = 0;
	double amplitude = 0.2;
	char filename[125] = "output.wav",
	     *notes = NULL;
	while ((ch = getopt(argc, argv, "hi:psnmHo:r:C:PA:")) != -1) {	/* 获取参数 */
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
			       "    -r <FILE> 输入文件（启用后其他选项无效）\n"
			       "    -C <STR>  额外指定音符(用`|`或` `分割)\n"
			       "    -P        打印音符(格式化)\n"
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
			status |= FLG_SMOOTH_PORTAMENTO;
			break;
		case 'H':
			status &= ~FLG_HARMONICS;
			break;
		case 'o':
			strcpy(filename, optarg);
			break;
		case 'A':
			amplitude = strtof(optarg, NULL);
			if (amplitude > 0.9 || amplitude < 0)
				amplitude = 0.2;
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
		default:
			break;
		}
	}
	melody(id, filename, amplitude, notes);
	if (notes)
		free(notes);
	return 0;
}
