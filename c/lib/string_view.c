/**
 * @file        string_view.c
 * @author      u0_a221
 * @date        2026-04-30
 * @brief       简要描述该文件的作用
 */

#include "../include/string_view.h"
#include <string.h>

SV_t sv_from_cstr(const char *p)
{
	if (!p) return (SV_t){.len=0, .p=NULL};
	return (SV_t){.len=strlen(p), .p=p};
}

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

SV_t sv_chop_by_type(SV_t *s, int (*istype)(int c))
{
	if (!s) return (SV_t){0,NULL};
	size_t i = 0;
	while (i < s->len && !istype(s->p[i])) i++;
	SV_t left = *s;
	if (i < s->len) {
		sv_chop_right(&left, left.len-i);
		sv_chop_left(s, i+1);
	} else {
		sv_chop_left(s, i);
	}
	return left;
}

void sv_trim_left_by_type(SV_t *s, int (*istype)(int c))
{
	if (!s) return;
	while(s->len > 0 && istype(s->p[0])) sv_chop_left(s, 1);
}

bool sv_cmp(SV_t s1, SV_t s2)
{
	if (s1.len != s2.len) return false;
	if (s1.p == s2.p) return true;
	return strncmp(s1.p, s2.p, s1.len) == 0;
}
