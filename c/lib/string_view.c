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
	if (!s || !istype) return;
	while(s->len > 0 && istype(s->p[0])) sv_chop_left(s, 1);
}

void sv_trim_right_by_type(SV_t *s, int (*istype)(int c))
{
	if (!s || !istype) return;
	while(s->len > 0 && istype(s->p[s->len-1])) sv_chop_right(s, 1);
}

bool sv_issame(SV_t s1, SV_t s2)
{
	if (memcmp(&s1, &s2, sizeof(s1)) == 0) return true;
	if (!s1.len && !s2.len) return true;
	if (!s1.p || !s2.p) return false;
	if (s1.len != s2.len) return false;
	if (s1.p == s2.p) return true;
	return memcmp(s1.p, s2.p, s1.len) == 0;
}

#define min(var1, var2) ((var1)<=(var2) ? (var1) : (var2))
#define max(var1, var2) ((var1)>=(var2) ? (var1) : (var2))
int sv_cmp(SV_t s1, SV_t s2)
{
	if (s1.p == s2.p && s1.len == s2.len) return 0;
	if (!s1.p || !s2.p) return s1.p ? 1 : 0;
	if (!s1.len && !s2.len) return 0;
	size_t len = min(s1.len, s2.len);
	int ret = memcmp(s1.p, s2.p, len);
	if (ret == 0) return s1.len - s2.len;
	return ret;
}

int sv_case_cmp(SV_t s1, SV_t s2)
{
	if (s1.p == s2.p && s1.len == s2.len) return 0;
	if (!s1.p || !s2.p) return s1.p ? 1 : 0;
	if (!s1.len && !s2.len) return 0;
	size_t len = min(s1.len, s2.len);
	int ret = strncasecmp(s1.p, s2.p, len);
	if (ret == 0) return s1.len - s2.len;
	return ret;
}

bool sv_begin_with(SV_t s, SV_t pat)
{
	if (!s.p || !pat.p) return false;
	if (s.len < pat.len) return false;
	return !memcmp(s.p, pat.p, pat.len);
}

bool sv_end_with(SV_t s, SV_t pat)
{
	if (!s.p || !pat.p) return false;
	if (s.len < pat.len) return false;
	return !memcmp(s.p+s.len-pat.len, pat.p, pat.len);
}

bool sv_case_begin_with(SV_t s, SV_t pat)
{
	if (!s.p || !pat.p) return false;
	if (s.len < pat.len) return false;
	return !strncasecmp(s.p, pat.p, pat.len);
}

bool sv_case_end_with(SV_t s, SV_t pat)
{
	if (!s.p || !pat.p) return false;
	if (s.len < pat.len) return false;
	return !strncasecmp(s.p+s.len-pat.len, pat.p, pat.len);
}

bool sv_forline(SV_t *line, SV_t *left)
{
	if (!line || !left || !left->p || !left->len) return false;
	*line = sv_chop_by_delim(left, '\n');
	if (line->len && line->p[line->len-1] == '\r') sv_chop_right(line, 1);
	return true;
}

bool sv_forline_reverse(SV_t *line, SV_t *left)
{
	if (!line || !left || !left->p || !left->len) return false;
	size_t i = 0;
	while (i < left->len && left->p[left->len-i-1] != '\n') i++;
	// printf("-- %zu/%zu\n", i, left->len);
	*line = *left;
	sv_chop_left(line, left->len-i);
	sv_chop_right(left, min(i+1, left->len));
	if (left->len && left->p[left->len-1] == '\r') sv_chop_right(left, 1);
	if (!line->len && !left->len) return false;
	return true;
}

size_t sv_countlines(SV_t content)
{
	SV_t line = {};
	size_t ind = 0;
	while (sv_forline(&line, &content)) ind++;
	return ind;
}

SV_t sv_seekline(SV_t base, SV_t slice, int line_offset)
{
	if (!base.p || !slice.p || slice.p < base.p || slice.p+slice.len > base.p+base.len)
		return (SV_t){};
	// const int8_t direc = line_offset >= 0 ? 1 : -1;
	size_t idx = slice.p - base.p;
	while (idx > 0 && base.p[idx] != '\n') idx--;
	if (base.p[idx] == '\n' && idx < base.len) idx++;
	if (idx && slice.p[0] == '\n') idx--;
	int count = 0;
	if (line_offset >= 0) {
		line_offset++;
		sv_chop_left(&base, idx);
		while (count < line_offset && sv_forline(&slice, &base)) count++;
	} else {
		line_offset--;
		sv_chop_right(&base, base.len-idx);
		while (count > line_offset && sv_forline_reverse(&slice, &base)) count--;
	}
	if (count != line_offset) slice.len = 0;
	return slice;
}

size_t sv_getlinenum(SV_t base, SV_t slice)
{
	slice = sv_seekline(base, slice, 0);
	if (!slice.p) return 0;
	if (!slice.len) slice.len++;
	base.len = slice.p+slice.len-base.p;
	return sv_countlines(base);
}

SV_t sv_merge(SV_t base, SV_t slice1, SV_t slice2)
{
	if (!base.p ||
	    !slice1.p || slice1.p < base.p || slice1.p+slice1.len > base.p+base.len ||
	    !slice2.p || slice2.p < base.p || slice2.p+slice2.len > base.p+base.len)
		return (SV_t){};
	// const int8_t direc = line_offset >= 0 ? 1 : -1;
	const char *p1 = min(slice1.p, slice2.p);
	const char *p2 = max(slice1.p+slice1.len, slice2.p+slice2.len);
	return (SV_t){.p = p1, .len = p2-p1};
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

SVA_t *sva_chop_right(SVA_t *s, size_t len)
{
	if (!s || !s->p || !s->capacity) return NULL;
	if (len > s->len) len = 0;
	s->len -= len;
	if (s->len < s->capacity) s->p[s->len] = '\0';
	return s;
}

SVA_t *sva_clear(SVA_t *s)
{
	if (!s || !s->p || !s->capacity) return NULL;
	if (s->len > s->capacity) s->len = s->capacity;
	memset(s->p, 0, s->len);
	s->len = 0;
	return s;
}
