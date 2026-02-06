/*
 *   Copyright (C) 2026 u0_a221
 *
 *   文件名称：protectFiles.c
 *   创 建 者：u0_a221
 *   创建日期：2026年01月24日
 *   描    述：byd windows,连个fork()都没有。。。
 *
 */


#include <limits.h>
#include "tools.h"

#ifdef _WIN32
#define IS_WIN (1)
#else
#define IS_WIN (0)
#endif

#define MAXLIST 1024*4
static const char SEP = IS_WIN ? '\\' : '/';
static void *PTABLE[MAXLIST] = {NULL};
static char *filelist[MAXLIST] = {NULL};

void *l_malloc(long size)
{
	void *p = malloc(size);
	memset(p, 0, size);
	int i = 0;
	for (;i < MAXLIST && PTABLE[i];i++);
	PTABLE[i] = p;
	return p;
}

void l_free(void **p)
{
	if (!p || !*p) {
		for (int i = 0; i < MAXLIST;i++)
			if (PTABLE[i]) free(PTABLE[i]);
		return;
	}
	int i = 0;
	for (;i < MAXLIST && PTABLE[i] != *p;i++);
	if (i>0 && PTABLE[i] == *p) {
		free(PTABLE[i]);
		PTABLE[i] = NULL;
		*p = NULL;
	}
}

int l_mkdir(const char *p, int mode)
{
#ifdef _WIN32
	return mkdir(p);
#else
	return mkdir(p, mode);
#endif
}

char *get_father(char *path)
{
	if (!path) return NULL;
	int len = 0;
	for (int i=0;*(path+i);i++)
		if (*(path+i) == SEP) len = i;
	if (!len) return NULL;
	char *dir = l_malloc((len+1)*sizeof(char));
	strncpy(dir, path, len);
	return dir;
}

int mkndir(char *p)
{
	char *p2 = l_malloc((strlen(p)+1)*sizeof(char));
	for (int i=0;*(p+i);i++) {
		if (*(p+i) != SEP) continue;
		if (!i) continue;
		strncpy(p2, p, i);
		if (!access(p2, F_OK)) continue;
		l_mkdir(p2, 0755);
		printf("MKDIR: %s\n", p2);
	}
	if (strcmp(p, p2)) {
		l_mkdir(p, 0755);
		printf("MKDIR: %s\n", p);
	}
	return 0;
}

int self_protected(char *file_path)
{
	if (!file_path)
		return 0;
	char path[PATH_MAX] = "";
	printf("PATH_MAX:%d\n", PATH_MAX);
	strcpy(path, file_path);
#ifdef __linux__
	if(!realpath(file_path, path))
		return 3;
#else
	if(!_fullpath(path, file_path, PATH_MAX))
		return 3;
#endif
	char *dir = get_father(path);
	if (!dir) return 1;
	printf("DIR: %s\n", dir);
	printf("FILE: %s\n", path);

	FILE *fp = fopen(path, "rb");
	long len = 0;
	if (!fp)
		return 0;
	fseek(fp, 0L, SEEK_END);
	len = ftell(fp);
	fseek(fp, 0L, SEEK_SET);

	char *content = NULL;
	content = l_malloc((len+1)*sizeof(char));
	fread(content, len, sizeof(char), fp);
	fclose(fp);

	int count = 0;
	struct stat st;
	stat(path, &st);
	while (1) {
		if (!access(path, F_OK)) {
			sleep(1);
			continue;
		}
		if (access(dir, F_OK)) mkndir(dir);
		fp = fopen(path, "wb");
		if (!fp) return 2;
		fwrite(content, len, sizeof(char), fp);
		fclose(fp);
		chmod(path, st.st_mode);
		count++;
		printf("RECOVER: [%s] (%d)\n", path, count);
	}
	return 0;
}

int read_config(char *file_path)
{
	char *dir = get_father(file_path);
	char *cfg_f = l_malloc(strlen(dir)+14);
	sprintf(cfg_f, "%s%c%s", dir, SEP, "filelist.txt");
	l_free((void**)&dir);
	FILE *fp = fopen(cfg_f, "r");
	if (!fp) return 1;
	char p[PATH_MAX] = "";
	for (int i = 0; i<MAXLIST && fgets(p, PATH_MAX, fp) != NULL;i++) {
		if (p[strlen(p)-1] == '\n') p[strlen(p)-1] = '\0';
		filelist[i] = l_malloc(strlen(dir)+strlen(p)+2);
		sprintf(filelist[i], "%s/%s", dir, p);
		printf("ADD FILE:%s\n", filelist[i]);
	}
	fclose(fp);
	return 0;
}

int main(int argc, char *argv[])
{
	char path[PATH_MAX] = "";
	strcpy(path, argv[0]);
#ifdef __linux__
	sprintf(path, "/proc/%d/exe", getpid());
	if (!readlink(path, path, PATH_MAX))
		strcpy(path, argv[0]);
#endif
	read_config(path);
	for (int i = 0; i < MAXLIST; i++) {
		if (!filelist[i]) continue;
		int pid = fork();
		if (!pid) {
			printf("pid: %d\n", getpid());
			printf("RETURN CODE: %d\n", self_protected(filelist[i]));
			l_free(NULL);
			return 0;
		}
		usleep(1000000/20);
	}
	int pid = fork();
	if (!pid) {
		printf("pid: %d\n", getpid());
		printf("RETURN CODE: %d\n", self_protected(path));
		l_free(NULL);
		return 0;
	}
	l_free(NULL);
	return 0;
}

