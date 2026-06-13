/**
 * @file        music_ctx.c
 * @author      u0_a221
 * @date        2026-05-02
 * @brief       旋律相关（旋律处理，合并轨道）
 */

#include "../core.h"

void music_ctx_free(MusicCtx_t *ctx)
{
	if (!ctx) return;
	if (ctx->buffer) free(ctx->buffer);
	if (ctx->pcmf16_buffer) free(ctx->pcmf16_buffer);
	if (ctx->notes) note_free(ctx->notes);
	ctx->buffer = NULL;
	ctx->pcmf16_buffer = NULL;
	ctx->notes = NULL;
	free(ctx);
}

MusicCtx_t *music_ctx_create(size_t bufflen)
{
	if (bufflen <= 0) return NULL;
	MusicCtx_t *p = malloc(sizeof(*p));
	if (!p) return NULL;
	*p = (MusicCtx_t){
		.notes = NULL,
		.position = 0,
		.track_position = 0,
		.tracks = {NULL},
		.buffer_len = bufflen,
		.buffer = malloc(sizeof(*p->buffer)*bufflen*2),
		.pcmf16_buffer = malloc(sizeof(*p->pcmf16_buffer)*bufflen*2),
		.pcm_handle = NULL,
		.amplitude = 1.0,
	};
	if (!p->buffer || !p->pcmf16_buffer) {
		music_ctx_free(p);
		return NULL;
	}
	return p;
}

/* 打印曲谱状态，返回预计长度 */
size_t music_ctx_stat(MusicCtx_t *ctx)
{
	if (!ctx) return 0;
	size_t total_len = 0;
	size_t count1 = 0, count2 = 0, count3 = 0;
	Note_t *p = ctx->notes;
	if (!p) return 0;
	while (p) {
		count1++;
		if (p->track == 0 && p->pcm_data) count3 += p->pcm_data->sample_num;
		p = p->next;
	}
	NoteData_t *nd = ctx->notes->pcm_data;
	while (nd) {
		count2++;
		nd = nd->next;
	}
	total_len = count3;
	printf("[INFO] 全谱共计%lu个音符，不同的音符有%lu个\n", count1, count2);
	printf("[INFO] 音符复用率大约为%.2lf%%\n", (1-(double)count2/count1)*100);
	printf("[INFO] 曲谱预计时长为%.2lfs\n", (double)count3/SAMPLE_RATE);
	return total_len;
}

/* 更新ctx轨道指针(重新遍历链表) */
bool music_ctx_tracks_reset(MusicCtx_t *ctx)
{
	if (!ctx || !ctx->notes) return false;
	size_t position = 0,
	       max_len = 0,
	       track_sizes[ARRAY_LEN(ctx->tracks)] = {0};
	Note_t *p = ctx->notes;
	uint8_t track = 0;
	memset(ctx->tracks, 0, sizeof(ctx->tracks));
	while (p && p->pcm_data) {
		if (track != 0 && p->track == 0) {    /* 切出 */
			ctx->tracks[0] = p;
			for (size_t i = 1; i < ARRAY_LEN(track_sizes); i++)
				if(track_sizes[i]>max_len) max_len=track_sizes[i];
			memset(track_sizes, 0, sizeof(track_sizes));
			/* 保存分轨初始位置
			 * 因为计算其他轨道时主轨道长度未被计算 */
			ctx->track_position = position;
			track = 0;
		} else if (p->track != track) {    /* 其余轨道切换(包括切入) */
			if (p->track >= 0 || p->track < ARRAY_LEN(track_sizes))
				ctx->tracks[p->track] = p;
			track = p->track;
		} else if (p->track >= 0 || p->track < ARRAY_LEN(track_sizes))   /* 累加轨道长度 */
			track_sizes[p->track] += p->pcm_data->sample_num;

		if (p->track == 0) position += p->pcm_data->sample_num;
		if (position > ctx->position) {
			position -= p->pcm_data->sample_num;
			break;
		}
		if (max_len && position-ctx->track_position >= max_len) {
			ctx->track_position = 0;
			max_len = 0;
			memset(ctx->tracks, 0, sizeof(ctx->tracks));
		}
		p = p->next;
	}
	if (!p) return false;
	if (!ctx->tracks[0]) {
		ctx->tracks[0] = p;
		ctx->track_position = position;
	}
	return true;
}

