/**
 * @file        termux_check_mem.c
 * @author      u0_a221
 * @date        2026-06-27
 * @brief       专门用于在termux下检查通报内存占用
 */

#include <stddef.h>
#include <stdio.h>
#include <stdbool.h>
#include <stdcountof.h>
#include <sys/wait.h>
#include <unistd.h>
#include <errno.h>
#include <dirent.h>
#include <limits.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <time.h>
#include <sys/sysinfo.h>
#include <signal.h>

typedef struct {
	size_t rss;    // kb
	size_t swap;   // kb
	size_t total;  // kb
	char comm[NAME_MAX];
	int pid;
} Process_t;

/* 用于qsort排序比较 */
int proc_cmp(const void *p1, const void *p2)
{
	if (!p1 || !p2) return 0;
	int cmp = ((Process_t*)p2)->total - ((Process_t*)p1)->total;
	if (cmp) return cmp;
	return ((Process_t*)p2)->pid - ((Process_t*)p1)->pid;
}


/* 获取命令名称（无参数|带参数） */
void read_comm(Process_t *proc)
{
	if (!proc) return;
	sprintf(proc->comm, "/proc/%d/comm", proc->pid);

	FILE *fp = fopen(proc->comm, "r");
	if (!fp) return;
	fgets(proc->comm, sizeof(proc->comm), fp);
	char *p = strrchr(proc->comm, '\n');
	if (p) *p = '\0';
	fclose(fp);
}

/* 读取MemAvailable(而不是MemFree) */
size_t read_meminfo()
{
	FILE *fp = fopen("/proc/meminfo", "r");
	if (!fp) return 0;
	char buffer[PATH_MAX] = "";
	size_t mem_available = 0;
	while (fgets(buffer, sizeof(buffer), fp) != NULL) {
		if (sscanf(buffer, "MemAvailable: %ld ", &mem_available) == 1)
			break;
	}
	fclose(fp);
	return mem_available;
}

/* 读取每个进程的状态信息 */
void read_status(Process_t *proc)
{
	if (!proc) return;
	sprintf(proc->comm, "/proc/%d/status", proc->pid);
	FILE *fp = fopen(proc->comm, "r");
	if (!fp) return;
	proc->rss = proc->swap = 0;
	while (fgets(proc->comm, sizeof(proc->comm), fp) != NULL) {
		if (sscanf(proc->comm, "VmRSS: %ld ", &proc->rss));
		else sscanf(proc->comm, "VmSwap: %ld ", &proc->swap);
		if (proc->rss && proc->swap) break;
	}
	fclose(fp);
	proc->total = proc->rss + proc->swap;
	return;
}

/* 遍历读取进程列表(自动置空+排序) */
int update(Process_t *proc_list, int proc_len)
{
	if (!proc_list || proc_len <= 0) return -1;
	memset(proc_list, 0, sizeof(*proc_list)*proc_len);

	static const char *proc_path = "/proc/";
	DIR *dp = opendir(proc_path);
	if (!dp) {
		fprintf(stderr, "ERROR 无法打开文件夹:%s\n", proc_path);
		fprintf(stderr, "ERROR 错误信息: %s\n", strerror(errno));
		return -2;
	}
	struct dirent *dp_item = NULL;
	char path[PATH_MAX] = "";
	Process_t proc = {0};
	size_t minid;
	int count = 0;
	int idx = 0;
	while ((dp_item = readdir(dp)) != NULL) {
		if (dp_item->d_type != DT_DIR) continue;
		strncpy(path, dp_item->d_name, sizeof(path));
		for (char *p = path; p && *p && (proc.pid = isdigit(*p)) != 0; p++);
		if (proc.pid == 0) continue;
		proc.pid = atoi(path);

		read_status(&proc);

		if (idx < proc_len) {
			proc_list[idx] = proc;
			idx++;
			count++;    /* 统计有效数字 */
			continue;
		}

		/* 找出最小值 */
		for (idx = 0, minid = 0; idx < proc_len; idx++) {
			if (proc_list[idx].total < proc_list[minid].total)
				minid = idx;
		}
		/* 取代最小值 */
		if (proc_list[minid].total < proc.total)
			proc_list[minid] = proc;
		idx = proc_len;
	}
	closedir(dp);
	qsort(proc_list, proc_len, sizeof(proc_list[0]), proc_cmp);
	idx = 0;
	while (idx < proc_len) {
		if (proc_list[idx].pid <= 0) break;
		/* 减少读无用进程名 */
		read_comm(proc_list+idx);
		idx++;
	}
	return count;
}

