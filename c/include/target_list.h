/**
 * @file        target_list.h
 * @author      Chglish
 * @date        2026-07-16
 * @brief       依赖链库头文件
 */

#pragma once

#ifndef _TARGET_LIST_H
#define _TARGET_LIST_H

#include "path.h"

typedef struct Target_t {
	SVA_t name;
	double time;
	bool (*build)(struct Target_t*);
	void *cfgdata;    /* 任意配置字段 */

	bool isupdated;
	enum {TY_NORM = 0, TY_PHONY, TY_DEP} type;
	enum {TS_NOCHECK = 0, TS_WORKING, TS_SUCCESS, TS_FAILD} status;

	size_t depend_len;
	struct Target_t **dependencies;
	struct Target_t *prev;
	struct Target_t *next;
} Target_t;

Target_t *target_create(SV_t name);
void target_free(Target_t *target);
void target_freelist(Target_t *list);
void target_append(Target_t *list, Target_t *target);
void target_depend_append(Target_t *target, Target_t *dependency);
Target_t *target_get_by_name(Target_t *list, SV_t name);
Target_t *target_get_or_create(Target_t *list, SV_t name);
void target_build(Target_t *target);
void target_buildlist(Target_t *list);
void *target_build_for_pthread(void *target);
void target_buildlist_for_pthread(Target_t *list, int8_t ptr_max);
void target_printlist(Target_t *list, uint8_t mode);

#endif //TARGET_LIST_H

