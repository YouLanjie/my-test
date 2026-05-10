/**
 * @file        string_view.h
 * @author      u0_a221
 * @date        2026-04-30
 * @brief       尝试实现引用字符串
 */

#pragma once

#ifndef _STRING_VIEW_H
#define _STRING_VIEW_H

#include <stdio.h>
#include <stdbool.h>
#include <string.h>

typedef struct {
	size_t len;
	const char *p;
} SV_t;

static inline SV_t sv_from_cstr(const char *p)
{
	if (!p) return (SV_t){.len=0, .p=NULL};
	return (SV_t){.len=strlen(p), .p=p};
}
void sv_chop_left(SV_t *s, size_t len);
void sv_chop_right(SV_t *s, size_t len);
SV_t sv_chop_by_delim(SV_t *s, char delim);    // 返回左侧部分，*s保存右侧部分
SV_t sv_chop_by_type(SV_t *s, int (*istype)(int c));
void sv_trim_left_by_type(SV_t *s, int (*istype)(int c));
bool sv_cmp(SV_t s1, SV_t s2);


/* 具有所有权的sv */
typedef struct {
	size_t capacity;
	size_t len;
	char *p;
} SVA_t;
int sva_create(SVA_t *s);
int sva_free(SVA_t *s);
int sva_from_sv(SVA_t *s, SV_t sv);
int sva_from_cstr(SVA_t *s, const char *p);
static inline SV_t sv_from_sva(const SVA_t *s)   /* 注意需要避免SVA释放后SV仍存在 */
{
	return s ? (SV_t){.len=s->len, .p=s->p} : (SV_t){.len=0, .p=NULL};
}
int sva_smallest(SVA_t *s);
int sva_double(SVA_t *s);
int sva_sprintf(SVA_t *ret,char *fmt, ...) __attribute__((format(printf, 2, 3)));
int sva_sprintfcat(SVA_t *ret, char *fmt, ...) __attribute__((format(printf, 2, 3)));

#endif //STRING_VIEW_H

