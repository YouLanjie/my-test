/**
 * @file        core.c
 * @author      u0_a221
 * @date        2026-05-01
 * @brief       声音合成器核心件(拆分自原ALSA.c)
 *              文件头、声音合成、滤波器、包络器
 */

#include "../core.h"
#include <math.h>
/* #ifndef DISABLE_OMP
#include <omp.h>
#endif */

uint32_t SAMPLE_RATE = 44100;

WavHeader_t create_wav_header(uint32_t duration)
{
	WavHeader_t header = {0};
	memcpy(header.RIFF, "RIFF", 4);
	memcpy(header.WAVE, "WAVE", 4);
	memcpy(header.fmt, "fmt ", 4);
	memcpy(header.data, "data", 4);

	header.fmt_size = 16;
	header.audio_format = 1;	// PCM
	header.channels = 2;
	header.sample_rate = SAMPLE_RATE;
	header.bits_per_sample = 16;
	header.block_align = header.channels * header.bits_per_sample / 8;
	header.byte_rate = SAMPLE_RATE * header.block_align;

	int data_size = duration * header.block_align;
	header.data_size = data_size;
	header.file_size = data_size + sizeof(WavHeader_t) - 8;
	/*printf("DATA_SIZE:%d\nFILE_SIZE:%d\nDELTER:%d\n",*/
	 /*data_size,header->file_size,header->file_size-data_size);*/
	return header;
}

/* 创建并初始化双二阶滤波器
 * p    : 滤波器结构体指针
 * type : 滤波器类型（低通/高通/带通）
 * fc   : 中心频率/截止频率 (Hz)
 * bw   : 带宽 (Hz)，仅对带通有效，其他类型可传0使用默认Q
 */
void biquad_compile(Biquad_t *p)
{
	if (!p) return;
	if (!p->fc) {
		static const double def_f[BQ_MAX] = {0, 150, 8000, 4000};
		p->fc = def_f[p->bq_type];
	}
	double w0 = 2 * M_PI * p->fc / SAMPLE_RATE;
	double cosw0 = cos(w0);
	double sinw0 = sin(w0);
	double Q = (p->bq_type == BQ_BP && p->bw > 0) ? p->fc / p->bw : 0.707;	// Butterworth Q
	double alpha = sinw0 / (2 * Q);

	double a0 = 1 + alpha;
	double a1_norm = -2 * cosw0 / a0;	// 归一化后的 a1'
	double a2_norm = (1 - alpha) / a0;	// 归一化后的 a2'

	double b0, b1, b2;

	switch (p->bq_type) {
	case BQ_LP:		// 低通
		b0 = (1 - cosw0) / 2;
		b1 = 1 - cosw0;
		b2 = b0;
		break;
	case BQ_HP:		// 高通
		b0 = (1 + cosw0) / 2;
		b1 = -(1 + cosw0);
		b2 = b0;
		break;
	case BQ_BP:		// 带通（常增益峰值）
		b0 = alpha;
		b1 = 0;
		b2 = -alpha;
		break;
	default:
		*p = (Biquad_t){};
		return;		// 未知类型
	}

	// 归一化系数
	*p = (Biquad_t){
		.bq_type = p->bq_type,
		.b0 = b0 / a0,
		.b1 = b1 / a0,
		.b2 = b2 / a0,
		.a1 = a1_norm,
		.a2 = a2_norm,
		// x1, x2, y1, y2 自动初始化为 0
	};
}

void biquad_set(Biquad_t *bq, char *key, char *value)
{
	if (!bq || !value) return;
	if (!key || !*key) {
		if (strcmp(value, "low-pass") == 0) bq->bq_type = BQ_LP;
		else if (strcmp(value, "high-pass") == 0) bq->bq_type = BQ_HP;
		else if (strcmp(value, "band-pass") == 0) bq->bq_type = BQ_BP;
		else {
			bq->bq_type = BQ_NOSET;
			LOG("不可用的滤波器：%s", value);
		}
		return;
	}
	if (bq->bq_type == BQ_NOSET) return;

	if (strcmp(key, "freq") == 0) {
		bq->fc = atof(value);
	} else if (strcmp(key, "bw") == 0) {
		bq->bw = atof(value);
	} else return;
	biquad_compile(bq);
	return;
}