// #define DEBUG
void send_notification(char *title, char *content)
{
	pid_t pid = fork();
	if (pid != 0) {
		fprintf(stderr, "[INFO] 启动通知子进程(PID%d)\n", pid);
		return;
	}
	char *notificatior = "termux-notification";
	execvp(notificatior, (char*[]){
	       notificatior,
	       "--title", title,
	       "--content", content,
	       "--priority", "max",
	       "--vibrate", "500",
	       "--sound", NULL});

#ifndef DEBUG
	notificatior = "dunstify";
	execvp(notificatior, (char*[]){
	       notificatior,
	       "-u", "CRITICAL",
	       title, content, NULL});
#endif

	fprintf(stderr, "[WARN] 启动通知进程错误，未找到可用命令\n");
	exit(127);    /* command not found */
}

int monitor(struct timespec delay, size_t len, bool auto_kill)
{
	const double MINPMEM = 80.;           // 内存占比大于该值为active
	const double MINAVAIMEM = 250*1024;   // 可用内存小于该值为active
	const double MINMEM = 250*1024;       // 处理逻辑所需最低内存峰值
	const double MINSTOPMEM = 600*1024;   // 考虑自动暂停内存占用超过该值的进程
	const double MINKILLMEM = 3700*1024;  // 考虑自动KILL掉内存占用超过该值的进程

	if (access("/data/data/com.termux/files/home/", R_OK|W_OK) != 0) {
		if (auto_kill) {
			fprintf(stderr, "[WARN] 在非termux环境下无法使用SIGKILL\n");
		}
		auto_kill = false;
		// 非termux环境下禁用自动杀死进程
	}

	char buffer_title[PATH_MAX] = {0};
	char buffer_content[PATH_MAX] = {0};
	Process_t proc_list[len];
	struct sysinfo info;
	sysinfo(&info);
	double total_mem = 0;
	double pmem = 0;
	time_t timep;
	struct tm *timetmp;
	int hold = 0;
	size_t freeram = 0;
	bool active = 0;

	memset(proc_list, 0, sizeof(proc_list));
	fprintf(stderr, "[INFO] 自动监视模式工作\n");
	for (;; nanosleep(&delay, NULL)) {
		sysinfo(&info);
		freeram = read_meminfo();  // kb
		total_mem = (info.totalram+info.totalswap)/1024.;
		pmem = 100*(1-(freeram+info.freeswap)/1024./total_mem);

#ifndef DEBUG
		// pmem超过阈值 或者 可用内存过少
		active = pmem >= MINPMEM || freeram <= MINAVAIMEM;
		if (active) update(proc_list, countof(proc_list));
#else
		update(proc_list, countof(proc_list));
#endif

		waitpid((pid_t)-1, NULL, WNOHANG);    /* 不阻塞回收子进程(通知) */
#ifndef DEBUG
		if (!active || proc_list[0].total < MINMEM) {
			if (hold) fprintf(stderr, "[INFO] 脱离临界情况\n");
			hold = 0;
			continue;
		}
		if (hold && hold <= 40 && proc_list[0].total < MINSTOPMEM && hold++) continue;
#endif
		time(&timep);
		timetmp = localtime(&timep);
		strftime(buffer_title, sizeof(buffer_title), "%Y.%m.%d %H:%M:%S", timetmp);
		sprintf(buffer_content, "[%s] 当前内存使用率 %.1f%% (阈值 %.1f%%)",
			buffer_title, pmem, MINPMEM);
		for (size_t i = 0; i < countof(proc_list); i++) {
			char *p = "";
#ifndef DEBUG
			if (auto_kill && proc_list[i].total >= MINKILLMEM) {
				hold = 114514;
				kill(proc_list[i].pid, SIGKILL);
				p = " [SIGKILL]";
			} else if (proc_list[i].total >= MINSTOPMEM && freeram <= MINAVAIMEM) {
				kill(proc_list[i].pid, SIGSTOP);
				p = " [SIGSTOP]";
			}
#endif
			sprintf(buffer_title, "\n[%4.1f%%] %.1lfMB (pid:%d) %s%s",
			       100.*proc_list[i].total/total_mem, proc_list[i].total/1024.,
			       proc_list[i].pid, proc_list[i].comm,
			       p);
			strncat(buffer_content, buffer_title, sizeof(buffer_content)-strlen(buffer_content)-1);
		}
		sprintf(buffer_title, "[WARN] 内存占用警告 (%.1f%%)", pmem);

#ifndef DEBUG
		if (hold && hold <= 40 && hold++) continue;
		hold = 1;
#endif

		printf("===============================\n");
		printf("[TITLE] %s\n", buffer_title);
		printf("[CONTENT] %s\n", buffer_content);

		send_notification(buffer_title, buffer_content);
	}
	while (wait(NULL) != -1);
}

