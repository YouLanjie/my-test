/**
 * @file        tree.c
 * @author      u0_a221
 * @date        2026-04-25
 * @brief       做个简易文件树程序
 */

#include <assert.h>
#include <linux/limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <errno.h>
#include <dirent.h>
#include <limits.h>

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

char *normalize_path(char *path)
{
	if (!path) return NULL;
	int i1 = 0, i2 = 0;
	char out[PATH_MAX] = "";
	memset(out, 0, PATH_MAX);
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
	int size = strlen(out)+1;
	char *p = malloc(size);
	strlcpy(p, out, size);
	return p;
}

void tree(char *cwd, char *dirname, int level)
{
	if (!dirname) return;
	if (!cwd) cwd = "./";
	char path[PATH_MAX] = "", *p;
	sprintf(path, "%s/%s", cwd, dirname);
	p = normalize_path(path);
	assert(p);
	strlcpy(path, p, PATH_MAX);
	free(p);

	DIR *dp = opendir(path);
	if (!dp) {
		fprintf(stderr, "ERROR 无法打开文件夹:%s\n", dirname);
		fprintf(stderr, "ERROR 错误信息: %s\n", strerror(errno));
		return;
	}
	if (level <= 1) printf("%s\n", dirname);
	struct dirent *dp_item = NULL;
	int i = 0;
	for (;;) {
		if ((dp_item = readdir(dp)) == NULL) break;
		uint8_t type = dp_item->d_type;
		if (strcmp(dp_item->d_name, "..")==0 || strcmp(dp_item->d_name, ".")==0) {
			continue;
		}
		for (i = 0; i < level; i++) printf("|   ");
		printf("%s\n", dp_item->d_name);
		if (type == DT_DIR) {
			tree(path, dp_item->d_name, level+1);
		}
	}
	closedir(dp);
	return;
}

int main(int argc, char *argv[])
{
	if (argc == 1) {
		tree(NULL, "./", 1);
		return 0;
	}
	while (--argc && ++argv) {
		tree(NULL, *argv, 1);
		/*listdir(*argv);*/
	}
	return 0;
}
