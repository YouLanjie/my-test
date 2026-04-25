/**
 * @file        path.c
 * @author      u0_a221
 * @date        2026-04-26
 * @brief       路径处理函数
 */

#include <stdbool.h>
#include <string.h>
#include <limits.h>
//#include "path.h"

bool str_start_with(char *s, char *pat)
{
	if (!s || !pat) return false;
	if (strlen(s) < strlen(pat)) return false;
	return !strncmp(s, pat, strlen(pat));
}

bool str_end_with(char *s, char *pat)
{
	if (!s || !pat) return false;
	if (strlen(s) < strlen(pat)) return false;
	int size = strlen(pat);
	return !strncmp(s+strlen(s)-size, pat, size);
}

char *normalize_path(char *path, char *buf, size_t size)
{
	if (!path) return NULL;
	int i1 = 0, i2 = 0;
	char out[PATH_MAX] = {0};
	for (; path[i1] && i2 < PATH_MAX; i1++) {
		if (i2 && out[i2-1] == '/' && path[i1] == '/') continue;
		if (i2 && str_end_with(out, "/.") && path[i1] == '/') {
			out[i2-1] = 0;
			i2--;
			continue;
		}
		if (i2 > 2 && str_end_with(out, "/..") && path[i1] == '/') {
			out[i2-3] = 0;
			if (!strcmp(out, "..") || str_end_with(out, "/..") ||
			    !strcmp(out, ".") || str_end_with(out, "/.")) {
				out[i2-3] = '/';
				out[i2] = path[i1];
				i2++;
				continue;
			}
			for (;i2 >= 0 && out[i2] != '/'; i2--) out[i2] = 0;
			i2++;
			continue;
		}
		out[i2] = path[i1];
		i2++;
	}
	while (out[0] != '/' && str_end_with(out, "/.")) out[strlen(out)-1] = 0;
	if (i2 == 0) {
		out[i2] = '.';
		out[i2+1] = 0;
	}
	strlcpy(buf, out, size);
	return buf;
}


