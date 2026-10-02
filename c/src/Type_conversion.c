#include <dirent.h>
#include <stdio.h>
#include <string.h>
#include <inttypes.h>
#include <stdcountof.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/wait.h>
#include <unistd.h>
#include <poll.h>
#include "../include/target_list.h"

#include <fcntl.h>
#include <stdlib.h>
// #include <errno.h>

static int read_ffmpeg_output(Target_t *target, int pid, int subout, int suberr)
{
	if (!target || subout < 0 || suberr < 0) return -1;
	struct pollfd fds[2] = {
		{ .fd = subout, .events = POLLIN, },
		{ .fd = suberr, .events = POLLIN, },
	};
	char buffer[2*PATH_MAX];
	/* 单位：微秒μs */
	uint64_t duration_total = 0,
		 duration_now = 0;
	bool active = true;
	int ret = 0;
	while (active && poll(fds, countof(fds), 0.1e3) != -1) {
		for (size_t i = 0; i < countof(fds); i++) {
			if (!(fds[i].revents & POLLIN)) continue;
			ssize_t size = read(fds[i].fd, buffer, sizeof(buffer));
			if (size <= 0) continue;
			if (i == 1) {
				/* STDERR转日志 */
				sva_append(&target->log, (SV_t){.p=buffer,.len=size});
				continue;
			}
			if ((size_t)size >= sizeof(buffer)) buffer[--size] = 0;
			SV_t line = {}, left = (SV_t){.p=buffer,.len=size};
			char c = 0;
			while (sv_forline(&line, &left)) {
				if (sscanf(line.p, "progress=en%c", &c) == 1 && c == 'd') {
					active = false;
					break;
				}
				if (sscanf(line.p, "out_time_us=%"SCNu64, &duration_now) < 1) continue;
				if (!duration_total) continue;
				target->progress = (double)duration_now/duration_total;
			}
		}
		int stat = waitpid(pid, &ret, WNOHANG);
		if (stat < 0 || (stat > 0 && !WIFEXITED(ret))) break;
		if (duration_total) continue;
		SV_t line = {}, left = sv_from_sva(&target->log);
		int h = 0, m = 0, s = 0, cs = 0;
		for (int i = 0; i < 50 && sv_forline(&line, &left); i++) {
			if (sscanf(line.p, " Duration: %d:%2d:%2d.%2d,", &h, &m, &s, &cs) < 4) continue;
			duration_total = (h*3600+m*60+s)*1000000L+cs*10000L;
			break;
		}
	}
	return WIFEXITED(ret) ? WEXITSTATUS(ret) : UINT8_MAX+WTERMSIG(ret);
}

static SVA_t *argv2str(SVA_t *dest, char *argv[])
{
	if (!dest || !argv) return NULL;
	SVA_t tmp = {};
	for (size_t idx = 0; argv[idx]; idx++) {
		if (idx) sva_append(dest, sv_from_lstr(" "));
		if (!strpbrk(argv[idx], " \t\r\n\\\"'`!?#$%^&|(){}[]<>~") && strlen(argv[idx]) != 0) {
			sva_sprintfcat(dest, "%s", argv[idx]);
			continue;
		}
		sva_from_cstr(&tmp, argv[idx]);
		sva_replace(&tmp, sv_from_lstr("'"), sv_from_lstr("'\\''"));
		sva_sprintfcat(dest, "'%.*s'", (int)tmp.len, tmp.p);
	}
	sva_free(&tmp);
	return dest;
}

