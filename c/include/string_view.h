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

SV_t sv_from_cstr(const char *p);
void sv_chop_left(SV_t *s, size_t len);
void sv_chop_right(SV_t *s, size_t len);
SV_t sv_chop_by_delim(SV_t *s, char delim);    // 返回左侧部分，*s保存右侧部分
SV_t sv_chop_by_type(SV_t *s, int (*istype)(int c));
void sv_trim_left_by_type(SV_t *s, int (*istype)(int c));
bool sv_cmp(SV_t s1, SV_t s2);

#endif //STRING_VIEW_H

