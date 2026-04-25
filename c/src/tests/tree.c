/**
 * @file        tree.c
 * @author      u0_a221
 * @date        2026-04-25
 * @brief       做个简易文件树程序
 */

#include <linux/limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <errno.h>
#include <dirent.h>
#include <limits.h>

char *dirtypes[][2] = {
	[DT_UNKNOWN]
		  = {"未  知", "31"},
	[DT_FIFO] = {"管  道", "37"},
	[DT_CHR]  = {"块设备", "33"},
	[DT_DIR]  = {"文件夹", "34"},
	[DT_BLK]  = {"存  储", "33"},
	[DT_REG]  = {"文  件", "37"},
	[DT_LNK]  = {"链  接", "36"},
	[DT_SOCK] = {"套接字", "35"},
	[DT_WHT]  = {"啥玩意", "44"},
};
int typeslen = sizeof(dirtypes)/sizeof(dirtypes[0]);

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
			if (!strcmp(out, "..") || str_end_with(out, "/..")) {
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
	printf("<<< %s\n>>> %s\n", path, out);
	return NULL;
	int size = strlen(out)+1;
	char *p = malloc(size);
	strlcpy(p, out, size);
	return p;
}

void listdir(char *dirname)
{
	if (!dirname) return;
	DIR *dp = opendir(dirname);
	if (!dp) {
		fprintf(stderr, "ERROR 无法打开文件夹:%s\n", dirname);
		fprintf(stderr, "ERROR 错误信息: %s\n", strerror(errno));
		return;
	}
	printf("\033[1;33m%s :\033[0m\n", dirname);
	struct dirent *dp_item = NULL;
	for (;;) {
		if ((dp_item = readdir(dp)) == NULL) break;
		uint8_t type = dp_item->d_type;
		if (type >= typeslen) type = DT_UNKNOWN;
		printf("%6s  ||  \033[1;%sm%s\033[0m\n",
		       dirtypes[type][0], dirtypes[type][1],
		       dp_item->d_name);
	}
	closedir(dp);
	printf("\033[1;36m==================\033[0m\n");
}

void tree(char *dirname, int level)
{
	if (!dirname) return;
	DIR *dp = opendir(dirname);
	if (!dp) {
		fprintf(stderr, "ERROR 无法打开文件夹:%s\n", dirname);
		fprintf(stderr, "ERROR 错误信息: %s\n", strerror(errno));
		return;
	}
	printf("%s\n", dirname);
	struct dirent *dp_item = NULL;
	for (;;) {
		if ((dp_item = readdir(dp)) == NULL) break;
		/*uint8_t type = dp_item->d_type;*/
		/*if (type >= typeslen) type = DT_UNKNOWN;*/
		printf("%s\n", dp_item->d_name);
	}
	closedir(dp);
	return;
}

int main(int argc, char *argv[])
{
	normalize_path(__FILE__);
	normalize_path(__FILE_NAME__);
	normalize_path("../../../README.org");
	normalize_path("../aa../../README.org");
	normalize_path("../aa../../..//.sb./..//README.org");
	normalize_path(".");
	normalize_path("./");
	normalize_path("./.");
	normalize_path("././");
	normalize_path("");
	return 0;
	tree("./", 0);
	return 0;
	if (argc == 1) {
		listdir("./");
		return 0;
	}
	while (--argc && ++argv) {
		listdir(*argv);
	}
	return 0;
}
