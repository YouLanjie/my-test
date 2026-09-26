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
#include <time.h>
// #include <string.h>
#include <unistd.h>
#include <dirent.h>
#include <errno.h>

Target_t *target_create(SV_t name)
{
	if (name.len == 0 || !name.p) return NULL;
	Target_t *target = malloc(sizeof(*target));
	if (!target) return NULL;
	*target = (Target_t){
		.time_outoftime = 60,
	};
	sva_from_sv(&target->name, name);
	return target;
}

void target_free(Target_t *target)
{
	if (!target) return;
	sva_free(&target->name);
	sva_free(&target->log);
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
	while (list && sv_cmp(sv_from_sva(&list->name), name) != 0) {
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

static double get_nowtime()
{
	struct timespec t = {};
	clock_gettime(CLOCK_MONOTONIC, &t);
	return t.tv_sec + t.tv_nsec/1e9;
}

static void secnanosleep(double sec)
{
	struct timespec rqt = {
		.tv_sec = sec,
		.tv_nsec = ((sec-(long)sec)*1e9),
	}, remain = {};
	while (nanosleep(&rqt, &remain) == -1
	       && errno == EINTR
	       && (rqt=remain,true));
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
	target->time_start = get_nowtime();
	for (size_t i = 0; i < target->depend_len; i++) {
		target_build(target->dependencies[i]);
		switch (target->dependencies[i]->status) {
		case TS_WORKING:
			need_wait = true;
			break;
		case TS_SKIP:
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
			secnanosleep(0.01);
			if (get_nowtime()-target->time_start > target->time_outoftime) {
				target->status = TS_FAILD;
				return;
			}
			need_wait = false;
		}
	}
	target->time_start = get_nowtime();
	target->time_stop = target->time_start;
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
	target->time_stop = get_nowtime();
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

static size_t target_get_subdeps(Target_t *target)
{
	if (!target) return 0;
	if (!target->dependencies) return 1;
	size_t count = 1;
	for (size_t i = 0; i < target->depend_len; i++) {
		count += target_get_subdeps(target->dependencies[i]);
	}
	return count;
}

struct target_subdeps_t {
	Target_t *target;
	int64_t subdeps;
};

static int target_subdeps_cmp(const void *ts1, const void *ts2)
{
	if (!ts1 || !ts2) return 0;
	return ((struct target_subdeps_t*)ts1)->subdeps - ((struct target_subdeps_t*)ts2)->subdeps;
}

Target_t *target_sort_by_subdeps(Target_t *list)
{
	if (!list) return NULL;
	size_t len = 0;
	for (Target_t *target = list; target; target = target->next) len++;
	if (len == 0) return list;
	struct target_subdeps_t *target_list = malloc(len*sizeof(*target_list));
	if (!target_list) return list;
	size_t i = 0;
	for (Target_t *target = list; target; target = target->next) {
		target_list[i].target = target;
		target_list[i].subdeps = target_get_subdeps(target);
		i++;
	}
	qsort(target_list, len, sizeof(*target_list), target_subdeps_cmp);
	for (i = 0; i < len; i++) {
		target_list[i].target->prev = i>0 ? target_list[i-1].target : NULL;
		target_list[i].target->next = i+1<len ? target_list[i+1].target : NULL;
	}
	list = target_list[0].target;
	free(target_list);
	return list;
}

void *target_build_for_pthread(void *target)
{
	if (!target) return NULL;
	target_build(target);
	pthread_exit(NULL);
	return NULL;
}

static inline void wait_jobs(Target_t *list, Target_t *ptr_target[], pthread_t ptrs[],
			     int ptr_max, int wait_num, bool print_process,
			     double *t0)
{
	if (!ptr_target || !ptrs) return;
	int count = 0, i = 0;
	for (; i < ptr_max; i++)
		if (ptrs[i]) count++;
	if (count < wait_num) return;
	double t1;
	while (count && count >= wait_num) {
		for (i = 0; count && i < ptr_max; i++) {
			if (!ptrs[i] || !ptr_target[i]) continue;
			if (ptr_target[i]->status == TS_WORKING
			    || ptr_target[i]->status == TS_NOCHECK) continue;
			pthread_join(ptrs[i], NULL);
			ptrs[i] = 0;
			ptr_target[i] = NULL;
			count--;
		}
		secnanosleep(0.01);
		if (!t0) continue;
		if (print_process && (t1 = get_nowtime()) > *t0 + 1) {
			*t0 = t1;
			printf("==== 当前任务列 ====\n");
			target_printlist(list, 0b11011);
		}
	}
}

void target_buildlist_for_pthread(Target_t *list, int8_t ptr_max, int8_t print_process)
{
	if (!list) return;
	if (ptr_max <= 1) {    /* 单线程 */
		target_buildlist(list);
		return;
	}

	int i;
	Target_t *ptr_target[ptr_max] = {};
	pthread_t ptrs[ptr_max] = {};
	double t0 = get_nowtime();
	for (Target_t *p = list; p; p = p->next) {
		if (p->type != TY_NORM && p->type != TY_DEP) continue;
		if (p->type == TY_DEP && !p->build) continue;
		wait_jobs(list, ptr_target, ptrs, ptr_max, ptr_max, print_process, &t0);
		for (i = 0; i < ptr_max && ptrs[i]; i++);
		if (i >= ptr_max) continue;
		// target_build(p);
		pthread_create(&ptrs[i], NULL, target_build_for_pthread, p);
		ptr_target[i] = p;
	}
	wait_jobs(list, ptr_target, ptrs, ptr_max, 0, print_process, &t0);
}

void target_printlist(Target_t *list, uint16_t mode)
{
	if (!list) return;
	mode ^= 0b011111001;    /* 切换默认模式 */
	static const char *statusstr[] = {
		[TS_NOCHECK] = "",
		[TS_WORKING] = "\e[33m<WORKING>\e[0m",
		[TS_SUCCESS] = "\e[32m<DONE>\e[0m",
		[TS_FAILD] = "\e[31m<FAILD>\e[0m",
		[TS_SKIP] = "\e[33m<SKIP>\e[0m",
	};
	static const char *typestr[] = {
		[TY_NORM] = "",
		[TY_PHONY] = "\e[2m(PHONY)\e[0m",
		[TY_DEP] = "\e[2m(DEP)\e[0m",
	};
	char pointer_buf[16] = {};
	for (Target_t *p = list; p; p = p->next) {
		if (!(mode&(1<<p->status) && mode&(1<<(p->type+5)))) continue;
		if (mode&(1<<8) && !p->isupdated) continue;
		snprintf(pointer_buf, sizeof(pointer_buf), "%p", p);
		printf("[\e[2m%.4s..%.4s\e[0m] %s%s'\e[32m%.*s\e[0m'",
		       pointer_buf, pointer_buf+12-4, statusstr[p->status%countof(statusstr)],
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
		if (mode&(1<<9) && p->build) printf(" <- func<\e[2m%p\e[0m>", p->build);
		if (p->time_stop - p->time_start > 0.01)
			printf(" (took %.3gs)", p->time_stop - p->time_start);
		printf("\n");
		if (p->status != TS_WORKING && p->progress != TS_FAILD)
			continue;
		if (p->status == TS_WORKING) {
			const double progres = p->progress > 1
				? 1 : (p->progress < 0 ? 0 : p->progress);
			if (progres == 0) continue;
			printf("    \e[2m[%-20.*s] %6.2f%%\e[0m\n",
			       (int)(progres*20),
			       "#####################",
			       progres);
			continue;
		}
		if (!p->log.p || !p->log.len) continue;
		SV_t log = sv_from_sva(&p->log);
		SV_t line;
		while (sv_forline(&line, &log)) {
			printf("    LOG> %.*s\n", (int)line.len, line.p);
		}
	}
}

Target_t *target_fordir(Target_t *list, char *cwd, SV_t dirname,
			bool (*rule)(SV_t d_name, uint8_t d_type),
			Target_t *(*action)(Target_t *list, SV_t full_path))
{
	if (!action) return list;
	if (!cwd) cwd = "./";

	Path_t path = {0};
	path_join(sva_from_cstr(&path, cwd), dirname);

	if (!path.p) return list;
	DIR *dp = opendir(path.p);
	if (!dp) {
		if (path_get_st(path).isfile && rule && rule(path_basename(sv_from_sva(&path)), DT_REG)) {
			list = action(list, sv_from_sva(&path));
		} else {
			fprintf(stderr, "ERROR 无法打开文件夹:%s\n", path.p);
			fprintf(stderr, "ERROR 错误信息: %s\n", strerror(errno));
		}
		sva_free(&path);
		return list;
	}
	Path_t tmp = {0};
	struct dirent *dp_item = NULL;
	for (;;) {
		if ((dp_item = readdir(dp)) == NULL) break;
		if (rule && rule(sv_from_cstr(dp_item->d_name), dp_item->d_type) == false) continue;
		if (dp_item->d_type == DT_DIR) {
			list = target_fordir(list, path.p, sv_from_cstr(dp_item->d_name), rule, action);
			continue;
		}
		path_join(sva_from_sv(&tmp, sv_from_sva(&path)),
			  sv_from_cstr(dp_item->d_name));
		list = action(list, sv_from_sva(&tmp));

	}
	closedir(dp);
	sva_free(&tmp);
	sva_free(&path);
	return list;
}

