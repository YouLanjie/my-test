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


/*
 * 技巧（From ioccc 2024/straadt）
 * · 底鼓来自正弦波，提高音调以产生通鼓。
 * · Roland TR-808 风格的踩镲（一堆高通滤波的方波）。
 * · SID/C64 风格的小军鼓（在噪声和方波之间振荡）。
 * · 贝斯中使用了 FM 合成。
 * · 施罗德混响器，类似 Freeverb。同样的构建模块也用于回声效果。
 * · 使用 tanh(x) 进行波形塑形/过载，哦也..
 * */

/* 滤波器结构体 */
typedef struct {
	double b0, b1, b2;   // 前向系数（分子）
	double a1, a2;       // 反馈系数（分母，注意差分方程中为 -a1, -a2）
	double x1, x2;       // 前两个输入样本
	double y1, y2;       // 前两个输出样本
} Biquad;
enum BiquadType { BQ_LP=1, BQ_HP, BQ_BP, BQ_MAX};    /* :bq=num; 指定 */


/* 音符数据 */
typedef struct Note {
	char ch;
	int  ind;
	int  line;
	double freq;
	double amplitude;
	double   type;		/* type分音符 */
	uint8_t  notes;		/* 以notes分音符为一拍 */
	uint8_t  beates;	/* 每小节beates拍 */
	uint16_t speed;		/* 速度，n拍/分钟 */
	uint64_t duration;
	uint8_t  track;
	uint8_t  flag;		/* NFLG_* flags */
	int8_t  instrument;
	int8_t  wave_func;
	int8_t  bq;
	double   bq_freq;
	int8_t   harmonics;
	int16_t *pwav;
	struct Note *pNext;
} Note_t;
// (Note_t).flag 对应值含义
#define NFLG_LEFT (1 << 0)    /* 左声道 */
#define NFLG_RIGHT (1 << 1)    /* 右声道 */
#define NFLG_LEGATO (1 << 2)    /* 连音 */
#define NFLG_BE_LEGATO (1 << 3)    /* 被连音 */
#define NFLG_PORTAMENTO (1 << 4)    /* 滑音 */

enum Instruments {
	INST_SIN, INST_DRUM, INST_HIHAT, INST_MAX
};
enum Harmonics {
	HAR_PIANO, HAR_NONE,
	HAR_3, HAR_4, HAR_5,
	HAR_MAX, HAR_NOSET=-1
};
enum WaveFuncs {
	WF_SIN, WF_SQUARE,
	WF_TRIANGLE,
	WF_SAWTOOTH, WF_NOISE,
	WF_MAX
};

#define sigmoid(x) (1 - 1/(1+exp(x)))

// 声波函数集
double square_wave(double t);
double triangle_wave(double t);
double sawtooth_wave(double t);
double noise_wave(double t);

// txt曲谱解释函数
void print_note(Note_t *p);
void check_notes(Note_t *p, bool print);
Note_t *parse_notes(const char *str, FILE *fp, double base_amplitude);
void free_notes(Note_t *p);

// 滤波器函数
void create_biquad(Biquad *p, enum BiquadType type, double fc, double bw);
double apply_biquad(Biquad *f, double x);

// 声音生成处理
double gen_wave(double x, double volume, double f, Note_t *p, bool no_har);
double adsr_envelope(int current, int total, bool no_fade);
double get_portamento_freq(double start, double end, double x, bool smooth);
int create_note_wave(Note_t **pp, bool no_fade, bool smooth, bool no_har);

// 旋律整体处理、轨道处理
struct Melody_opts_t {
	bool no_fade;
	bool smooth;
	bool no_har;
	bool print_formated_note;
	bool print_debug_info;
};
void merge_tracks(Note_t *tracks[UINT8_MAX], int duration, uint64_t offset, bool print_merge_info);
void clean_tracks_wav(Note_t *(*tracks)[UINT8_MAX], bool print);
uint64_t melody_0(Note_t *pH, void (*pcm_handle)(int16_t*,int), struct Melody_opts_t opts);
#define melody(pH,pcm_handle,...) melody_0(pH, pcm_handle, (struct Melody_opts_t){__VA_ARGS__})

// wav文件头生成
WavHeader_t create_wav_header(uint32_t duration);

#endif //CORE_H
