/**
 * @file        path.c
 * @author      u0_a221
 * @date        2026-04-26
 * @brief       路径处理函数
 */

#include <stdbool.h>
#include <string.h>
#include <limits.h>
#include <sys/stat.h>
#include "../include/string_view.h"

typedef SVA_t Path_t;

bool str_start_with(const char *s, const char *pat)
{
	if (!s || !pat) return false;
	if (strlen(s) < strlen(pat)) return false;
	return !strncmp(s, pat, strlen(pat));
}

bool str_end_with(const char *s, const char *pat)
{
	if (!s || !pat) return false;
	if (strlen(s) < strlen(pat)) return false;
	int size = strlen(pat);
	return !strncmp(s+strlen(s)-size, pat, size);
}

#define path_from_cstr sva_from_cstr
#define path_from_sv sva_from_sv

SV_t path_basename(SV_t path)
{
	SV_t last = {0, NULL};
	while (path.len != 0) last = sv_chop_by_delim(&path, '/');
	path = last;
	last.len = 0;
	while (last.len == 0 && path.len != 0) last = sv_chop_by_delim(&path, '.');
	return last;
}

/*
Path_t path_normalize(const char *path)
{
	if (!path) return (Path_t){0};
	Path_t ret = {0};
	char *p = ret.path;
	int i = 0;
 * */
int path_normalize(Path_t *ret, SV_t path)
{
	if (!ret || !path.p) return -1;
	Path_t tmp = *ret;
	ret->p = NULL;
	sva_create(ret);
	for (; *path.p && path.len; sv_chop_left(&path, 1)) {
		if (ret->len >= ret->capacity) sva_double(ret);
		if (*path.p != '/' || ret->len == 0) {    /* 不进入下级啥也不管 */
			ret->p[ret->len] = *path.p;
			ret->len++;
			continue;
		}
		/* 进入下级且前文不为空 */
		if (ret->p[ret->len-1] == '/') continue;    /* 忽略重复的 */
		if (str_end_with(ret->p, "/.")) {    /* 跳过单独'.'充数的 */
			ret->p[ret->len-1] = 0;
			ret->len--;
			continue;
		}
		if (str_end_with(ret->p, "/..")) {    /* 撤回一个目录层级 */
			ret->p[ret->len-3] = 0;    /* '/' -> '\0' */
			if (!strcmp(ret->p, "..") || str_end_with(ret->p, "/..") ||
			    !strcmp(ret->p, ".") || str_end_with(ret->p, "/.")) {
				ret->p[ret->len-3] = '/';    /* 如果上一级目录也是..则取消一次撤回 */
				ret->p[ret->len] = *path.p;
				ret->len++;
				continue;
			}
			for (;ret->len >= 0 && ret->p[ret->len] != '/'; ret->len--) ret->p[ret->len] = 0;
			ret->len++;
			continue;
		}
		ret->p[ret->len] = *path.p;
		ret->len++;
	}
	while (ret->p[0] != '/' && str_end_with(ret->p, "/.")) ret->p[strlen(ret->p)-1] = 0;
	if (ret->len == 0) {
		ret->p[ret->len] = '.';
		ret->p[ret->len+1] = 0;
	}
	sva_smallest(ret);
	sva_free(&tmp);    /* 清理原内存空间 */
	return 0;
}

int path_join(Path_t *ret, SV_t path, SV_t child)
{
	const static char sep = '/';
	Path_t tmp = {0};
	if (child.len && child.p[0] == '/') sva_from_sv(&tmp, child);
	else sva_sprintf(&tmp, "%.*s%c%.*s",(int)path.len, path.p,
			 sep, (int)child.len, child.p);
	path_normalize(ret, sv_from_sva(&tmp));
	sva_free(&tmp);
	return 0;
}

typedef struct {
	struct stat st;
	bool isexist;
	bool islink;
	bool isdir;
	bool isfile;
} Path_st_t;

/* fl的文件是否比f的新 */
Path_st_t path_get_st(Path_t f)
{
	Path_st_t st = {0};
	if (stat(f.p, &st.st) == -1) {
		st.isexist = false;
		return st;
	}
	st.isdir = S_ISDIR(st.st.st_mode);
	st.isfile = S_ISREG(st.st.st_mode);
	st.islink = S_ISLNK(st.st.st_mode);
	return st;
}

