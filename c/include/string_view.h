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

typedef struct {
	size_t len;
	char *p;
} SV_t;

void sv_chop_left(SV_t *s, size_t len);
void sv_chop_right(SV_t *s, size_t len);
SV_t sv_chop_by_delim(SV_t *s, char delim);

#endif //STRING_VIEW_H