/* 将单个Note_t的数据转入buffer
 * i: ctx->buffer的索引位置
 * idx_from: 当前读取音符缓存数据(pcm_data->pwav)的索引 */
static bool music_ctx_note2buf(MusicCtx_t *ctx, Note_t *p, size_t *i, size_t idx_from)
{
	if (!ctx || !p || !i) return false;
	if (*i >= ctx->buffer_len) return false;
	// if (*i+offset >= p->pcm_data->sample_num) return 0;

	double value = 0;
	for (; *i < ctx->buffer_len && idx_from < p->pcm_data->sample_num; (*i)++, idx_from++) {
		if (!p->pcm_data->pwav) note_gen_wave(p->pcm_data, ctx->flg_no_har);
		if (!p->pcm_data->pwav) {
			*i += p->pcm_data->sample_num - idx_from;
			if (*i > ctx->buffer_len) *i = ctx->buffer_len;
			return true;
		}
		value = p->pcm_data->pwav[idx_from];
		if (!ctx->flg_no_fade) value *= adsr_get_envelope(&p->adsr, idx_from, p->pcm_data->sample_num);
		if (p->biquad) value = biquad_apply(p->biquad, value);
		value *= p->amplitude;
		if (p->flg_left)  ctx->buffer[*i*2]   += value;
		if (p->flg_right) ctx->buffer[*i*2+1] += value;
	}
	return true;
}

