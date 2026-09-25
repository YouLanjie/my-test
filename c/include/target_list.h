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
	enum {TS_NOCHECK = 0, TS_WORKING, TS_SUCCESS, TS_FAILD, TS_SKIP} status;

	SVA_t log;
	double progress;

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
/**
 * @brief 打印任务列表
 *
 * @param list 列表本身
 * @param mode 模式
 * 0: TS_NOCHECK (ON)
 * 1: TS_WORKING
 * 2: TS_SUCCESS
 * 3: TS_SKIP    (ON)
 * 4: TS_FAILD   (ON)
 * 5: TY_NORM    (ON)
 * 6: TY_PHONY   (ON)
 * 7: TY_DEP     (ON)
 * 8: 仅有已更新项目
 */
void target_printlist(Target_t *list, uint16_t mode);
/* 按照总依赖多少重排序(由少到多)，返回新表头 */
Target_t *target_sort_by_subdeps(Target_t *list);
/**
 * @brief 递归遍历文件夹
 *
 * @param list 要查找的目标列表(留空自动创建)
 * @param cwd 工作目录，可以为空
 * @param dirname 要查找的工作目录下的子目录
 * @param rule 规则判断函数,返回false则跳过
 * @param action 对文件的行为函数
 * @return 构建好的列表
 */
Target_t *target_fordir(Target_t *list, char *cwd, SV_t dirname,
			bool (*rule)(SV_t d_name, uint8_t d_type),
			Target_t *(*action)(Target_t *list, SV_t full_path));

#endif //TARGET_LIST_H

