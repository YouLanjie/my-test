/**
 * @file        core.h
 * @author      u0_a221
 * @date        2026-05-01
 * @brief       简要描述该文件的作用
 */

#pragma once

#ifndef _CORE_H
#define _CORE_H

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <limits.h>
#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include <math.h>

// #define ENABLE_REVERPHASE

/* 采样率（Hz） */
extern uint32_t SAMPLE_RATE;

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
} WavHeader_t;
// wav文件头生成
WavHeader_t create_wav_header(uint32_t duration);


// 解析器上下文定义
struct StringCtx_t {
	int ch;
	int line;
	int col;
	const char *desc;
};
int str_switch(const char *strlist[], int listlen, const char *str, int *possible, struct StringCtx_t *ctx);
#define str_switch2(str, possible, desc, ...) \
	str_switch((const char*[]){__VA_ARGS__}, \
		   sizeof((const char*[]){__VA_ARGS__})/sizeof(char*),\
		   str, possible, desc)


/*
 * 技巧（From ioccc 2024/straadt）
 * · 底鼓来自正弦波，提高音调以产生通鼓。
 * · Roland TR-808 风格的踩镲（一堆高通滤波的方波）。
 * · SID/C64 风格的小军鼓（在噪声和方波之间振荡）。
 * · 贝斯中使用了 FM 合成。
 * · 施罗德混响器，类似 Freeverb。同样的构建模块也用于回声效果。
 * · 使用 tanh(x) 进行波形塑形/过载，哦也..
 * */

/* :bq=type; 指定 */
enum BiquadType { BQ_NOSET=0, BQ_LP, BQ_HP, BQ_BP, BQ_MAX};
/* 滤波器结构体 */
typedef struct {
	double b0, b1, b2;   // 前向系数（分子）
	double a1, a2;       // 反馈系数（分母，注意差分方程中为 -a1, -a2）
	double x1, x2;       // 前两个输入样本
	double y1, y2;       // 前两个输出样本
	double fc, bw;       // 频率与带宽
	enum BiquadType bq_type;
} Biquad_t;
// 滤波器函数
void biquad_set(Biquad_t *bq, char *key, char *value, struct StringCtx_t ctx);
void biquad_compile(Biquad_t *p);
double biquad_apply(Biquad_t *f, double x);

/* 包络器 */
typedef struct {
	double attack;     /* 起振(秒) */
	double decay;      /* 衰减(秒) */
	double sustain;    /* 持续(保持的声音大小,0~1) */
	double release;    /* 释放(秒) */
} ADSR_t;

/* 泛音列配置 */
typedef struct {
	double freq_times;
	double amplitude;
	double decay_speed;
	// size_t len;
	// double amplitude;
	// double (*decay_func)(size_t ind,double sec);
} Harmonics_t;

/* 存储振荡器产生的PCM数据(单声道f64)
 * 未经过滤波器、ADSR
 * 独立成链表，可被查找 */
typedef struct NoteData {
	double *pwav;
	size_t  sample_num;    /* pwav数组长度 */
	size_t  ref_count;

	/* 获取泛音列函数(返回长度，参数存表，基本本倍率和衰减速度) */
	size_t (*har_func)(const Harmonics_t**);
	double (*wave_func)(double);        /* 波函数 */
	double (*flo_freq_func)(double);    /* LFO包络 */
	double   freq;
	double   type;		/* type分音符 */
	uint16_t speed;		/* 速度，n拍/分钟 */
	uint8_t  notes;		/* 以notes分音符为一拍 */
	uint8_t  beates;	/* 每小节beates拍 */

	struct NoteData *next;
} NoteData_t;
void notedata_setlen(NoteData_t *p);

