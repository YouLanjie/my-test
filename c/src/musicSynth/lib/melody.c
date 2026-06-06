/**
 * @file        melody.c
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
	};
	if (!p->buffer || !p->pcmf16_buffer) {
		music_ctx_free(p);
		return NULL;
	}
	return p;
}

/* 更新ctx轨道指针(重新遍历链表) */
bool music_ctx_tracks_reset(MusicCtx_t *ctx)
{
	if (!ctx || !ctx->notes) return false;
	size_t position = 0,
	       max_len = 0,
	       track_sizes[ARRARY_LEN(ctx->tracks)] = {0};
	Note_t *p = ctx->notes;
	uint8_t track = 0;
	memset(ctx->tracks, 0, sizeof(ctx->tracks));
	while (p && p->pcm_data) {
		if (track != 0 && p->track == 0) {    /* 切出 */
			ctx->tracks[0] = p;
			for (size_t i = 1; i < ARRARY_LEN(track_sizes); i++)
				if(track_sizes[i]>max_len) max_len=track_sizes[i];
			memset(track_sizes, 0, sizeof(track_sizes));
			/* 保存分轨初始位置
			 * 因为计算其他轨道时主轨道长度未被计算 */
			ctx->track_position = position;
			track = 0;
		} else if (p->track != track) {    /* 其余轨道切换(包括切入) */
			ctx->tracks[p->track] = p;
			track = p->track;
		} else    /* 累加轨道长度 */
			track_sizes[p->track] += p->pcm_data->sample_num;

		if (p->track == 0) position += p->pcm_data->sample_num;
		if (max_len && position-ctx->track_position >= max_len) {
			ctx->track_position = 0;
			max_len = 0;
			memset(ctx->tracks, 0, sizeof(ctx->tracks));
		}
		if (position > ctx->position) {
			position -= p->pcm_data->sample_num;
			break;
		}
		p = p->pNext;
	}
	if (!p) return false;
	ctx->tracks[0] = p;
	ctx->track_position = position;
	return true;
}

size_t music_ctx_gen_pcm(MusicCtx_t *ctx)
{
	if (!ctx || !ctx->notes) return 0;
	if (!ctx->tracks[0])    /* 刷新查找当前位置 */
		if (!music_ctx_tracks_reset(ctx)) return 0;
	memset(ctx->buffer, 0, sizeof(*ctx->buffer)*ctx->buffer_len*2);
	Note_t *p = NULL;
	size_t i = 0,
	       begin = 0,
	       max_i = 0,    /* 最长到达位置(主谱位置) */
	       offset = 0,    /* 起始点向后正向偏移 */
	       offset_played = 0,    /* 反向偏移 */
	       len = ctx->buffer_len;
	double value = 0;
	for (int16_t track = ARRARY_LEN(ctx->tracks)-1; track >= 0; track--) {
		p = ctx->tracks[track];
		if (!p) continue;
		if (p->biquad) biquad_compile(p->biquad);
		offset = ctx->position - ctx->track_position;
		offset_played = 0;
		for (i = begin; i < len; i++) {
			if (!p->pcm_data) break;
			while (p && p->track == track && offset+i-offset_played-begin >= p->pcm_data->sample_num) {
				offset_played += p->pcm_data->sample_num;
				p = p->pNext;
				// offset = 0;
			}
			if (!p) break;
			if (p->track != track) break;
			if (offset_played > offset+i-begin) {
				i = offset_played + begin - offset - 1;
				continue;
			}

			if (!p->pcm_data->pwav) note_gen_wave(p->pcm_data);
			if (!p->pcm_data->pwav) {
				i = p->pcm_data->sample_num+offset_played+begin - offset - 1;
				continue;
			}
			value = p->pcm_data->pwav[offset+i-offset_played-begin] * adsr_get_envelope(&p->adsr, offset+i-offset_played-begin, p->pcm_data->sample_num);
			if (p->biquad) value = biquad_apply(p->biquad, value);
			value *= p->amplitude;
			ctx->buffer[i*2]   += value;
			ctx->buffer[i*2+1] += value;
		}
		if (i > len) i = len;
		if (track == 0) max_i = i;
		if (!p) continue;
		if (track == 0 && p->track != 0) {    /* 单轨结束，开始多音轨部分 */
			begin += i;
			ctx->position += i;
			if (!music_ctx_tracks_reset(ctx)) return 0;
			track = ARRARY_LEN(ctx->tracks);    /* 使track回到最大值 */
			continue;
		}
	}
	ctx->position += max_i - begin;
	return max_i;
}

void music_ctx_pcm_to_i16(MusicCtx_t *ctx, double scale)
{
	if (!ctx || !ctx->pcmf16_buffer || !ctx->buffer) return;
	if (scale <= 0) scale = 3;
	memset(ctx->pcmf16_buffer, 0, sizeof(*ctx->pcmf16_buffer)*ctx->buffer_len*2);
	for (size_t i = 0; i < ctx->buffer_len * 2; i++) {
		/* 归一化裁切处理 */
		if (ctx->buffer[i]) ctx->pcmf16_buffer[i] = tanh(ctx->buffer[i]/scale)*INT16_MAX;
		// else ctx->pcmf16_buffer[i] = 0;
		// printf("%ld,%d\n", i, ctx->pcmf16_buffer[i]*INT16_MAX);
	}
	return;
}