void print_top(Process_t proc_list[], size_t len)
{
	len = update(proc_list, len);

	Process_t proc_summary = {0};
	struct sysinfo info;
	sysinfo(&info);
	const double total_mem = (info.totalram+info.totalswap)/1024.;

	printf("Top %ld:\n", len);
	for (size_t i = 0; i < len; i++) {
		proc_summary.rss += proc_list[i].rss;
		proc_summary.swap += proc_list[i].swap;
		printf("[%4.1f%%] %.1lfMB (rss:%.1f ,swap:%.1f) (pid:%d) %s\n",
		       100.*proc_list[i].total/total_mem, proc_list[i].total/1024.,
		       proc_list[i].rss/1024., proc_list[i].swap/1024.,
		       proc_list[i].pid, proc_list[i].comm);
	}
	proc_summary.total += proc_summary.rss+proc_summary.swap;
	printf("SUMMARY:\n"
	       "- RSS(%.1f/%.1f [%.1f%%])\n"
	       "- SWAP(%.1f/%.1f [%.1f%%])\n"
	       "- Total(%.1f/%.1f [%.1f%%])\n",
	       proc_summary.rss/1024., info.totalram/1024./1024., proc_summary.rss*1024./info.totalram*100,
	       proc_summary.swap/1024., info.totalswap/1024./1024., proc_summary.swap*1024./info.totalswap*100,
	       proc_summary.total/1024., total_mem/1024., proc_summary.total*1024./(info.totalram+info.totalswap)*100
	       );
}

int main(int argc, char *argv[])
{
	size_t len = 0;
	double delay = 2;
	int ch = 0;
	bool flg_watch = false;
	bool flg_monitor = false;
	bool flg_kill = false;
	while ((ch = getopt(argc, argv, "ht:n:wmk")) != -1) {
		switch (ch) {
		case '?':
		case 'h':
			printf("Usage: %s [OPTIONS]\n"
			       "Options:\n"
			       "    -h       显示帮助\n"
			       "    -t <NUM> 打印的进程次数\n"
			       "    -n <SEC> 检查间隔\n"
			       "    -w       类似于watch，自动重复运行\n"
			       "    -m       类似-w,但只在超限时打印并发送通知\n"
			       "    -k       指定-m时自动暂停/杀死超限进程（仅termux）\n"
			       "    -s       在rish(shizuku)里运行\n",
			       argc>0?argv[0]:"memcheck");
			return 0;
			break;
		case 't':
			sscanf(optarg, "%zu", &len);
			break;
		case 'w':
			flg_watch = true;
			break;
		case 'm':
			flg_monitor = true;
			break;
		case 'k':
			flg_kill = true;
			break;
		case 'n':
			sscanf(optarg, "%lf", &delay);
			if (delay < 0.001) delay = 0.1;
			else if (delay > 1000) delay = 1000;
			break;
		}
	}
	struct timespec tm = {
		.tv_sec = delay,
		.tv_nsec = (delay-(int)delay)*1e9,
	};
	if (flg_monitor) {
		if (len == 0 || len > 50) len = 3;
		return monitor(tm, len, flg_kill);
	}
	if (len == 0) len = 20;
	else if (len > 2000) len = 2000;
	Process_t proc_list[len];
	memset(proc_list, 0, sizeof(proc_list));
	if (!flg_watch) {
		print_top(proc_list, countof(proc_list));
		return 0;
	}
	for (;;) {
		print_top(proc_list, countof(proc_list));
		printf("[INFO] 每%g秒一轮\n", delay);
		nanosleep(&tm, NULL);
		printf("\e[2J\e[H");
	}
	return 0;
}