/* 存储乐谱数据 */
typedef struct Note {
	double      amplitude;    /* 响度修正 */
	NoteData_t *pcm_data;     /* 基声波数据引用 */
	Biquad_t   *biquad;       /* 滤波器 */
	ADSR_t      adsr;         /* ADSR包络 */

	uint8_t track;       /* 声轨 */
	bool flg_left;       /* 左声道 */
	bool flg_right;      /* 右声道 */
	bool flg_legato;     /* 连音 */
	bool flg_be_legato;  /* 被连音 */
	bool flg_portamento; /* 滑音 */
	bool flg_be_portam;  /* 被滑音 */

	int ch;
	int ind;
	int line;

	struct Note *prev;
	struct Note *next;
} Note_t;

#define sigmoid(x) (1 - 1/(1+exp(x)))
#ifndef ARRAY_LEN
#define ARRAY_LEN(arr) (sizeof(arr)/sizeof(arr[0]))
#endif
#define CALLCLS(obj, func, ...) (obj)->func((obj) __VA_OPT__(,) __VA_ARGS__)

#define LOG(fmt, ...) fprintf(stderr, "%s:%d:%s: " fmt "\n", __FILE__, __LINE__, __FUNCTION__ __VA_OPT__(,) __VA_ARGS__)
#define LOG2(fmt, ...) fprintf(stderr, "%s:%d: " fmt "\n", __FILE__, __LINE__ __VA_OPT__(,) __VA_ARGS__)
#define LOGVAR(typ, var) LOG("var '%s' = " typ " (as %s)", #var, (var), #typ)
#define LOGVAR2(typ, var) LOG2("var '%s' = " typ " (as %s)", #var, (var), #typ)
// #define CALL(func, ...) (LOG("call '%s' at (%p)", #func, (func)), (func)(__VA_ARGS__))


#define HARMONICS_LIST    \
	HARMONIC(piano1)  \
	HARMONIC(piano2)  \
	HARMONIC(piano3)  \
	HARMONIC(piano4)  \
	HARMONIC(none)    \
	HARMONIC(openhat) \
	HARMONIC(h3)      \
	HARMONIC(h4)      \
	HARMONIC(h5)
/* 泛音列表 */
#define HARMONIC(x) size_t har_set_##x(const Harmonics_t ** harp);
HARMONICS_LIST
#undef HARMONIC

// 低频信号函数集
double flo_inverse_liner(double t);
double flo_rand(double t);

// 声波函数集
double wave_square(double t);
double wave_triangle(double t);
double wave_sawtooth(double t);
double wave_noise(double t);

// txt曲谱解释函数
void print_note(Note_t *p);
void check_notes(Note_t *p, bool print);
void note_free(Note_t *p);
Note_t *note_parser(int (*stream)(void*), void *stream_ctx);
Note_t *note_search_last(Note_t *pH, Note_t *obj);

// 声音生成处理
void note_gen_wave(NoteData_t *p, bool no_har);
double adsr_get_envelope(ADSR_t *adsr,int current, int total);
double get_portamento_freq(double start, double end, double x, bool smooth);

typedef struct Music_t {
	Note_t *notes;      /* 音符列指针头 */
	size_t position;    /* 位置（采样点精度）(能指示的最大时间超过10万年所以无所畏惧) */
	size_t track_position;  /* 轨道分支位置|音符起始位置 */
	Note_t *tracks[INT8_MAX];  /* 各轨道指针 */
	size_t buffer_len;
	double *buffer;
	double amplitude;

	bool flg_no_fade;
	bool flg_smooth;
	bool flg_no_har;

	int16_t *pcmf16_buffer;
	void   (*pcm_handle)(int16_t*,int);
} MusicCtx_t;
void music_ctx_free(MusicCtx_t *ctx);
MusicCtx_t *music_ctx_create(size_t bufflen);
bool music_ctx_tracks_reset(MusicCtx_t *ctx);
size_t music_ctx_gen_pcm(MusicCtx_t *ctx);
void music_ctx_pcm_to_i16(MusicCtx_t *ctx);
size_t music_ctx_stat(MusicCtx_t *ctx);

#endif //CORE_H
