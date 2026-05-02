/**
 * @file        cstr.c
 * @author      u0_a221
 * @date        2026-05-02
 * @brief       对c字符串的读写操作
 */

#include "../include/string_view.h"
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

typedef struct {
	char *p;
	size_t capacity;
	size_t len;
} SVA_t;

SVA_t sva_create()
{
	SVA_t s;
	s.capacity = 128;
	s.len = 0;
	s.p = malloc(s.capacity);
	if (!s.p) s.capacity = 0;
	return s;
}

SVA_t sv_to_sva(SV_t *sv)
{
	SVA_t s;
	s.capacity = sv->len+1;
	s.len = sv->len;
	s.p = malloc(s.capacity);
	if (!s.p) {
		s.capacity = 0;
		return s;
	}
	strncpy(s.p, sv->p, sv->len);
	s.p[s.len] = 0;
	return s;
}

/* 注意需要避免SVA释放后SV仍存在 */
SV_t sva_to_sv(SVA_t s)
{
	return (SV_t){.p=s.p, .len=s.len};
}

SVA_t *sva_smallest(SVA_t *s)
{
	if (!s) return NULL;
	s->capacity = s->len+1;
	s->p = realloc(s->p, s->capacity);
	return s;
}

SVA_t sva_double(SVA_t *s)
{
	if (!s) return (SVA_t){.capacity=0};
	s->capacity *= 2;
	s->p = realloc(s->p, s->capacity);
	s->p[s->len] = 0;
	return *s;
}

__attribute__((format(printf, 1, 2)))
SVA_t sprintf_alloc_(char *fmt, ...)
{
	SVA_t s = {.capacity=0};
	if (!fmt) return s;
	va_list ap;
	int64_t n = 0;

	va_start(ap, fmt);
	n  = vsnprintf(NULL, 0, fmt, ap);    /* 检测所需容量 */
	va_end(ap);
	if (n < 0) return s;
	s.capacity = n + 1;
	s.p = malloc(s.capacity);
	if (!s.p) return s.capacity = 0, s;

	va_start(ap, fmt);
	n = vsnprintf(s.p, s.capacity, fmt, ap);
	va_end(ap);
	if (n < 0) {
		free(s.p);
		s.p = 0, s.capacity = 0;
		return s;
	}
	/*s.len = strlen(s.p);*/
	/*sva_smallest(&s);*/
	s.len = n;
	return s;
}

