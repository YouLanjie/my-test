/**
 * @file        target_list.c
 * @author      Chglish
 * @date        2026-07-16
 * @brief       测试依赖列表功能分离出的文件
 */

#include "../include/target_list.h"
#include <pthread.h>
#include <stdbool.h>
#include <stdcountof.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
// #include <string.h>
#include <unistd.h>

Target_t *target_create(SV_t name)
{
	if (name.len == 0 || !name.p) return NULL;
	Target_t *target = malloc(sizeof(*target));
	if (!target) return NULL;
	*target = (Target_t){};
	sva_from_sv(&target->name, name);
	return target;
}

void target_free(Target_t *target)
{
	if (!target) return;
	if (target->prev) target->prev->next = target->next;
	if (target->next) target->next->prev = target->prev;
	if (target->dependencies) free(target->dependencies);
	free(target);
}

void target_freelist(Target_t *list)
{
	Target_t *next;
	while (list) {
		next = list->next;
		target_free(list);
		list = next;
	}
}

void target_append(Target_t *list, Target_t *target)
{
	if (!list || !target) return;
	if (target->prev || target->next) return;
	if (list == target) return;    /* 跳过已有项 */
	while (list->next) {
		if (list == target) return;
		list = list->next;
	}
	list->next = target;
	target->prev = list;
	return;
}

void target_depend_append(Target_t *target, Target_t *dependency)
{
	if (!target || !dependency) return;
	if (!target->dependencies) {
		target->depend_len = 1;
		target->dependencies = malloc(target->depend_len*sizeof(*target->dependencies));
	} else {
		for (size_t i = 0; i < target->depend_len; i++)
			if (dependency == target->dependencies[i]) return;
		target->depend_len++;
		target->dependencies = realloc(target->dependencies, target->depend_len*sizeof(*target->dependencies));
	}
	if (!target->dependencies) {
		target->depend_len = 0;
		return;
	}
	target->dependencies[target->depend_len-1] = dependency;
}

Target_t *target_get_by_name(Target_t *list, SV_t name)
{
	while (list && sv_cmp(sv_from_sva(&list->name), name) != true) {
		list = list->next;
	}
	return list;
}

Target_t *target_get_or_create(Target_t *list, SV_t name)
{
	Target_t *get = target_get_by_name(list, name);
	if (get) return get;
	get = target_create(name);
	target_append(list, get);
	return get;
}

void target_build(Target_t *target)
{
	if (!target) return;
	/* 跳过已操作项目 */
	if (target->status != TS_NOCHECK) return;
	target->status = TS_WORKING;
	bool isexist    = false;
	bool need_wait  = false;
	bool need_build = false;
	Path_st_t st = path_get_st(target->name);
	target->time = st.st.st_mtim.tv_sec + st.st.st_mtim.tv_nsec*1e-9;
	isexist = st.isexist;
	if (!st.isexist) need_build = true;
	double waittime = 0;
	for (size_t i = 0; i < target->depend_len; i++) {
		target_build(target->dependencies[i]);
		switch (target->dependencies[i]->status) {
		case TS_WORKING:
			need_wait = true;
			break;
		case TS_SUCCESS:
			if (target->type == TY_PHONY)
				need_build = true;
			if (target->dependencies[i]->time > target->time) {
				target->time = target->dependencies[i]->time;
				need_build = true;
			}
			break;
		case TS_NOCHECK:
		case TS_FAILD:
			target->status = TS_FAILD;
			return;
			break;
		}
		if (need_wait && i+1 >= target->depend_len) {
			i = -1;
			usleep(1e3);
			waittime += 1e3;
			if (waittime > 60e6) {
				target->status = TS_FAILD;
				return;
			}
			need_wait = false;
		}
	}
	if (!need_build) {
		target->status = TS_SUCCESS;
		return;
	}
	if (!isexist && !target->build) {
		printf("[ERROR] 没有规则用于构建 %.*s\n", (int)target->name.len, target->name.p);
		target->status = TS_FAILD;
		return;
	}
	if (target->build) {
		// printf("[INFO] 构建 %s\n", target->name.p);
		target->status = target->build(target) ? TS_SUCCESS : TS_FAILD;
		if (target->status == TS_SUCCESS) {
			st = path_get_st(target->name);
			target->time = st.st.st_mtim.tv_sec + st.st.st_mtim.tv_nsec*1e-9;
			target->isupdated = true;
		}
	} else target->status = TS_SUCCESS;
	return;
}

