#include <dirent.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/wait.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include "../include/target_list.h"


static bool build_ffmpeg(Target_t *target)
{
	if (!target || !target->depend_len
	    || !target->dependencies || !target->dependencies[0]) return false;
	int i = 0;
	for (Target_t *p = target; p; p = p->prev) i++;
	Path_t logfile = {};
	SV_t name = sv_from_sva(&target->name);
	SV_t basename = path_stemname(name);
	if (!basename.len) basename = path_basename(name);
	sva_sprintfcat(sva_from_sv(&logfile, path_father(name)),
		       "/Log%03d_%.*s.txt", i, (int)basename.len, basename.p);
	path_normalize(&logfile);

	i = open(logfile.p, O_CREAT|O_WRONLY, S_IRUSR|S_IWUSR|S_IRGRP|S_IROTH);
	if (i == -1) {
		sva_from_cstr(&target->log, strerror(errno));
		return false;
	}
	char *argv1[] = {
		"ffmpeg", "-hide_banner", "-y",
		"-i", target->dependencies[0]->name.p,
		target->name.p,
		NULL,
	};
	/* 3gp参数 */
	char *argv2[] = {
		"ffmpeg", "-hide_banner", "-y",
		"-i", target->dependencies[0]->name.p,
		"-r", "12", "-b:v", "400k", "-s", "352x288",
		"-ab", "12.2k", "-ac", "1", "-ar", "8000",
		target->name.p,
		NULL,
	};
	char **argv = sv_case_end_with(name, sv_from_lstr(".3gp"))
		? argv2 : argv1;

	printf("[\e[32mRUN\e[0m] ");
	for (size_t idx = 0; argv[idx]; idx++) {
		if (idx) printf(" ");
		printf("'%s'", argv[idx]);
	}
	printf("\n");

	pid_t pid = fork();
	if (!pid) {
		dup2(i, STDOUT_FILENO);
		dup2(i, STDERR_FILENO);
		// 奇怪，关掉stdin后就会发生奇怪的事情(ffmpeg运行失败等)
		// close(STDIN_FILENO);
		close(i);
		// ffmpeg -hide_banner -y -i "INPUT" {OPTION} "OUTPUT" > Log001_xxx.txt
		execvp("ffmpeg", argv);
		perror("execvp");
		exit(1);
	}
	close(i);

	int ret = 0;
	waitpid(pid, &ret, 0);
	if (WEXITSTATUS(ret) == 0) remove(logfile.p);
	else {
		printf("[\e[31mFAILD\e[0m] ");
		for (size_t idx = 0; argv[idx]; idx++) {
			if (idx) printf(" ");
			printf("'%s'", argv[idx]);
		}
		printf("\n");
		remove(target->name.p);
	}
	sva_free(&logfile);
	return WEXITSTATUS(ret) == 0;
}


static bool rule_fordir(SV_t d_name, uint8_t d_type)
{
	if (!d_name.p || !d_name.len) return false;
	/* 跳过特殊路径 */
	if (sv_cmp(d_name, sv_from_cstr("..")) == 0 ||
	    sv_cmp(d_name, sv_from_cstr(".")) == 0)
		return false;
	/* 跳过文件夹 */
	if (d_type == DT_DIR) return false;
	/* 跳过非文件 */
	if (d_type != DT_REG)
		return false;
	if (!path_suffixname(d_name).len) return false;
	return true;
}

static SV_t output_type = {};
static Path_t output_dir = {};

static Target_t *action_file(Target_t *list, SV_t full_path)
{
	if (!output_type.len || !output_dir.len) return list;
	Target_t *target_src, *target_output;

	target_src = target_get_or_create(list, full_path);
	if (target_src) target_src->type = TY_DEP;
	if (!list) list = target_src;

	SV_t stem = path_stemname(full_path);
	if (!stem.len) stem = path_basename(full_path);
	Path_t tmp = {};
	sva_sprintfcat(path_join(sva_from_sva(&tmp, &output_dir), stem), ".%.*s", (int)output_type.len, output_type.p);
	target_output = target_get_or_create(list, sv_from_sva(&tmp));
	if (target_output) target_output->build = build_ffmpeg;
	target_depend_append(target_output, target_src);
	sva_free(&tmp);
	return list;
}

int main(int argc, char *argv[])
{
	SV_t inputdir = {};
	int opt;
	while ((opt = getopt(argc, argv, "t:d:h")) != -1) {
		switch (opt) {
		case 't':
			output_type = path_basename(sv_from_cstr(optarg));
			break;
		case 'd':
			inputdir = sv_from_cstr(optarg);
			break;
		case '?':
		case 'h':
		default:
			printf("本程序基于ffmpeg，转换格式时需要安装ffmpeg\n参数：Type_conversion [-t <目标格式>] [-d <文件夹>] [-h]帮助\n");
			return 0;
			break;
		}
	}
	if (!inputdir.len || !output_type.len) {
		printf("[!] 未指定输入文件夹或输出文件类型\n");
		return 1;
	}
	path_join(path_from_sv(&output_dir, inputdir), sv_from_lstr("out/"));
	path_mkdir(sv_from_sva(&output_dir), 0777);
	Target_t *list = NULL;
	list = target_fordir(list, NULL, inputdir, rule_fordir, action_file);

	int cpus = sysconf(_SC_NPROCESSORS_ONLN);
	if (cpus > 0) target_buildlist_for_pthread(list, cpus);
	else target_buildlist(list);

	target_printlist(list, 0);
	sva_free(&output_dir);
	return 0;
}

