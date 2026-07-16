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
	if (sv_end_with(sv_from_sva(path), sv_from_lstr("/."))) {    /* 跳过单独'.'充数的 */
		// path->p[path->len-1] = 0;
		path->len--;
		return;
	}
	while (sv_end_with(sv_from_sva(path), sv_from_lstr("/.."))) {    /* 撤回一个目录层级 */
		path->len -= 3;
		const SV_t sv = sv_from_sva(path);
		if (sv_cmp(sv, sv_from_cstr("..")) || sv_end_with(sv, sv_from_lstr("/.."))) {
			path->len+=3;    /* 如果上一级目录也是..则取消撤回并添加新字符 */
			break;
		} else if (sv_cmp(sv, sv_from_cstr("."))  || sv_end_with(sv, sv_from_lstr("/."))) {
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

int path_mkdir(SV_t path, int mode)
{
	if (!path.p || path.len == 0) return -1;
	Path_t sva = {};
	path_normalize(sva_from_sv(&sva, path));
	char *p = NULL;
	while (access(sva.p, F_OK) != 0 && (p = strrchr(sva.p, '/')) && p != sva.p) {
		*p = '\0';
	}
	int ret = 0;
	size_t len = 0;
	Path_st_t st;
	do {
		st = path_get_st(sva);
		if (st.isexist && !st.isdir) {
			ret = -2;
			break;
		}
		if (!st.isexist) {
			ret = mkdir(sva.p, mode);
			if (ret) break;
		}
		if ((len = strlen(sva.p)) < sva.len) sva.p[len] = '/';
	} while (strlen(sva.p) < sva.len);
	return ret;
}

SVA_t *path_readfile(SV_t path, SVA_t *dest, size_t maxsize)
{
	if (!path.p || !path.len || !dest) return NULL;
	SVA_t file = {};
	sva_from_sv(&file, path);
	FILE *fp = fopen(file.p, "r");
	if (!fp) return NULL;

	size_t size = 0;
	fseek(fp, 0L, SEEK_END);
	size = ftell(fp);
	fseek(fp, 0L, SEEK_SET);
	do {
		if (size <= 0 || size >= UINT64_MAX || size+1 == 0) break;
		if (size > maxsize) size = maxsize;
		sva_free(dest);
		dest->capacity = size+1;
		dest->p = malloc(dest->capacity);
		if (!dest->p) {
			dest->capacity = 0;
			perror("The file is too big");
			break;
		}
		dest->len = size;
		fread(dest->p, 1, size, fp);
		if (dest->len < dest->capacity) dest->p[dest->len] = '\0';
	} while (0);
	fclose(fp);
	return dest;
}