static bool music_ctx_group_synth(MusicCtx_t *ctx, Note_t *p, size_t *i, size_t idx_from)
{
	if (!ctx || !p || !p->pcm_data || !i) return false;
	if (*i >= ctx->buffer_len) return false;
	if (!p->flg_portamento && !p->flg_be_portam && !p->flg_legato && !p->flg_be_legato) return false;
	if ((p->flg_portamento||p->flg_legato)&&(!p->next||!p->next->pcm_data)) return false;
	if ((p->flg_be_portam||p->flg_be_legato)&&(!p->prev||!p->prev->pcm_data)) return false;

	/* 统计片段总时长 */
	size_t offset_portam = (p->flg_be_portam && p->prev && p->prev->pcm_data) ? p->prev->pcm_data->sample_num : 0,
	       total_portam_len = p->pcm_data->sample_num + \
				  ((p->flg_portamento && p->next && p->next->pcm_data) ? p->next->pcm_data->sample_num : offset_portam),
	       offset_played = 0,
	       total_legato_len = 0;
	for (Note_t *note = p; note && note->pcm_data; note = note->next) {
		if (note->track != p->track) break;
		if (!(note->flg_be_portam || note->flg_portamento || note->flg_be_legato || note->flg_legato)) break;
		if (note != p && note->flg_legato && !note->flg_be_legato) break;
		total_legato_len += note->pcm_data->sample_num;
	}
	for (Note_t *note = p->prev; (p->flg_be_legato||p->flg_be_portam) && note && note->pcm_data; note = note->prev) {
		if (note->track != p->track) break;
		if (!(note->flg_be_portam || note->flg_portamento || note->flg_be_legato || note->flg_legato)) break;
		total_legato_len += note->pcm_data->sample_num;
		offset_played += note->pcm_data->sample_num;
		if (note->flg_legato && !note->flg_be_legato) break;
	}

	double value = 0;
	double t = 0, f = 0;
	size_t idx = 0, main_idx = 0;
	Note_t *current_note = p;

	enum MODE_t {MODE_PREV, MODE_NOW, MODE_NEXT, MODE_END};
	for (enum MODE_t mode = MODE_PREV; mode < MODE_END; mode++) {
		if (mode == MODE_PREV) {
			// 排除被滑音、非被连音
			if (!p->flg_be_legato||p->flg_be_portam||!p->prev||p->prev->track!=p->track) continue;
			// 切到上一节点
			p = p->prev;
		}
		if (!p || !p->pcm_data) break;
		if (p->track != current_note->track) break;
		if (!(p->flg_be_portam || p->flg_portamento || p->flg_be_legato || p->flg_legato)) break;
		if (mode == MODE_NEXT && p->flg_legato && !p->flg_be_legato) break;

		if ((p->flg_portamento||p->flg_legato)&&(!p->next||!p->next->pcm_data)) break;
		if ((p->flg_be_portam||p->flg_be_legato)&&(!p->prev||!p->prev->pcm_data)) break;

		// 配置泛音列
		const Harmonics_t *har = NULL;
		size_t n = 0, j = 0, origin_idx_from = idx_from;
		if (p->pcm_data->har_func) n = p->pcm_data->har_func(&har);
		if (n == 0 || ctx->flg_no_har || !har) {
			n = 1;
			har = (Harmonics_t[]){{1, 1, 0}};
		}

		idx = *i;
		// if (mode == MODE_PREV) idx += p->pcm_data->sample_num*2/3;

		double f1 = p->flg_portamento ? p->pcm_data->freq       : (p->flg_be_portam ? p->prev->pcm_data->freq : 0),
		       f2 = p->flg_portamento ? p->next->pcm_data->freq : (p->flg_be_portam ? p->pcm_data->freq       : 0);
		for (; idx < ctx->buffer_len && idx_from < current_note->pcm_data->sample_num; idx++, idx_from++) {
			if (!p || !p->pcm_data) break;  // 奇怪的边界检查要求
			t = (double)(idx_from+offset_played) / SAMPLE_RATE;    // 当前时间（秒）
			f = p->flg_portamento||p->flg_be_portam ? \
			    get_portamento_freq(f1, f2, (double)(idx_from+offset_portam)/total_portam_len, ctx->flg_smooth) : \
			    p->pcm_data->freq;
			for (j = 0, value = 0; j < n; j++) {
				value += har[j].amplitude * p->pcm_data->wave_func(M_PI*2 * har[j].freq_times * f * t) *
					exp(-har[j].decay_speed * t);
			}

			// 应用 ADSR 包络
			if (!ctx->flg_no_fade)
				value *= adsr_get_envelope(&p->adsr, idx_from+offset_played, total_legato_len);

			// 可选：连音音量渐变（fade in/out）
			if (p->flg_be_legato && p->prev && p->prev->pcm_data && mode != MODE_PREV) {      // 被连音音符，起始渐入
				value *= sigmoid((int32_t)(mode == MODE_NOW ? idx_from: idx_from - p->prev->pcm_data->sample_num) / (SAMPLE_RATE / 100.0));
			}
			if (p->flg_legato && p && p->pcm_data && mode != MODE_NEXT) {         // 连音音符，尾部渐出
				value *= sigmoid((int32_t)(mode == MODE_NOW ? p->pcm_data->sample_num - idx_from: -idx_from) / (SAMPLE_RATE / 100.0));
			}

			// 应用滤波器
			if (p->biquad) value = biquad_apply(p->biquad, value);

			// 幅度修正
			value *= p->amplitude;

			// 累加到输出缓冲区（立体声）
			if (p->flg_left)  ctx->buffer[(idx) * 2]     += value;
			if (p->flg_right) ctx->buffer[(idx) * 2 + 1] += value;
		}
		if (mode == MODE_NOW) main_idx = idx;

		idx_from = origin_idx_from;
		p = p->next;
		if (p && p->flg_be_portam) p = p->next;
	}
	*i = main_idx;
	return true;
}

