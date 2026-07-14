/**
 * @file        test_depend_list.c
 * @author      Chglish
 * @date        2026-07-14
 * @brief       测试“依赖列表”功能
 */

#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <dirent.h>
#include "../../include/path.h"

typedef struct Target_t {
	SVA_t name;
	enum {TS_NOCHECK = 0, TS_SUCCESS, TS_FAILD} status;
	double time;
	bool (*build)(struct Target_t*);
	size_t depend_len;
	struct Target_t **dependencies;
	struct Target_t *prev;
	struct Target_t *next;
} Target_t;

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
		target->depend_len++;
		target->dependencies = reallocarray(target->dependencies, target->depend_len, sizeof(*target->dependencies));
	}
	if (!target->dependencies) {
		target->depend_len = 0;
		return;
	}
	target->dependencies[target->depend_len-1] = dependency;
}




#define BUILD_DIR  "./.build/"
#define BIN_DIR    "./bin/"
/* 将路径变为 BUILD_DIR/xxx_xxx_xxx.o */
static Path_t *path_hander_obj_replace(Path_t* path)
{
	if (!path || !path->p) return NULL;
	for (size_t n = 0; n < path->len; n++) if (path->p[n] == '/') path->p[n] = '_';
	SV_t sv = path_basename(sv_from_sva(path));
	while (sv.len > 0 && (sv.p[0] == '.' || sv.p[0] == '_')) sv_chop_left(&sv, 1);
	SVA_t new = {0};
	sva_from_sva(&new, path);
	sva_strcpy(path, sva_sprintfcat(path_join(sva_from_cstr(&new, BUILD_DIR), sv), ".o"));
	sva_free(&new);
	return path;
}
/* 将路径变为 BIN_DIR/xxx */
static Path_t *path_hander_elf(Path_t* path)
{
	if (!path || !path->p) return NULL;
	SVA_t basename = {0};
	sva_from_sv(&basename, path_basename(sv_from_sva(path)));
	if (basename.p == NULL) return NULL;
	sva_sprintf(path, "%s/%.*s", BIN_DIR, (int)basename.len, basename.p);
	sva_free(&basename);
	path_normalize(path);
	return path;
}

static Target_t *fordir(Target_t *list, char *cwd, char *dirname)
{
	if (!dirname) return list;
	if (!cwd) cwd = "./";
	if (sv_begin_with(sv_from_cstr(dirname), "lib")) return list;

	Path_t path = {0};
	Path_t obj = {0};
	Path_t tmp = {0};
	Target_t *target, *sub_target;
	path_join(sva_from_cstr(&path, cwd), sv_from_cstr(dirname));

	DIR *dp = opendir(path.p);
	if (!dp) return list;
	/*if (level <= 1) printf("%s\n", dirname);*/
	struct dirent *dp_item = NULL;
	for (;;) {
		if ((dp_item = readdir(dp)) == NULL) break;
		uint8_t type = dp_item->d_type;
		if (strcmp(dp_item->d_name, "..")==0 || strcmp(dp_item->d_name, ".")==0) {
			continue;
		}
		/*printf("%s\n", dp_item->d_name);*/
		if (type == DT_DIR) {
			list = fordir(list, path.p, dp_item->d_name);
		}
		if (type != DT_REG || !sv_end_with(sv_from_cstr(dp_item->d_name),".c"))
			continue;
		path_join(sva_from_sv(&obj, sv_from_sva(&path)),
			  sv_from_cstr(dp_item->d_name));

		target = target_create(sv_from_sva(&obj));
		if (!list) list = target;
		target_append(list, target);
		sva_strcpy(&tmp, &obj);

		path_hander_obj_replace(&tmp);
		sub_target = target_create(sv_from_sva(&tmp));
		target_append(list, sub_target);
		target_depend_append(sub_target, target);  // obj 依赖 .c
		target = sub_target;

		path_hander_elf(&obj);
		sub_target = target_create(sv_from_sva(&obj));
		target_append(list, sub_target);
		target_depend_append(sub_target, target);  // elf 依赖 .o
	}
	closedir(dp);
	sva_free(&tmp);
	sva_free(&obj);
	sva_free(&path);
	return list;
}

int main(void)
{
	Target_t *list = fordir(NULL, NULL, "./");

	/* 打印 */
	for (Target_t *p = list; p; p = p->next) {
		printf("[%p] '%.*s' <- {", p, (int)p->name.len, p->name.p);
		for (size_t i = 0; i < p->depend_len; i++) {
			if (!p->dependencies[i]) continue;
			printf("%p,", p->dependencies[i]);
		}
		printf("}\n");
	}

	Target_t *next;
	while (list) {
		next = list->next;
		target_free(list);
		list = next;
	}
	return 0;
}

