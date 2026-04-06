/**
 * @file        randlinkf.c
 * @author      u0_a221
 * @date        2026-04-05
 * @brief       将一个目录里的符合后缀的文件链接到目标位置
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <time.h>
#include <assert.h>
#include <limits.h>
#include <dirent.h>
#include <errno.h>
#include <sys/stat.h>

char *OUTDIR = "./randomlist/";
char *SUFFIX = ".mp3";

char *get_randommd5()
{
	static char filename[125];
	char t[] = "0123456789abcdef";
	int i = 0;
	for (; i < 32; i++) {
		filename[i] = t[rand() % (sizeof(t)-1)];
	}
	filename[i] = '\0';
	return filename;
}

void fordir(char *dirname)
{
	assert(dirname && *dirname);
	DIR *dp = opendir(dirname);
	if (!dp) {
		fprintf(stderr, "ERROR 无法打开文件夹:%s\n", dirname);
		fprintf(stderr, "ERROR 错误信息: %s\n", strerror(errno));
		return;
	}
	struct dirent *dp_item = NULL;
	char path[PATH_MAX];
	for (;;) {
		if ((dp_item = readdir(dp)) == NULL) break;
		if (dp_item->d_type != DT_REG && dp_item->d_type != DT_DIR)
			continue;
		char *d_name = dp_item->d_name;
		snprintf(path, sizeof(path), "%s/%s", dirname, d_name);
		if (dp_item->d_type == DT_DIR) {
			if (!strcmp(d_name, ".") && !strcmp(d_name, ".."))
				fordir(path);
			continue;
		}
		int offset = strlen(d_name) - 1 - strlen(SUFFIX);
		if (offset <= 0) continue;
		if (!strcmp(SUFFIX, d_name+offset)) continue;
		char path2[PASS_MAX];
		snprintf(path2, sizeof(path2), "%s/%s%s",
			 OUTDIR, get_randommd5(), SUFFIX);
		printf("LINK '%s' to '%s'\n", path, path2);
		if (symlink(path, path2)) {
			fprintf(stderr, "WARN 创建链接'%s'时出错: %s\n",
				path2 ,strerror(errno));
		}
	}
	closedir(dp);
}

int main(int argc, char *argv[])
{
	if (argc < 2 || argc > 3) {
		fprintf(stderr, "Usage: %s <music_dir> [out_dir]\n", argv[0]);
		return 1;
	}
	char *musicdir = argv[1];
	if (argc == 3) OUTDIR = argv[2];
	printf("INFO 输入目录：%s\n", musicdir);
	printf("INFO 输出目录：%s\n", OUTDIR);

	struct stat st;
	if (stat(OUTDIR, &st) && mkdir(OUTDIR, 0755)) {
		perror("mkdir");
		return 1;
	}
	if(S_ISREG(st.st_mode)) {
		fprintf(stderr, "ERROR 不是文件夹:%s\n", OUTDIR);
		return 1;
	}
	srand(time(NULL));
	fordir(musicdir);
	return 0;
}
