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
/* 从字面量字符串创建sv */
#define sv_from_lstr(str) ((SV_t){.len=sizeof(str)-1, .p=str})
void sv_chop_left(SV_t *s, size_t len);
void sv_chop_right(SV_t *s, size_t len);
SV_t sv_chop_by_delim(SV_t *s, char delim);    // 返回左侧部分，*s保存右侧部分，分隔符不保留
SV_t sv_chop_by_type(SV_t *s, int (*istype)(int c));    // 返回左侧部分，*s保存右侧部分
void sv_trim_left_by_type(SV_t *s, int (*istype)(int c));
bool sv_cmp(SV_t s1, SV_t s2);
bool sv_end_with(SV_t s, SV_t pat);
bool sv_begin_with(SV_t s, SV_t pat);


/* 具有所有权的sv */
typedef struct {
	size_t capacity;
	size_t len;
	char *p;
} SVA_t;
SVA_t *sva_create(SVA_t *s);
int sva_free(SVA_t *s);
/**
 * @brief 将sv复制到SVA(strcpy)
 *
 * @param s 目标地址，会自动申请、扩大内存
 * @param sv 原字符串
 * @return 设置后的地址
 */
SVA_t *sva_from_sv(SVA_t *s, SV_t sv);
SVA_t *sva_from_cstr(SVA_t *s, const char *p);
#define sva_from_sva(ret, from) sva_from_sv(ret, sv_from_sva(from))
SVA_t *sva_strcpy(SVA_t *ret, const SVA_t *from);
static inline SV_t sv_from_sva(const SVA_t *s)   /* 注意需要避免SVA释放后SV仍存在 */
{
	return s ? (SV_t){.len=s->len, .p=s->p} : (SV_t){.len=0, .p=NULL};
}
SVA_t *sva_smallest(SVA_t *s);
/* 二倍扩大内存空间 */
SVA_t *sva_double(SVA_t *s);
/* 调整确保内存容量大等于size */
SVA_t *sva_adjust_minimun(SVA_t *s, size_t size);
SVA_t *sva_sprintf(SVA_t *ret,char *fmt, ...) __attribute__((format(printf, 2, 3)));
SVA_t *sva_sprintfcat(SVA_t *ret, char *fmt, ...) __attribute__((format(printf, 2, 3)));
#define sva_cmp(s1, s2) sv_cmp(sv_from_sva(s1), sv_from_sva(s2))
SVA_t *sva_chop_right(SVA_t *s, size_t len);
/* 清除sva的内容但是不释放内存供下次使用 */
SVA_t *sva_clear(SVA_t *s);

#endif //STRING_VIEW_H

