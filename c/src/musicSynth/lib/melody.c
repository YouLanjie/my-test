/**
 * @file        melody.c
 * @author      u0_a221
 * @date        2026-05-02
 * @brief       旋律相关（旋律处理，合并轨道）
 */

#include "../core.h"

/* 将所有非0轨道合并到轨道0 */
void merge_tracks(Note_t *tracks[UINT8_MAX], int duration, uint64_t offset, bool print_merge_info)
{
	/* 用ai来修bug还是可以的hhh */
	if (!tracks[0] || !tracks[0]->pwav)
		return;

	if (offset%2 != 0) offset++;
	int16_t *pwav = tracks[0]->pwav;	// 主轨道缓冲区，长度 2*duration

	if (print_merge_info)
		fprintf(stderr, "> merge: duration:%d, offset:%ld\n", duration, offset);
	for (uint8_t i = 1; i < UINT8_MAX; i++) {
		if (!tracks[i])
			continue;
		Note_t *p = tracks[i];
		Note_t *now_merge = NULL;
		if (print_merge_info){
			fprintf(stderr, "> H[%d] ", i);
			print_note(p);
		}
		uint64_t len = 0;	// 已处理的样本数（时间）
		for (int j = 0; j < duration; j++) { // 找到覆盖当前时间 offset+j 的音符
			while (p) { // 跳过无音频数据的音符，并累加其持续时间
				if (!p->pwav) {
					/*len += p->duration;*/
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
			if (print_merge_info && now_merge != p) {
				now_merge = p;
				fprintf(stderr, "> ");
				print_note(p);
			}
			if (offset + j < len) {
				fprintf(stderr, "合并声轨时产生未知错误: %ld < %ld\n",
					offset + j, len);
				print_note(p);
				break;
			}
			int note_offset = (offset + j) - len;	// 在当前音符内的偏移（样本数）
			// 叠加左右声道
			pwav[2 * j] += p->pwav[2 * note_offset];
			pwav[2 * j + 1] += p->pwav[2 * note_offset + 1];
		}
	}
}

/* free space of pwav */
void clean_tracks_wav(Note_t *(*tracks)[UINT8_MAX], bool print)
{
	if (!tracks || !*tracks) return;
	if (print)
		fprintf(stderr, "[INFO] CLEANUP TRACKS\n");
	for(uint8_t i=1; i < UINT8_MAX; i++) {
		if (!(*tracks)[i]) continue;
		if (print)
			print_note((*tracks)[i]);
		for(Note_t *p = (*tracks)[i]; p && p->track == i; p = p->pNext) {
			if (!p->pwav) continue;
			free(p->pwav);
			p->pwav = NULL;
		}
		(*tracks)[i] = NULL;
	}
	*tracks[0] = 0;
	return;
}

/* RET: 总数据长度(用于保存计算) */
uint64_t melody_0(Note_t *pH, void (*pcm_handle)(int16_t*,int), struct Melody_opts_t opts)
{
	if (!pH) return 0;
	uint64_t size = 0;    /* 已播放采样大小（数量） */
	int64_t sizes[UINT8_MAX] = {0};
	char *colors[2][2] = { {"", ""}, {"\e[31m", "\e[0m"} };
	int istty = isatty(STDERR_FILENO);
	Note_t *p = pH, *tracks[UINT8_MAX] = {NULL};
	if (pH) check_notes(pH, opts.print_formated_note);
	if (!pcm_handle) return 0;
	for (; p ; p = p->pNext) {
		if (!p) break;
		int duration = create_note_wave(&p, opts.no_fade, opts.smooth, opts.no_har);
		if (duration && opts.print_debug_info) print_note(p);
		if (!p) break;
		if (duration <= 10) {
			fprintf(stderr,"[%sWARN%s] 波形为空(Code[%d]): ",
				colors[istty][0], colors[istty][1], duration);
			print_note(p);
			continue;
		}

		if (p->track == 0 && sizes[0] == 0) {    /* 直接播放 */
			size+=duration;
			pcm_handle(p->pwav, duration);
			free(p->pwav);
			p->pwav = NULL;
		} else if (p->track == 0) {    /* 并轨播放 */
			if (sizes[0] == -1) {
				sizes[0] = 0;
				for(uint8_t i=1; i < UINT8_MAX; i++) {
					/* 计算1以上的最长轨道 */
					if(sizes[i]>sizes[0])sizes[0]=sizes[i];
					sizes[i] = 0;    /* max size */
				}
				sizes[1] = size;    /* 保存当前已经播放采样数(偏移) */
			}
			tracks[0] = p;
			merge_tracks(tracks, duration, size-sizes[1], opts.print_debug_info);
			size+=duration;
			pcm_handle(p->pwav, duration);
			free(p->pwav);
			p->pwav = NULL;
			if (size-sizes[1] >= sizes[0]) {
				// exit clean up
				clean_tracks_wav(&tracks, 0);
				sizes[0] = 0;
				sizes[1] = 0;
			}
		} else {    /* 独立计算轨道 */
			if (sizes[0] > 0) {
				fprintf(stderr,
					"[%sWARN%s] 非零轨道比主轨道长(%lf - %lf = %lf > 0)\n",
					colors[istty][0], colors[istty][1],
					(double)sizes[0]/SAMPLE_RATE,
					(double)(size-sizes[1])/SAMPLE_RATE,
					(double)(sizes[0]-(size-sizes[1]))/SAMPLE_RATE);
				clean_tracks_wav(&tracks, 1);
				sizes[0] = 0;
				sizes[1] = 0;
			}
			if (!tracks[p->track]) tracks[p->track] = p;
			sizes[p->track] += duration;
			sizes[0] = -1;
		}
	}
	return size;
}