/* 应用滤波器
 * x：相位 */
double biquad_apply(Biquad_t *f, double x)
{
	if (!f) return x;
	double y = f->b0*x + f->b1*f->x1 + f->b2*f->x2 \
		   - f->a1*f->y1 - f->a2*f->y2;
	// 更新延迟单元
	f->x2 = f->x1;
	f->x1 = x;
	f->y2 = f->y1;
	f->y1 = y;
	return y;
}

/*
 * 获取应有频率
 * start: 初始频率
 * end: 目标频率
 * x: 0-1
 * */
double get_portamento_freq(double start, double end, double x, bool smooth)
{
	if (smooth) {
		/* 对数和指数计算让音调“均匀”变化 */
		/* f: e^{lnA+(lnB-lnA)*x} == A^{1-x} * B^{x}
		 * f->\int f: (A^{1-x} * B^{x})/ln(B/A)
		 * -> S f(x)/x */
		return pow(start, 1-x) * pow(end, x) / log(end/start) / x;
	}
	/* x=0.5附近从start跃变为end (provide by ai) */
	double k = 20;
	return start + log(1+exp(k*(x-0.5))) * (end-start)/(k*x);
}


#define note_sample_len_get(p) \
	(SAMPLE_RATE*((double)p->notes/p->type)*((double)60/p->speed))

void notedata_setlen(NoteData_t *p)
{
	if (!p) return;
	while (p) {
		p->sample_num = note_sample_len_get(p);
		p = p->pNext;
	}
}

/* 
 * 生成固定声波并填充到给定的 NoteData_t
 * 数学模型: f(x) = A*sin(ω*x)
 * x: 时间 (秒)
 * f: 基频频率 (Hz)
 */
void note_gen_wave(NoteData_t *p)
{
	if (!p || p->freq < 0 || p->pwav || !p->wave_func) return;
	// 各乐器前n次谐波幅度（包含基频）
	const Harmonics_t *har;
	size_t n = 0;
	if (p->har_func) n = p->har_func(&har);
	if (n == 0) {
		n = 1;
		har = (Harmonics_t[]){{1, 1, 0}};
	}
	p->sample_num = note_sample_len_get(p);
	double *buffer = malloc(sizeof(*buffer)*p->sample_num);
	if (!buffer) return;
	memset(buffer, 0, sizeof(*buffer)*p->sample_num);

	double f = 0, sec = 0;
	size_t i = 0, j = 0;
// #ifndef DISABLE_OMP
// /* 需编译和链接时使用 -fopenmp 编译参数 */
// #pragma omp parallel for
// #endif
	for (; i < p->sample_num; i++) {    /* 生成每个采样点声波的相位 */
		sec = (double)i/SAMPLE_RATE;
		f = p->freq;
		if (p->flo_freq_func) f *= p->flo_freq_func(sec);
// #ifndef DISABLE_OMP
// /* 需使用 -fopenmp 编译参数 */
// #pragma omp simd reduction(+:buffer[i])
// #endif
		for (j = 0; j < n; j++) {
			buffer[i] +=
				har[j].amplitude * p->wave_func(M_PI*2 * har[j].freq_times * f * sec) *
				exp((-1-har[j].decay_speed) * sec);
		}
	}
	p->pwav = buffer;
	return;
}

double adsr_get_envelope(ADSR_t *adsr,int current, int total)
{
	if (current < 0 || current > total)
		return 0;
	if (!adsr) return 1;
	double t = (double)current / total;
	if (t < adsr->attack)
		return t /adsr-> attack;	// 起振（线性增长）
	else if (t < adsr->attack + adsr->decay)
		return 1.0 - (1.0 - adsr->sustain) * (t - adsr->attack) / adsr->decay;	// 衰减
	else if (t < 1.0 - adsr->release)
		return adsr->sustain;	// 持续
	else
		return adsr->sustain * (1.0 - (t - (1.0 - adsr->release)) / adsr->release);	// 释放
}

