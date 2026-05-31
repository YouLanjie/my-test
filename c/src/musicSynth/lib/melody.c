/**
 * @file        melody.c
 * @author      u0_a221
 * @date        2026-05-02
 * @brief       旋律相关（旋律处理，合并轨道）
 */

#include "../core.h"
#include <stddef.h>

MusicCtx_t *music_ctx_create(size_t bufflen)
{
	if (bufflen <= 0) return NULL;
	MusicCtx_t *p = malloc(sizeof(*p));
	if (!p) return NULL;
	*p = (MusicCtx_t){
		.notes = NULL,
		.position = 0,
		.track_position = 0,
		.main_offset = 0,
		.tracks = {NULL},
		.buffer_len = bufflen,
		.buffer = malloc(sizeof(*p->buffer)*bufflen*2),
		.pcmf16_buffer = malloc(sizeof(*p->pcmf16_buffer)*bufflen*2),
		.pcm_handle = NULL,
	};
	if (!p->buffer) {
		if (p->pcmf16_buffer) free(p->pcmf16_buffer);
		free(p);
		return NULL;
	}
	if (!p->pcmf16_buffer) {
		if (p->buffer) free(p->buffer);
		free(p);
		return NULL;
	}
	return p;
}

int music_ctx_refresh_tracks(MusicCtx_t *ctx)
{
	if (!ctx || !ctx->notes) return 0;
	size_t position = 0,
	       max_len = 0,
	       track_sizes[ARRARY_LEN(ctx->tracks)];
	Note_t *p = ctx->notes;
	uint8_t track = 0;
	memset(ctx->tracks, 0, sizeof(ctx->tracks));
	memset(track_sizes, 0, sizeof(track_sizes));
	while (p && p->pcm_data) {
		if (track != 0 && p->track == 0) {    /* 切出 */
			ctx->tracks[0] = p;
			for (size_t i = 1; i < ARRARY_LEN(track_sizes); i++)
				if(track_sizes[i]>max_len) max_len=track_sizes[i];
			memset(track_sizes, 0, sizeof(track_sizes));
			/* 保存分轨初始位置
			 * 因为计算其他轨道时主轨道长度未被计算 */
			ctx->track_position = position;
		} else if (p->track != track)    /* 其余轨道切换(包括切入) */
			ctx->tracks[p->track] = p;
		else    /* 累加轨道长度 */
			track_sizes[p->track] += p->pcm_data->sample_num;

		if (p->track == 0) position += p->pcm_data->sample_num;
		if (position-ctx->track_position >= max_len) {
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
	if (!p) return 0;
	ctx->tracks[0] = p;
	ctx->main_offset = ctx->position - position;
	return 1;
}

size_t music_ctx_gen_pcm(MusicCtx_t *ctx)
{
	if (!ctx || !ctx->notes) return 0;
	if (!ctx->tracks[0])    /* 刷新查找当前位置 */
		if (!music_ctx_refresh_tracks(ctx)) return 0;
	memset(ctx->buffer, 0, sizeof(*ctx->buffer)*ctx->buffer_len*2);
	Note_t *p = NULL;
	size_t i = 0,
	       offset = 0,
	       offset_played = 0,
	       len = ctx->buffer_len;
	double value = 0;
	for (uint8_t track = 0; track < UINT8_MAX; track++) {
		p = ctx->tracks[track];
		if (!p) continue;
		if (p->biquad) biquad_compile(p->biquad);
		offset = ctx->main_offset;
		offset_played = 0;
		for (i = 0; i < len; i++) {
			if (offset+i-offset_played > p->pcm_data->sample_num) {
				offset_played += p->pcm_data->sample_num;
				p = p->pNext;
				if (!p) break;
				if (p->track != track) break;
			}
			if (!p->pcm_data->pwav) {
				i += p->pcm_data->sample_num - offset;
				continue;
			}
			value = p->pcm_data->pwav[offset+i] * adsr_get_envelope(&p->adsr, offset+i, p->pcm_data->sample_num);
			value = biquad_apply(p->biquad, value);
			value *= p->amplitude;
			ctx->buffer[i*2]   = value;
			ctx->buffer[i*2+1] = value;
		}
		if (!p) continue;
		if (track == 0 && p->track != 0) {
			ctx->position += i;
			if (!music_ctx_refresh_tracks(ctx)) return 0;
			track = -1;    /* 使track回到0 */
			continue;
		}
	}
	return 0;
}