// 变量含义图示说明：
// (`/`表轨道,`|`表分割,上面是代数关系示意,下面是位置示意)
//
// p0: track_position
// p1: position
// o1: offset_played || sum(len)
//
//       | <--------(p1-p0+i)---------> |
//       | <------o1------> | <--idx--> |
// TRACK /------/---+-------/-----------+---/------/------/------/
//       p0        p1 <--------i------> |
// BUFF             /-------------------+-----------------/
//
//
// 并轨后：
// p0: (old) track_position
// p1: (old) position
// p2: (new) track_position
// p3: (new) position
// o2: (new) offset_played || sum(len) [IN TRACK 1]
// o3: offset_refit = p3 - p1
//
//                   | <--------(p3-p2+i)----------> |
//                   | <---o3---> | <-o2-> | <-idx-> |
// TRACK 1                        /--------/---------+--/---------------/
// TRACK 0 /---/-----+------------/--------------/---+-----------/------/
//         p0       p1 <----------+---i------------> |
// BUFF             /-------------+------------------+-------/
//                                ^
//                              p2|p3
//
//
// 二次操作后（同图一）：
// o4: (neo) offset_played || sum(len)
//
//              | <-----------------(p3-p2+i)--------------------> |
//              | <-------------o4------------------> | <---idx--> |
// TRACK 1      /--------/---------+--/---------------/------------+----/
// TRACK 0 /----/--------------/---+-----------/------/------------+---/
//             p2                 p3 <--------------i------------> |
// BUFF                            /-------------------------------+-------/
//
//
// 总结：
// idx + offset_played + offset_refit == position - track_position + i
// i:             已生成的 PCM 样本总数（同时也是循环过程中累计的进度偏移，单位：样本帧）
// offset:        当前轨道起始到处理位置的绝对样本偏移： offset = ctx->position - ctx->track_position
// offset_played: 当前轨道上，已遍历过的 Note 节点累计样本数（每处理一个 Note，累加其 pcm_data->sample_num）
// offset_refit:  重拟合位移：当多轨对齐时，因 ctx->position 和 ctx->track_position 被重置而产生的修正值。用于保证 INDEX 计算正确。


/* 合并PCM数据 */
size_t music_ctx_gen_pcm(MusicCtx_t *ctx)
{
	if (!ctx || !ctx->notes) return 0;
	if (!ctx->tracks[0])    /* 刷新查找当前位置 */
		if (!music_ctx_tracks_reset(ctx)) return 0;
	memset(ctx->buffer, 0, sizeof(*ctx->buffer)*ctx->buffer_len*2);
	Note_t *p = NULL;
	size_t i = 0,
	       offset = 0,    /* 相对位移简写 */
	       offset_played = 0,    /* ctx->track_position到当前节点的位移 */
	       offset_refit = 0;     /* ctx->position重新设置的位移的反位移 */
#define INDEX (offset + i - offset_played - offset_refit)    /* pwav的索引值 */
	for (int16_t track = ARRAY_LEN(ctx->tracks)-1; track >= 0; track--) {
		p = ctx->tracks[track];
		if (!p) continue;
		offset = ctx->position - ctx->track_position;
		offset_played = 0;
		for (i = offset_refit; p && p->track == track && p->pcm_data;
		     offset_played+=p->pcm_data->sample_num, p=p->next) {
			if (offset_played+offset_refit > offset+i) break;    /* 避免INDEX为负 */
			if (INDEX >= p->pcm_data->sample_num) continue;
			// if (p->biquad) biquad_compile(p->biquad);
			if (p->flg_legato||p->flg_be_legato||p->flg_portamento||p->flg_be_portam) {
				if (!music_ctx_group_synth(ctx, p, &i, INDEX)) break;
			} else if (!music_ctx_note2buf(ctx, p, &i, INDEX)) break;
		}
		if (!p) continue;
		if (i < ctx->buffer_len && track == 0 && p->track != 0) {    /* 单轨结束，开始多音轨部分 */
			offset_refit += i;
			ctx->position += i;
			if (!music_ctx_tracks_reset(ctx)) return 0;
			track = ARRAY_LEN(ctx->tracks);    /* 使track回到最大值 */
			continue;
		}
	}
	ctx->position += i - offset_refit;
	return i;
}

void music_ctx_pcm_to_i16(MusicCtx_t *ctx)
{
	if (!ctx || !ctx->pcmf16_buffer || !ctx->buffer) return;
	memset(ctx->pcmf16_buffer, 0, sizeof(*ctx->pcmf16_buffer)*ctx->buffer_len*2);
	for (size_t i = 0; i < ctx->buffer_len * 2; i++) {
		/* 归一化裁切处理 */
		if (ctx->buffer[i]) ctx->pcmf16_buffer[i] = tanh(ctx->buffer[i]*ctx->amplitude)*INT16_MAX;
	}
	return;
}
