/**
 * @file        string_view.c
 * @author      u0_a221
 * @date        2026-04-30
 * @brief       简要描述该文件的作用
 */

#include "../include/string_view.h"
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>

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
	if (!s1.p || !s2.p) return false;
	if (s1.len != s2.len) return false;
	if (s1.p == s2.p) return true;
	return memcmp(s1.p, s2.p, s1.len) == 0;
}

bool sv_begin_with(SV_t s, SV_t pat)
{
	if (!s.p || !pat.p) return false;
	if (s.len < pat.len) return false;
	return !strncmp(s.p, pat.p, pat.len);
}

bool sv_end_with(SV_t s, SV_t pat)
{
	if (!s.p || !pat.p) return false;
	if (s.len < pat.len) return false;
	return !strncmp(s.p+s.len-pat.len, pat.p, pat.len);
}


/* ===================
 * SVA相关
 * =================== */

/* 创建sva_t对象 */
SVA_t *sva_create(SVA_t *s)
{
	if (!s) return NULL;
	s->capacity = 128;
	s->len = 0;
	s->p = malloc(s->capacity);
	if (!s->p) s->capacity = 0;
	else memset(s->p, 0, s->capacity);
	return s;
}

int sva_free(SVA_t *s)
{
	if (!s) return -1;
	if (s->p) free(s->p);
	s->p = NULL;
	s->len = 0;
	s->capacity = 0;
	return 0;
}

SVA_t *sva_from_sv(SVA_t *s, SV_t sv)
{
	if (!s || !sv.p) return NULL;
	s->len = sv.len;
	/* 若已申请内存且长度足够则不申请内存 */
	if (!s->p || s->capacity <= s->len) {
		if (s->p) free(s->p);
		s->capacity = s->len+1;
		s->p = malloc(s->capacity);
	}
	if (!s->p) {
		s->capacity = 0;
		s->len = 0;
		return NULL;
	}
	memcpy(s->p, sv.p, s->len);
	s->p[s->len] = 0;
	return s;
}

SVA_t *sva_from_cstr(SVA_t *s, const char *p)
{
	if (!s || !p) return NULL;
	return sva_from_sv(s, sv_from_cstr(p));
}

SVA_t *sva_smallest(SVA_t *s)
{
	if (!s) return NULL;
	if (!s->p) return s;
	s->capacity = s->len+1;
	char *old_p = s->p;
	s->p = realloc(s->p, s->capacity);
	if (!s->p) {
		s->capacity = 0;
		s->len = 0;
		free(old_p);
		return NULL;
	}
	return s;
}

SVA_t *sva_double(SVA_t *s)
{
	if (!s || s->capacity == 0 || s->p == NULL) return NULL;
	s->capacity *= 2;
	char *old_p = s->p;
	s->p = realloc(s->p, s->capacity);
	if (!s->p) {
		free(old_p);
		s->capacity = 0;
		s->len = 0;
		return NULL;
	}
	return s;
}

SVA_t *sva_adjust_minimun(SVA_t *s, size_t size)
{
	if (!s) return NULL;
	if (s->capacity >= size) return s;
	s->capacity = size;
	char *old_p = s->p;
	if (old_p) s->p = realloc(s->p, s->capacity);
	else s->p = malloc(s->capacity);
	if (!s->p && old_p) {
		free(old_p);
		s->capacity = 0;
		s->len = 0;
		return NULL;
	}
	return s;
}

SVA_t *sva_sprintf(SVA_t *ret, char *fmt, ...)
{
	if (!ret || !fmt) return NULL;
	va_list ap;
	int64_t n = 0;

	va_start(ap, fmt);
	n  = vsnprintf(NULL, 0, fmt, ap);    /* 检测所需容量 */
	va_end(ap);
	if (n < 0) return NULL;
	if ((size_t)n + 1 > ret->capacity) {
		ret->capacity = n + 1;
		ret->p = realloc(ret->p, ret->capacity);
	}
	if (!ret->p) return ret->capacity = 0, NULL;

	va_start(ap, fmt);
	n = vsnprintf(ret->p, ret->capacity, fmt, ap);
	va_end(ap);
	if (n < 0) {
		free(ret->p);
		ret->p = NULL, ret->capacity = 0;
		return NULL;
	}
	ret->len = n;
	return ret;
}

/* strcat,但是有fmt */
SVA_t *sva_sprintfcat(SVA_t *ret, char *fmt, ...)
{
	if (!ret || !fmt) return NULL;
	va_list ap;
	int64_t n = 0;

	va_start(ap, fmt);
	n  = vsnprintf(NULL, 0, fmt, ap);    /* 检测所需容量 */
	va_end(ap);
	if (n < 0) return NULL;
	if (ret->len + n + 1 > ret->capacity || !ret->p) {
		ret->capacity = ret->len + n + 1;    /* 扩增式 */
		ret->p = realloc(ret->p, ret->capacity);
		if (!ret->p) return ret->capacity = 0, NULL;
	}

	va_start(ap, fmt);
	n = vsnprintf(ret->p+ret->len, ret->capacity-ret->len, fmt, ap);
	va_end(ap);
	if (n < 0) {
		free(ret->p);
		ret->p = NULL, ret->capacity = 0, ret->len = 0;
		return NULL;
	} else ret->len += n;
	return ret;
}

SVA_t *sva_strcpy(SVA_t *ret, const SVA_t *from)
{
	return sva_from_sv(ret, sv_from_sva(from));
}
