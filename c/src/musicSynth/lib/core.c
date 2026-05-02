/**
 * @file        core.c
 * @author      u0_a221
 * @date        2026-05-01
 * @brief       声音合成器核心件(拆分自原ALSA.c)
 *              文件头、声音合成、滤波器、包络器
 */

#include "../core.h"

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
void create_biquad(Biquad *p, enum BiquadType type, double fc, double bw)
{
	if (!p) return;
	if (!fc) {
		static const double def_f[BQ_MAX] = {0, 150, 8000, 4000};
		fc = def_f[type];
	}
	double w0 = 2 * M_PI * fc / SAMPLE_RATE;
	double cosw0 = cos(w0);
	double sinw0 = sin(w0);
	double Q = (type == BQ_BP && bw > 0) ? fc / bw : 0.707;	// Butterworth Q
	double alpha = sinw0 / (2 * Q);

	double a0 = 1 + alpha;
	double a1_norm = -2 * cosw0 / a0;	// 归一化后的 a1'
	double a2_norm = (1 - alpha) / a0;	// 归一化后的 a2'

	double b0, b1, b2;

	switch (type) {
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
		*p = (Biquad){};
		return;		// 未知类型
	}

	// 归一化系数
	*p = (Biquad){
		.b0 = b0 / a0,
		.b1 = b1 / a0,
		.b2 = b2 / a0,
		.a1 = a1_norm,
		.a2 = a2_norm,
		// x1, x2, y1, y2 自动初始化为 0
	};
}

/* 应用滤波器
 * x：相位 */
