/**
 * @file        a.c
 * @author      Chglish
 * @date        2026-07-30
 * @brief       测试子进程time
 */

#include <limits.h>
#include <signal.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/resource.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <time.h>
#include <unistd.h>

#define IOVARLIST \
	IOVAR(rchar) \
	IOVAR(wchar) \
	IOVAR(syscr) \
	IOVAR(syscw) \
	IOVAR(read_bytes) \
	IOVAR(write_bytes)
#define IOVAR(v) size_t v;
typedef struct {
	IOVARLIST
} IOStat_t;
#undef IOVAR

void read_io(FILE *fp, IOStat_t *st)
{
	if (!fp || !st) return;
	char buffer[125] = "";
	if (!fp) return;
	*st = (IOStat_t){};
	while (fgets(buffer, sizeof(buffer), fp) != NULL) {
#define IOVAR(v) if (sscanf(buffer, #v": %zu ", &st->v) == 1); else
		IOVARLIST {};
#undef IOVAR
	}
	fseek(fp, 0L, SEEK_SET);
	clearerr(fp);
	return;
}

size_t read_status(FILE *fp, size_t max_mem)
{
	if (!fp) return 0;
	char buffer[125] = "";
	size_t rss = 0, swap = 0;
	int count = 0;
	while (fgets(buffer, sizeof(buffer), fp)) {
		if (sscanf(buffer, "VmRSS: %zu ", &rss) == 1) count++;
		else if (sscanf(buffer, "VmSwap: %zu ", &swap) == 1) count++;
		if (count == 2) break;
	}
	/* 刷新文件内容否则读到的内容不会变 */
	fflush(fp);
	fseek(fp, 0L, SEEK_SET);
	clearerr(fp);
	return rss+swap > max_mem ? rss+swap : max_mem;
}

#define NSEC (1000000000LL)    /* 1e9 */

static inline struct timespec timespec_norm(struct timespec t)
{
	if (t.tv_nsec > NSEC) {
		t.tv_sec += t.tv_nsec / NSEC;
		t.tv_nsec = t.tv_nsec % NSEC;
	} else if (t.tv_nsec < 0) {
		t.tv_sec += t.tv_nsec / NSEC - 1;
		t.tv_nsec = NSEC - t.tv_nsec%NSEC;
	}
	return t;
}

static inline struct timespec timespec_sub(struct timespec t, struct timespec t2)
{
	t2 = timespec_norm(t2);
	t.tv_sec -= t2.tv_sec;
	t.tv_nsec -= t2.tv_nsec;
	t = timespec_norm(t);
	return t;
}

bool is_pid_exited(pid_t pid)
{
	siginfo_t info = {};
	if (waitid(P_PID, pid, &info, WNOHANG|WEXITED|WNOWAIT))
		return true;
	if (info.si_pid != 0 && (info.si_code==CLD_EXITED||info.si_code==CLD_KILLED))
		return true;
	return false;
}

static int sig_count = 0;
static pid_t child_pid = 0;

void sig_handle(int sig)
{
	sig_count++;
	printf("[GOT SIGNAL(%d) -> PID %d] (%d/5)\n", sig, child_pid, sig_count);
	kill(child_pid, sig);
}

int main(int argc, char *argv[])
{
	if (argc <= 1) {
		fprintf(stderr, "Usage: test_time command ...\n");
		return 1;
	}
	pid_t pid = fork();
	struct timespec t1 = {};
	clock_gettime(CLOCK_MONOTONIC, &t1);
	if (!pid) {
		/* 重定向子进程stderr输出到stdout */
		dup2(STDOUT_FILENO, STDERR_FILENO);
		execvp(argv[1], argv+1);
		exit(127);
	}
	child_pid = pid;
	signal(SIGINT, sig_handle);
	signal(SIGTERM, sig_handle);

	FILE *fp_io = NULL, *fp_st = NULL;
	char path[PATH_MAX] = "";
	sprintf(path, "/proc/%d/io", pid);
	fp_io = fopen(path, "rb");
	sprintf(path, "/proc/%d/status", pid);
	fp_st = fopen(path, "rb");

	if (!fp_io || !fp_st) {
		if (fp_io) fclose(fp_io);
		if (fp_st) fclose(fp_st);
		fprintf(stderr, "[ERROR] 进程状态文件打开错误，等待子进程返回\n");
		while (wait(NULL) != -1);
		return 2;
	}

	struct timespec tm = {.tv_sec = 0, .tv_nsec = 0.01e9};
	size_t max_mem = 0;
	IOStat_t iost = {};
	while (!is_pid_exited(pid)) {
		max_mem = read_status(fp_st, max_mem);
		read_io(fp_io, &iost);
		if (is_pid_exited(pid)) break;
		if (sig_count >= 5) break;
		nanosleep(&tm, NULL);
	}

	struct timespec t2 = {};
	clock_gettime(CLOCK_MONOTONIC, &t2);
	t1 = timespec_sub(t2, t1);
	fclose(fp_st);
	fclose(fp_io);

	struct rusage st = {};
	int ret = 0;
	wait4(pid, &ret, 0, &st);

	if (WEXITSTATUS(ret) == 127) {
		fprintf(stderr, "[ERROR] COMMAND NOT FOUND\n");
	}
	fprintf(stderr, "[EXEC] '%s'\n", argv[1]);
	fprintf(stderr, "[TIME] 用户态:%ld.%06lds  内核态:%ld.%06lds  真实时间:%ld.%09lds  CPU:%.1f%%\n",
		st.ru_utime.tv_sec, st.ru_utime.tv_usec,
		st.ru_stime.tv_sec, st.ru_stime.tv_usec,
		t1.tv_sec, t1.tv_nsec,
		(st.ru_utime.tv_sec+st.ru_stime.tv_sec+(st.ru_utime.tv_usec+st.ru_stime.tv_usec)/1e6)/(t1.tv_sec+t1.tv_nsec/1e9)*100
		);
	fprintf(stderr, "[INFO] 内存峰值:%.2fMB  返回值:%d\n", max_mem/1024., WEXITSTATUS(ret));
	fprintf(stderr, "[IO] 输入:%.2fMB/%zu次  输出:%.2fMB/%zu次\n",
		iost.rchar/1024./1024, iost.syscr, iost.wchar/1024./1024, iost.syscw);
	fprintf(stderr, "[IO] 读盘:%.2fMB  写盘:%.2fMB\n",
		iost.read_bytes/1024./1024, iost.write_bytes/1024./1024);
	// printf("[SIZE] MaxRSS: %.2fMB  InBlock: %ld  OutBlock: %ld\n",
	//        st.ru_maxrss/1024., st.ru_inblock, st.ru_oublock);
	// #define IOVAR(v) printf("[INFO] %s: %ld\n", #v, iost.v);
	// 	IOVARLIST
	// #undef IOVAR
	return 0;
}

