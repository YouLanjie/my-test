/**
 * @file        string_view.c
 * @author      u0_a221
 * @date        2026-04-30
 * @brief       简要描述该文件的作用
 */

#include "../include/string_view.h"

void sv_chop_left(SV_t *s, size_t len)
{
	if (!s) return;
	if (len > s->len) len = 0;
	s->len -= len;
	s->p += len;
	return;
}

void sv_chop_right(SV_t *s, size_t len)
{
	if (!s) return;
	if (len > s->len) len = 0;
	s->len -= len;
	return;
}

SV_t sv_chop_by_delim(SV_t *s, char delim)
{
	if (!s) return (SV_t){0,NULL};
	size_t i = 0;
	while (i < s->len && s->p[i] != delim) i++;
	SV_t left = *s;
	if (i < s->len) {
		sv_chop_right(&left, left.len-i);
		sv_chop_left(s, i+1);
	} else {
		sv_chop_left(s, i);
	}
	return left;
}