double apply_biquad(Biquad *f, double x)
{
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
 * 生成固定声波
 * volume: 0% ~ 100% (通常取 0.0 ~ 1.0)
 * 数学模型: f(x) = A*sin(ω*x)
 * x: 时间 (秒)
 * f: 基频频率 (Hz)
 * 返回值: 16位线性PCM样本值（已乘以 INT16_MAX 和 volume）
 */
double gen_wave(double x, double volume, double f, Note_t *p, bool no_har)
{
	// 各乐器前15次谐波幅度（包含基频）
	// 指针数组，方便通过枚举索引获取对应数组
	static const double har_amps[HAR_MAX][15] = {
		{1,0.340,0.102,0.085,0.070,0.065,0.028,0.085,
			0.011,0.030,0.010,0.014,0.012,0.013,0.004},
		{1,0,0,0,0,0,0,0,0,0,0,0,0,0,0},
		{1.000, 0.100, 0.800, 0.100, 0.600, 0.100, 0.400, 0.100,
			0.200, 0.050, 0.100, 0.020, 0.050, 0.010, 0.020},
		{1,0.8,0.1,0.6,0.1,0.4,0.1,0.2,0.05,0.1,0.02,0.05,0.01,0.02,0},
		{1,0.9,0.9,0.8,0.8,0.7,0.7,0.2,0.2,0.1,0.1,0.05,0.05,0.01,0.01},
	};
	static double (*wave_funcs[WF_MAX])(double) = {
		sin, square_wave, triangle_wave, sawtooth_wave,
		noise_wave,
	};

	if (f < 0) return 0;

	int8_t instrument = p->instrument;
	int8_t wave_func = p->wave_func;
	int8_t harmonics = p->harmonics;
	double value = 0;
	switch (instrument) {
	case INST_SIN:
		if (harmonics == HAR_NOSET) harmonics = HAR_PIANO;
		break;
	case INST_DRUM:
		f = f / (10*x + 1);
		if (harmonics == HAR_NOSET) harmonics = HAR_NONE;
		break;
	case INST_HIHAT:
		/* 未完成 */
		wave_func = WF_SQUARE;
		if (harmonics == HAR_NOSET) harmonics = HAR_NONE;
		if (!p->bq) p->bq = BQ_HP;
		double (*func)(double) = wave_funcs[wave_func];
		double f2 = f;
		for (int i = 1; i < 8; ++i) {
			value += har_amps[HAR_PIANO][i] * func(2 * M_PI * f2 * x);
			f2 += rand() % 50;
		}
		value = tanh(value);
		break;
	}
	if (harmonics == HAR_NOSET) harmonics = HAR_PIANO;
	instrument %= INST_MAX;
	wave_func %= WF_MAX;
	harmonics %= HAR_MAX;

	const double *harmonicsp = har_amps[harmonics];
	value += harmonicsp[0] * wave_funcs[wave_func](2 * M_PI * f * x);
	// 基频 + 14个泛音
	// 44100 / 2 = 22050 (No Check)
	if (no_har)    // 若禁止泛音，只使用基频
		return INT16_MAX * volume * value;
	double (*func)(double) = wave_funcs[wave_func];
#ifndef DISABLE_OMP
/* 需使用 -fopenmp 编译参数 */
#pragma omp simd reduction(+:value)
#endif
	for (int i = 1; i < 15; i++) {
		value += harmonicsp[i] * func(2 * M_PI * (i + 1) * f * x);
	}
	return INT16_MAX * volume * value;
}

double adsr_envelope(int current, int total, bool no_fade)
{
	if (current < 0 || current > total)
		return 0;
	if (no_fade) return 1.0;
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


#define pduration(p) (SAMPLE_RATE*((double)p->notes/p->type)*((double)60/p->speed))
/*
 * 解读字符串形式的音符并产生对应声波
 * RET: duration (单位：1/SAMPLE_RATE s)
 * memsize = duration*sizeof(uint16_t)*2
 *              时长    notes   双声道
 * */
int create_note_wave(Note_t **pp, bool no_fade, bool smooth, bool no_har)
{
	if (!pp || !*pp) return -1;
	Note_t *p = *pp;
	uint32_t sample_num = pduration(p);
	if (p->flag&(NFLG_LEGATO|NFLG_PORTAMENTO)) {
		uint8_t flag = p->flag&(NFLG_LEGATO|NFLG_PORTAMENTO);
		sample_num = 0;
		for (Note_t *p2 = p;
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

	/* 缓冲区大小，同时控制声音片段大小
	 * 缓冲区越大声音持续越久，也越慢 */
	if (sample_num == 0) return -2;
	int16_t *buffer = malloc(sizeof(int16_t)*(sample_num * 2));    /* 双声道 */
	if (!buffer) return -3;
	memset(buffer, 0, sizeof(uint16_t)*(sample_num * 2));
	int flag_slide = 0;    /* 启用滑音时的初始频率 */
	int duration_offset = 0;
	for (; p ; p = p->pNext) {
		if (p->flag&NFLG_PORTAMENTO) {    /* 滑音 != 连音 */
			flag_slide = p->freq;
			continue;
		}
		uint32_t duration = pduration(p);
#ifndef DISABLE_OMP
/* 需编译和链接时使用 -fopenmp 编译参数 */
#pragma omp parallel for
#endif
		for (int i = 0; i < sample_num; i++) {    /* 生成每个采样点声波的相位 */
			double env = adsr_envelope(i, sample_num, no_fade)*p->amplitude;
			double freq = flag_slide ? get_portamento_freq(flag_slide, p->freq, (double)i/sample_num, smooth) : p->freq;
			int16_t phase = (int16_t)gen_wave((double)i/SAMPLE_RATE, env, freq, p, no_har);
			if (p->flag&NFLG_BE_LEGATO)    /* fade in */
				phase *= sigmoid((int32_t)(i-duration_offset)/((double)SAMPLE_RATE/100));
			if (p->flag&NFLG_LEGATO)    /* fade out */
				phase *= sigmoid((int32_t)(duration_offset+duration-i)/((double)SAMPLE_RATE/100));
			if (p->flag&NFLG_LEFT)  buffer[i * 2] += phase;    /* 左声道 */
#ifdef ENABLE_REVERPHASE
			if (p->flag&NFLG_RIGHT) buffer[i*2+1] -= phase;    /* 右声道 */
#else
			if (p->flag&NFLG_RIGHT) buffer[i*2+1] += phase;    /* 右声道 */
#endif
		}
		if (p->bq) {
			Biquad bq_l = {}, bq_r = {};
			create_biquad(&bq_l, p->bq, p->bq_freq, 0);
			create_biquad(&bq_r, p->bq, p->bq_freq, 0);
			for (int i = 0; i < sample_num; i++) {
				buffer[i * 2] = apply_biquad(&bq_l, buffer[i * 2]);
				buffer[i*2+1] = apply_biquad(&bq_r, buffer[i*2+1]);
			}
		}
		duration_offset += duration;
		if (!p) continue;
		if (p == *pp) break;
	}
	if (!p) {
		free(buffer);
		return -4;
	}
	p->duration = sample_num;
	p->pwav = buffer;
	/*printf("[%d/%d %dbpm:%d] 1/%-3.2lg %5.1lfHz (%0.4lg%%) [%s] [%p(%d)]\n",*/
	       /*p->notes,p->beates,p->speed,p->track,p->type,*/
	       /*p->freq, p->amplitude*100, i2b(p->flag, 1, buf, 30),*/
	       /*p->pwav, sample_num);*/
	return sample_num;
}
