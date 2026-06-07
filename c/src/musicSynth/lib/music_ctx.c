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
		p = p->pNext;
	}
	NoteData_t *nd = ctx->notes->pcm_data;
	while (nd) {
		count2++;
		nd = nd->pNext;
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
		p = p->pNext;
	}
	if (!p) return false;
	if (!ctx->tracks[0]) {
		ctx->tracks[0] = p;
		ctx->track_position = position;
	}
	return true;
}

/* 将单个Note_t的数据转入buffer */
static bool music_ctx_note2buf(MusicCtx_t *ctx, Note_t *p, size_t *i, size_t offset)
{
	if (!ctx || !p || !i) return false;
	if (*i >= ctx->buffer_len) return false;
	// if (*i+offset >= p->pcm_data->sample_num) return 0;

	double value = 0;
	for (; *i < ctx->buffer_len && *i+offset < p->pcm_data->sample_num; (*i)++) {
		if (!p->pcm_data->pwav) note_gen_wave(p->pcm_data, ctx->flg_no_har);
		if (!p->pcm_data->pwav) {
			*i += p->pcm_data->sample_num - (*i+offset);
			if (*i > ctx->buffer_len) *i = ctx->buffer_len;
			return true;
		}
		value = p->pcm_data->pwav[*i+offset];
		if (!ctx->flg_no_fade) value *= adsr_get_envelope(&p->adsr, *i+offset, p->pcm_data->sample_num);
		if (p->biquad) value = biquad_apply(p->biquad, value);
		value *= p->amplitude;
		if (p->flg_left)  ctx->buffer[*i*2]   += value;
		if (p->flg_right) ctx->buffer[*i*2+1] += value;
	}
	return true;
}

static bool music_ctx_group_synth(MusicCtx_t *ctx, Note_t *p, Note_t *pl, size_t *i, size_t offset)
{
	if (!ctx || !p || !p->pcm_data || !i) return false;
	if (*i >= ctx->buffer_len) return false;
	if (!pl) pl = note_search_last(ctx->notes, p);
	if (pl && pl->pcm_data && p == pl->pNext &&
	    ((p->flg_be_portam && pl->flg_portamento) || 
	     (p->flg_be_legato && pl->flg_legato))) {
		p = pl;
		offset+=p->pcm_data->sample_num;
	}
	if (!p->flg_portamento && !p->flg_legato && !p->flg_be_legato) return false;
	if ((p->flg_portamento||p->flg_legato)&&(!p->pNext||!p->pNext->pcm_data)) return false;

	const Harmonics_t *har;
	size_t n = 0, j = 0;
	if (p->pcm_data->har_func) n = p->pcm_data->har_func(&har);
	if (n == 0 || ctx->flg_no_har) {
		n = 1;
		har = (Harmonics_t[]){{1, 1, 0}};
	}

	size_t duration = (p->flg_portamento||p->flg_legato) ? \
			  p->pcm_data->sample_num+p->pNext->pcm_data->sample_num:\
			  p->pcm_data->sample_num,
	       i0 = *i;
	double value = 0;
	double t = 0,
	       f = 0;

	while (p && p->pcm_data && *i+offset < duration) {
		for (; *i < ctx->buffer_len; (*i)++) {
			if (*i+offset >= duration) break;

			t = (double)(*i+offset) / SAMPLE_RATE;    // 当前时间（秒）
			f = p->flg_portamento ? get_portamento_freq(p->pcm_data->freq, p->pNext->pcm_data->freq, (double)(*i+offset)/duration, ctx->flg_smooth) : \
			    p->pcm_data->freq;
			for (j = 0, value = 0; j < n; j++) {
				value += har[j].amplitude * p->pcm_data->wave_func(M_PI*2 * har[j].freq_times * f * t) *
					exp(-har[j].decay_speed * t);
			}

			// 应用 ADSR 包络
			if (!ctx->flg_no_fade)
				value *= adsr_get_envelope(&p->adsr, *i+offset, duration);

			// 可选：连音音量渐变（fade in/out）
			if (p->flg_be_legato && pl && pl->pcm_data) {      // 被连音音符，起始渐入
				value *= sigmoid((int32_t)(*i+offset-pl->pcm_data->sample_num) / (SAMPLE_RATE / 100.0));
			} if (p->flg_legato) {         // 连音音符，尾部渐出
				value *= sigmoid((int32_t)(p->pcm_data->sample_num - (*i+offset)) / (SAMPLE_RATE / 100.0));
			}

			// 应用滤波器
			if (p->biquad) value = biquad_apply(p->biquad, value);

			// 幅度修正
			value *= p->amplitude;

			// 累加到输出缓冲区（立体声）
			if (p->flg_left)  ctx->buffer[(*i) * 2]     += value;
			if (p->flg_right) ctx->buffer[(*i) * 2 + 1] += value;
		}
		if (!(p->flg_legato && p->pNext && p->pNext->flg_be_legato)) break;
		pl = p;
		p = p->pNext;
		*i = i0;
	}
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
//       | <------o1------> | <--ind--> |
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
//                   | <---o3---> | <-o2-> | <-ind-> |
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
//              | <-------------o4------------------> | <---ind--> |
// TRACK 1      /--------/---------+--/---------------/------------+----/
// TRACK 0 /----/--------------/---+-----------/------/------------+---/
//             p2                 p3 <--------------i------------> |
// BUFF                            /-------------------------------+-------/
//
//
// 总结：
// ind + offset_played + offset_refit == position - track_position + i

/* 合并PCM数据 */
size_t music_ctx_gen_pcm(MusicCtx_t *ctx)
{
	if (!ctx || !ctx->notes) return 0;
	if (!ctx->tracks[0])    /* 刷新查找当前位置 */
		if (!music_ctx_tracks_reset(ctx)) return 0;
	memset(ctx->buffer, 0, sizeof(*ctx->buffer)*ctx->buffer_len*2);
	Note_t *p = NULL, *pl = NULL;
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
		     offset_played+=p->pcm_data->sample_num, pl = p, p=p->pNext) {
			if (offset_played+offset_refit > offset+i) break;    /* 避免INDEX为负 */
			if (INDEX >= p->pcm_data->sample_num) continue;
			if (p->biquad) biquad_compile(p->biquad);
			if (p->flg_legato||p->flg_be_legato||p->flg_portamento||p->flg_be_portam) {
				if (!music_ctx_group_synth(ctx, p, pl, &i, INDEX-i)) break;
			} else if (!music_ctx_note2buf(ctx, p, &i, INDEX-i)) break;
		}
		// if (i > len) i = len;
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