void target_buildlist(Target_t *list)
{
	if (!list) return;
	for (Target_t *p = list; p; p = p->next) {
		if (p->type != TY_NORM) continue;
		target_build(p);
	}
}

void *target_build_for_pthread(void *target)
{
	if (!target) return NULL;
	target_build(target);
	pthread_exit(NULL);
	return NULL;
}

void target_buildlist_for_pthread(Target_t *list, int8_t ptr_max)
{
	if (!list) return;
	if (ptr_max <= 1) {    /* 单线程 */
		target_buildlist(list);
		return;
	}

	int8_t count = 0, i;
	Target_t *ptr_target[ptr_max] = {};
	pthread_t ptrs[ptr_max] = {};
	for (Target_t *p = list; p; p = p->next) {
		if (p->type != TY_NORM) continue;
		while (count >= ptr_max) {
			for (i = 0; i < ptr_max; i++) {
				if (!ptrs[i] || !ptr_target[i]) continue;
				if (ptr_target[i]->status == TS_WORKING) continue;
				pthread_join(ptrs[i], NULL);
				ptrs[i] = 0;
				ptr_target[i] = NULL;
				count--;
			}
			usleep(1e3);
		}
		for (i = 0; ptrs[i] && i < ptr_max; i++);
		// target_build(p);
		pthread_create(&ptrs[i], NULL, target_build_for_pthread, p);
		ptr_target[i] = p;
		count++;
	}
	while (count > 0) {
		for (i = 0; i < ptr_max; i++) {
			if (!ptrs[i] || !ptr_target[i]) continue;
			if (ptr_target[i]->status == TS_WORKING) continue;
			pthread_join(ptrs[i], NULL);
			ptrs[i] = 0;
			ptr_target[i] = NULL;
			count--;
		}
		usleep(1e3);
	}
}

/**
 * @brief 打印任务列表
 *
 * @param list 列表本身
 * @param mode 模式
 * 0: TS_NOCHECK (ON)
 * 1: TS_WORKING
 * 2: TS_SUCCESS
 * 3: TS_FAILD   (ON)
 * 4: TY_NORM    (ON)
 * 5: TY_PHONY   (ON)
 * 6: TY_DEP     (ON)
 * 7: 仅有已更新项目
 */
void target_printlist(Target_t *list, uint8_t mode)
{
	if (!list) return;
	mode ^= 0b01111001;    /* 切换默认模式 */
	static const char *statusstr[] = {
		[TS_NOCHECK] = "",
		[TS_WORKING] = "\e[33m<WORKING>\e[0m",
		[TS_SUCCESS] = "\e[32m<DONE>\e[0m",
		[TS_FAILD] = "\e[31m<FAILD>\e[0m",
	};
	static const char *typestr[] = {
		[TY_NORM] = "",
		[TY_PHONY] = "\e[2m(PHONY)\e[0m",
		[TY_DEP] = "\e[2m(DEP)\e[0m",
	};
	for (Target_t *p = list; p; p = p->next) {
		if (!(mode&(1<<p->status) && mode&(1<<(p->type+4)))) continue;
		if (mode&(1<<7) && !p->isupdated) continue;
		printf("[\e[2m%p\e[0m] %s%s'\e[32m%.*s\e[0m'",
		       p, statusstr[p->status%countof(statusstr)],
		       typestr[p->type%countof(typestr)],
		       (int)p->name.len, p->name.p);
		if (p->depend_len > 0) printf(" <- {");
		for (size_t i = 0; i < p->depend_len; i++) {
			if (!p->dependencies[i]) continue;
			// printf("[%p]%s,", p->dependencies[i], p->dependencies[i]->name.p);
			printf("\e[33m%s\e[0m%s", p->dependencies[i]->name.p,
			       i+1 >= p->depend_len ? "" : ", ");
		}
		if (p->depend_len > 0) printf("}");
		if (p->build) printf(" <- func<\e[2m%p\e[0m>", p->build);
		printf("\n");
	}
}