static bool build_ffmpeg(Target_t *target)
{
	if (!target || !target->depend_len
	    || !target->dependencies || !target->dependencies[0]) return false;
	SV_t name = sv_from_sva(&target->name);

	char *argv[64] = {
		"ffmpeg", "-hide_banner", "-nostdin", "-y", "-progress", "pipe:1", "-nostats",
		"-i", target->dependencies[0]->name.p, NULL,
	};
	size_t argc = 0;
	for (argc = 0; argc < countof(argv) && argv[argc]; argc++);
	if (sv_case_end_with(name, sv_from_lstr(".3gp"))) {
		char *options[] = {
			"-r", "12", "-b:v", "400k", "-s", "352x288",
			"-ab", "12.2k", "-ac", "1", "-ar", "8000"
		};
		for (size_t i = 0; i < countof(options); i++)
			argv[argc++] = options[i];
	}
	argv[argc++] = target->name.p;
	argv[argc++] = NULL;

	/* STDOUT 0:读端 1:写端 | STDERR 2:读端 3:写端 */
	int pipefd[4] = {-1, -1, -1, -1};
	int ret = false;
	if (pipe(pipefd) == -1) {
		perror("pipe#1");
		goto EXIT_THREAD_AND_CLEANUP;
	}
	if (pipe(pipefd+2) == -1) {
		perror("pipe#2");
		goto EXIT_THREAD_AND_CLEANUP;
	}

	SVA_t command = {};
	argv2str(&command, argv);
	printf("[\e[32mRUN\e[0m] %s\n", command.p);

	pid_t pid = fork();
	if (!pid) {
		dup2(pipefd[1], STDOUT_FILENO);
		dup2(pipefd[3], STDERR_FILENO);
		for (size_t i = 0; i < countof(pipefd); i++) close(pipefd[i]);
		execvp("ffmpeg", argv);
		perror("execvp");
		exit(1);
	}
	close(pipefd[1]);
	pipefd[1] = -1;
	close(pipefd[3]);
	pipefd[3] = -1;

	ret = read_ffmpeg_output(target, pid, pipefd[0], pipefd[2]);
	// waitpid(pid, &ret, 0);
	// ret = WIFEXITED(ret) && WEXITSTATUS(ret) == 0;
	if (ret != 0) {
		sva_sprintfcat(&target->log, "\n[INFO] 退出状态：code %d\n", ret);
		sva_sprintfcat(&target->log, "[COMMAND] %.*s\n", (int)command.len, command.p);
		printf("[\e[31mFAILD\e[0m] %s\n", target->name.p);
		remove(target->name.p);
		ret = false;
	} else ret = true;
	sva_free(&command);

EXIT_THREAD_AND_CLEANUP:
	for (size_t i = 0; i < countof(pipefd); i++) {
		if (pipefd[i] >= 0) close(pipefd[i]);
	}
	return ret;
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
	if (d_type != DT_REG && d_type != DT_LNK)
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

	Path_t tmp = {};
	sva_from_sv(&tmp, full_path);
	path_normalize(&tmp);
	if (!path_get_st_follow(sv_from_sva(&tmp)).isfile) {
		sva_free(&tmp);
		return NULL;
	}
	target_src = target_get_or_create(list, sv_from_sva(&tmp));
	if (target_src) target_src->type = TY_DEP;
	if (!list) list = target_src;

	SV_t stem = path_stemname(full_path);
	if (!stem.len) stem = path_basename(full_path);
	sva_sprintfcat(path_join(sva_from_sva(&tmp, &output_dir), stem),
		       ".%.*s", (int)output_type.len, output_type.p);
	target_output = target_get_or_create(list, sv_from_sva(&tmp));
	if (target_output) target_output->build = build_ffmpeg;
	target_depend_append(target_output, target_src);
	sva_free(&tmp);
	return list;
}

int main(int argc, char *argv[])
{
	SV_t exe_name = path_basename(sv_from_cstr(argv[0]));
	SV_t inputdir = {};
	int opt;
	bool print_list = false;
	int proc_limit = sysconf(_SC_NPROCESSORS_ONLN) / 2;
	while ((opt = getopt(argc, argv, "ht:d:l:p")) != -1) {
		switch (opt) {
		case 't':
			output_type = path_basename(sv_from_cstr(optarg));
			break;
		case 'd':
			inputdir = sv_from_cstr(optarg);
			break;
		case 'l':
			sscanf(optarg, "%d", &proc_limit);
			if (proc_limit <= 0) proc_limit = 1;
			break;
		case 'p':
			print_list = true;
			break;
		case '?':
		case 'h':
		default:
			printf("本程序基于ffmpeg，转换格式时需要安装ffmpeg\n"
			       "Usage: %.*s [OPTIONS]\n"
			       "OPTIONS:\n"
			       "    -t <FMT>  指定目标格式\n"
			       "    -d <PATH> 输入文件夹\n"
			       "    -l <NUM>  设置任务并行上限\n"
			       "    -p        结束后打印任务列表\n"
			       "    -h        打印帮助\n",
			       (int)exe_name.len, exe_name.p);
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

	printf("[INFO] 并行上限: %d\n", proc_limit);
	if (proc_limit > 1) target_buildlist_for_pthread(list, proc_limit, true);
	else target_buildlist(list);

	int total = 0, completed = 0, faild = 0, skiped = 0;
	for (Target_t *p = list; p; p = p->next) {
		if (p->type != TY_NORM) continue;
		total++;
		if (p->time_stop == p->time_start) {
			skiped++;
			p->status = TS_SKIP;
		} else if (p->status == TS_SUCCESS) completed++;
		else if (p->status == TS_FAILD) faild++;
	}
	printf("[INFO] 所有任务执行完成，全 %d 个，%d 完成，%d 跳过，%d 失败\n",
	       total, completed, skiped, faild);
	if (print_list) target_printlist(list, 0b110|1<<7);
	else target_printlist(list, 0);
	target_freelist(list);
	sva_free(&output_dir);
	return (total>0 && faild==total) ? 2 : (faild<=0 ? 0 : 1);
}

