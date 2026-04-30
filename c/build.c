/**
 * @file        build.c
 * @author      u0_a221
 * @date        2026-04-26
 * @brief       构建脚本
 */

#include "include/string_view.h"
#include "lib/path.c"
#include "lib/string_view.c"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <dirent.h>
#include <limits.h>
#include <sys/stat.h>
#include <unistd.h>
#include <wait.h>

#include "config.c"

/* fl的文件是否比f的新 */
bool is_newer(char *f, int fl_len, char *fl[])
{
	if (!f || !fl) return false;
	struct stat st;
	if (stat(f, &st) == -1) return true;
	time_t t = st.st_mtim.tv_sec;
	for (int i = 0; i < fl_len && fl[i]; i++) {
		if (stat(fl[i], &st) == -1) return false;
		if (st.st_mtim.tv_sec > t) return true;
	}
	return false;
}

bool cmd_cflags_join(char *cmd, int len, char *source, int mode)
{
	if (!cmd || !source) return false;
	Path_t ret = {0};
	CFLAGS_t *flags = CFILEFLAGS; 
	char *p = NULL;
	for (uint64_t i = 0; i < ARRAY_LEN(CFILEFLAGS); i++) {
		p = ((char*[]){flags[i].libs, flags[i].comp, flags[i].link})[mode];
		if (!flags[i].filename || !p) continue;
		ret = normalize_path(flags[i].filename);
		if (strcmp(ret.path, source) != 0) continue;
		if (strcmp(p, "//NOEXE") == 0) return false;
		strlcat(cmd, " ", len);
		strlcat(cmd, p, len);
		return true;
	}
	return true;
}

bool build_obj(char *cwd, char *filename)
{
	if (!filename) return false;
	if (!cwd) cwd = "./";
	char cmd[5*PATH_MAX] = {0};
	SV_t basename = {0};
	Path_t source = {0},
	       obj = {0};

	sprintf(source.path, "%s/%s", cwd, filename);
	source = normalize_path(source.path);
	if ((basename = path_basename(source)).len == 0) return false;
	sprintf(obj.path, BUILD_DIR"%.*s.o", (int)basename.len, basename.p);
	if (!is_newer(obj.path, 1, (char*[]){source.path})) return true;

	sprintf(cmd, COMPILOR" "CCOMFLAGS" -c -o \"%s\" \"%s\"", obj.path, source.path);
	if (!cmd_cflags_join(cmd, sizeof(cmd), source.path, 1)) return false;
	printf("[RUN] %s\n", cmd);
	if (system(cmd)) {
		printf("[WARN] non-zero returned\n");
		return false;
	}
	return true;
}

void build_c(char *cwd, char *filename, char *bin_dir)
{
	if (!filename) return;
	if (!cwd) cwd = "./";
	char cmd[5*PATH_MAX] = {0};
	SV_t basename = {0};
	Path_t source = {0},
	       obj = {0},
	       bin = {0};

	sprintf(source.path, "%s/%s", cwd, filename);
	source = normalize_path(source.path);
	if ((basename = path_basename(source)).len == 0) return;
	sprintf(obj.path, BUILD_DIR"%.*s.o", (int)basename.len, basename.p);
	sprintf(bin.path, "%s%.*s", bin_dir?bin_dir:BIN_DIR, (int)basename.len, basename.p);
	if (!is_newer(bin.path, 1, (char*[]){obj.path})) return;

	sprintf(cmd, COMPILOR" "CLINKFLAGS" -o \"%s\" \"%s\"", bin.path, obj.path);
	if (!cmd_cflags_join(cmd, sizeof(cmd), source.path, 2)) return;
	printf("[RUN] %s\n", cmd);
	if (system(cmd)) {
		printf("[WARN] non-zero returned\n");
		printf("[WARN] (%s) %s\n", source.path, cmd);
	}
}

void build_libs()
{
	char cmd[5*PATH_MAX] = {0};
	Path_t path = {0};
	SV_t p = {0};
	for (uint64_t i = 0; i < ARRAY_LEN(CLIBS); i++) {
		if (!is_newer(CLIBS[i].libname, ARRAY_LEN(CLIBS[i].sources), CLIBS[i].sources))
			continue;
		sprintf(cmd, "ar rcs \"%s\" ", CLIBS[i].libname);
		uint64_t j = 0;
		for (j = 0; j < ARRAY_LEN(CLIBS[i].sources) && CLIBS[i].sources[j]; j++) {
			build_obj(NULL, CLIBS[i].sources[j]);

			strlcpy(path.path, CLIBS[i].sources[j], sizeof(path.path));
			if ((p = path_basename(path)).len == 0) continue;
			sprintf(path.path, BUILD_DIR"%.*s.o", (int)p.len, p.p);

			strlcat(cmd, " \"", sizeof(cmd));
			strlcat(cmd, path.path, sizeof(cmd));
			if(str_end_with(cmd, ".c")) cmd[strlen(cmd)] = 'o';
			strlcat(cmd, "\"", sizeof(cmd));
		}
		if (j == 0) continue;
		printf("[RUN] %s\n", cmd);
		system(cmd);
	}
}

int pcount = 0;

void fordir(char *cwd, char *dirname)
{
	if (!dirname) return;
	if (!cwd) cwd = "./";
	if (str_start_with(dirname, "lib")) return;

	Path_t path = {0};
	sprintf(path.path, "%s/%s", cwd, dirname);
	path = normalize_path(path.path);

	DIR *dp = opendir(path.path);
	if (!dp) {
		fprintf(stderr, "ERROR 无法打开文件夹:%s\n", dirname);
		fprintf(stderr, "ERROR 错误信息: %s\n", strerror(errno));
		return;
	}
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
			fordir(path.path, dp_item->d_name);
		}
		if (type != DT_REG || !str_end_with(dp_item->d_name,".c"))
			continue;
		while (pcount >= 8) if (wait(NULL) > 0) pcount--;
		pid_t pid = fork();
		if (pid == 0) {
			if (build_obj(path.path, dp_item->d_name))
				build_c(path.path, dp_item->d_name, NULL);
			exit(0);
		}
		pcount++;
	}
	closedir(dp);
	return;
}

int main(int argc, char *argv[])
{
	mkdir(BUILD_DIR, 0755);
	mkdir(BIN_DIR, 0755);
	build_libs();
	fordir(NULL, SOURCE_DIR);
	build_obj(NULL, __FILE__);
	build_c(NULL, __FILE__, "./");
	while(wait(NULL) > 0) usleep(0010000);
	return 0;
}

