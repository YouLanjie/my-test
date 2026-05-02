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

typedef struct {
	char path[PATH_MAX];
} Path_t;

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

Path_t path_from_cstr(const char *s)
{
	Path_t path;
	strlcpy(path.path, s, sizeof(path.path));
	return path;
}

Path_t path_from_sv(SV_t sv)
{
	Path_t path = {0};
	strncpy(path.path, sv.p, sv.len);
	return path;
}

SV_t path_basename(Path_t path)
{
	SV_t s = {strlen(path.path), path.path}, last = {0, NULL};
	while (s.len != 0) last = sv_chop_by_delim(&s, '/');
	s = last;
	last.len = 0;
	while (last.len == 0 && s.len != 0) last = sv_chop_by_delim(&s, '.');
	return last;
}

Path_t path_normalize(const char *path)
{
	if (!path) return (Path_t){0};
	Path_t ret = {0};
	char *p = ret.path;
	int i = 0;
	for (; *path && i < PATH_MAX; path++) {
		if (i && p[i-1] == '/' && *path == '/') continue;
		if (i && str_end_with(p, "/.") && *path == '/') {
			p[i-1] = 0;
			i--;
			continue;
		}
		if (i > 2 && str_end_with(p, "/..") && *path == '/') {
			p[i-3] = 0;
			if (!strcmp(p, "..") || str_end_with(p, "/..") ||
			    !strcmp(p, ".") || str_end_with(p, "/.")) {
				p[i-3] = '/';
				p[i] = *path;
				i++;
				continue;
			}
			for (;i >= 0 && p[i] != '/'; i--) p[i] = 0;
			i++;
			continue;
		}
		p[i] = *path;
		i++;
	}
	while (p[0] != '/' && str_end_with(p, "/.")) p[strlen(p)-1] = 0;
	if (i == 0) {
		p[i] = '.';
		p[i+1] = 0;
	}
	return ret;
}

Path_t path_join(Path_t path, SV_t s)
{
	int size = strlen(path.path);
	char sep = '/';
	if (size <= 0 || path.path[size-1] == sep) sep = 0; 
	if (size+s.len+(sep!=0) >= sizeof(path.path)) return path;
	if (sep) {
		path.path[size] = sep;
		path.path[size+1] = 0;
	}
	strncat(path.path, s.p, s.len);
	return path_normalize(path.path);
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
	if (stat(f.path, &st.st) == -1) {
		st.isexist = false;
		return st;
	}
	st.isdir = S_ISDIR(st.st.st_mode);
	st.isfile = S_ISREG(st.st.st_mode);
	st.islink = S_ISLNK(st.st.st_mode);
	return st;
}

