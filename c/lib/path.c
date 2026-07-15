/**
 * @file        path.c
 * @author      u0_a221
 * @date        2026-04-26
 * @brief       路径处理函数
 */

#include "../include/path.h"

SV_t path_basename(SV_t path)
{
	SV_t left = {0, path.p};
	while (path.len != 0) left = sv_chop_by_delim(&path, '/');
	return left;
}

SV_t path_stemname(SV_t path)
{
	path = path_basename(path);
	size_t i = 0;
	while (i < path.len && path.p[path.len-i-1] != '.') i++;
	if (i >= path.len) i = 0;
	else i = path.len-i-1;
	path.len = i;
	return path;
}

SV_t path_suffixname(SV_t path)
{
	path = path_basename(path);
	size_t i = 0;
	while (i < path.len && path.p[path.len-i-1] != '.') i++;
	if (i >= path.len) i = path.len;
	else i = path.len-i-1;
	path.p += i;
	path.len -= i;
	return path;
}

SV_t path_father(SV_t path)
{
	while (path.len > 1 && path.p[path.len] == '/') path.len--;
	while (path.len && path.p[path.len-1] != '/') sv_chop_right(&path, 1);
	return path;
}

/* 专供normalize用的检查处理函数
 * c: 待添加的分隔符（'/'或'\0'） */
static inline void _path_tails_process(Path_t *path, char c)
{
	if (!path->p || path->p[path->len-1] == '/') return;    /* 忽略重复的 */
	if (sv_end_with(sv_from_sva(path), "/.")) {    /* 跳过单独'.'充数的 */
		// path->p[path->len-1] = 0;
		path->len--;
		return;
	}
	while (sv_end_with(sv_from_sva(path), "/..")) {    /* 撤回一个目录层级 */
		path->len -= 3;
		const SV_t sv = sv_from_sva(path);
		if (sv_cmp(sv, sv_from_cstr("..")) || sv_end_with(sv, "/..")) {
			path->len+=3;    /* 如果上一级目录也是..则取消撤回并添加新字符 */
			break;
		} else if (sv_cmp(sv, sv_from_cstr("."))  || sv_end_with(sv, "/.")) {
			/* 如果上一级是.则替换为.. */
			path->p[path->len] = '.';
			path->len+=1;
		}
		if (!path->len) path->len++;
		for (;path->len > 0 && path->p[path->len-1] != '/'; path->len--);
		path->p[path->len] = 0;
		return;
	}
	path->p[path->len] = c;
	if (c) path->len++;
}

Path_t * path_normalize(Path_t *path)
{
	if (!path || !path->capacity) return NULL;
	const int len = path->len;
	int i = 0;
	path->len = 0;
	for (; path->p && i < len; i++) {
		if (path->len >= path->capacity) sva_double(path);
		if (!path->p[i]) break;
		if (path->p[i] != '/' || path->len == 0) {    /* 不进入下级啥也不管 */
			path->p[path->len] = path->p[i];
			path->len++;
			continue;
		}
		/* 进入下级且前文不为空 */
		_path_tails_process(path, path->p[i]);
	}
	if (!path->p) return NULL;
	path->p[path->len] = 0;
	_path_tails_process(path, '\0');
	if (path->len == 0) sva_sprintf(path, "./");
	return path;
}

Path_t *path_join(Path_t *path, SV_t child)
{
	if (!path || !path->capacity || !path->p) return NULL;
	static const char sep = '/';
	if (child.len && child.p[0] == sep) sva_sprintf(path, "%.*s", (int)child.len, child.p);
	else sva_sprintfcat(path, "%c%.*s", sep, (int)child.len, child.p);
	return path_normalize(path);
}

/* fl的文件是否比f的新 */
Path_st_t path_get_st(Path_t f)
{
	Path_st_t st = {0};
	if (stat(f.p, &st.st) == -1) {
		st.isexist = false;
		return st;
	}
	st.isexist = true;
	st.isdir = S_ISDIR(st.st.st_mode);
	st.isfile = S_ISREG(st.st.st_mode);
	st.islink = S_ISLNK(st.st.st_mode);
	return st;
}

