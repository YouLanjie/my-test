/**
 * @file        termux_check_mem.c
 * @author      u0_a221
 * @date        2026-06-27
 * @brief       专门用于在termux下检查通报内存占用
 */

#include <bits/getopt.h>
#include <stddef.h>
#include <stdio.h>
#include <stdbool.h>
#include <stdcountof.h>
#include <unistd.h>
#include <errno.h>
#include <dirent.h>
#include <limits.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <time.h>
#include <sys/sysinfo.h>

typedef struct {
	size_t rss;    // kb
	size_t swap;   // kb
	size_t total;  // kb
	char comm[NAME_MAX];
	int pid;
} Process_t;

void read_comm(const char *path, Process_t *proc)
{
	if (!path || !proc) return;
	FILE *fp = fopen(path, "r");
	if (!fp) return;
	char comm[PATH_MAX] = "";
	fgets(comm, sizeof(comm), fp);
	fclose(fp);
	char *p = strrchr(comm, '/');
	if (!p || !*(p+1)) p = comm;
	strncpy(proc->comm, p, sizeof(proc->comm));
	p = strrchr(proc->comm, '\n');
	if (p) *p = '\0';
}

size_t read_meminfo()
{
	FILE *fp = fopen("/proc/meminfo", "r");
	if (!fp) return 0;
	char buffer[PATH_MAX] = "";
	size_t mem_available = 0;
	while (fgets(buffer, sizeof(buffer), fp) != NULL) {
		sscanf(buffer, "MemAvailable: %ld ", &mem_available);
		if (mem_available) break;
	}
	fclose(fp);
	return mem_available;
}

void read_status(const char *path, Process_t *proc)
{
	if (!path || !proc) return;
	FILE *fp = fopen(path, "r");
	if (!fp) return;
	char buffer[PATH_MAX] = "";
	proc->rss = proc->swap = 0;
	while (fgets(buffer, sizeof(buffer), fp) != NULL) {
		sscanf(buffer, "VmRSS: %ld ", &proc->rss);
		sscanf(buffer, "VmSwap: %ld ", &proc->swap);
		if (proc->rss && proc->swap) break;
	}
	fclose(fp);
	proc->total = proc->rss + proc->swap;
	return;
}

int update(Process_t *proc_summary, Process_t *proc_list, int proc_len)
{
	if (!proc_summary || !proc_list || proc_len <= 0) return -1;
	memset(proc_summary, 0, sizeof(*proc_summary));
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
	int idx = 0;
	while ((dp_item = readdir(dp)) != NULL) {
		if (dp_item->d_type != DT_DIR) continue;
		strncpy(path, dp_item->d_name, sizeof(path));
		for (char *p = path; p && *p && (proc.pid = isdigit(*p)) != 0; p++);
		if (proc.pid == 0) continue;
		proc.pid = atoi(path);

		sprintf(path, "/proc/%d/status", proc.pid);
		read_status(path, &proc);
		proc_summary->rss += proc.rss;
		proc_summary->swap += proc.swap;
		proc_summary->total += proc.total;
		sprintf(path, "/proc/%d/comm", proc.pid);
		read_comm(path, &proc);

		if (idx < proc_len) {
			proc_list[idx] = proc;
			idx++;
			continue;
		}
		for (idx = 0; idx < proc_len; idx++) {
			if (proc_list[idx].total >= proc.total) continue;
			proc_list[idx] = proc;
			break;
		}
		idx = proc_len;
	}
	closedir(dp);
	return 0;
}

int proc_cmp(const void *p1, const void *p2)
{
	if (!p1 || !p2) return 0;
	return ((Process_t*)p2)->total - ((Process_t*)p1)->total;
}

int monitor()
{
	const double MINPMEM = 85.;           // 触发需要的最少内存占比
	const double MINAVAIMEM = 250*1024;   // 可用内存小于该值触发
	const double MINMEM = 250*1024;       // 需要的最少Termux最大内存占用进程内存占用
	const double MINSTOPMEM = 600*1024;   // 考虑自动暂停内存占用超过该值的进程
	const double MINKILLMEM = 3700*1024;  // 考虑自动KILL掉内存占用超过该值的进程
	char *NOTIFICATIOR = "termux-notification";

	char buffer_title[PATH_MAX] = {0};
	char buffer_content[PATH_MAX] = {0};
	Process_t proc_summary = {0};
	Process_t proc_list[3] = {0};
	struct sysinfo info;
	sysinfo(&info);
	const double total_mem = (info.totalram+info.totalswap)/1024.;
	double pmem = 0;
	time_t timep;
	struct tm *timetmp;
	int hold = 0;
	size_t freeram = 0;
	bool active = 0;

	printf("[INFO] 自动监视模式工作\n");
	while (true) {
		usleep(0.5*1e6);
		sysinfo(&info);
		freeram = read_meminfo();  // kb
		pmem = 100*(1.-(double)(1024*freeram+info.freeswap)/(info.totalram+info.totalswap));

		// pmem超过阈值 或者 可用内存过少
		active = pmem >= MINPMEM || freeram <= MINAVAIMEM;
		if (active) {
			update(&proc_summary, proc_list, countof(proc_list));
			qsort(proc_list, countof(proc_list), sizeof(proc_list[0]), proc_cmp);
		}

		if (!active || proc_list[0].total < MINMEM) {
			if (hold) printf("[INFO] 脱离临界情况\n");
			hold = 0;
			continue;
		}
		if (hold && hold <= 40 && proc_list[0].total < MINSTOPMEM && hold++) continue;
		time(&timep);
		timetmp = localtime(&timep);
		strftime(buffer_title, sizeof(buffer_title), "%Y.%m.%d %H:%M:%S", timetmp);
		sprintf(buffer_content, "[%s] 当前内存使用率 %.1f%% (阈值 %.1f%%)",
			buffer_title, pmem, MINPMEM);
		for (size_t i = 0; i < countof(proc_list); i++) {
			char *p = "";
			if (proc_list[i].total >= MINKILLMEM) {
				hold = 114514;
				kill(proc_list[i].pid, SIGKILL);
				p = " [SIGKILL]";
			} else if (proc_list[i].total >= MINSTOPMEM && freeram <= MINAVAIMEM) {
				kill(proc_list[i].pid, SIGSTOP);
				p = " [SIGSTOP]";
			}
			sprintf(buffer_title, "\n[%4.1f%%] %.1lfMB (pid:%d) %s%s",
			       100.*proc_list[i].total/total_mem, proc_list[i].total/1024.,
			       proc_list[i].pid, proc_list[i].comm,
			       p);
			strncat(buffer_content, buffer_title, sizeof(buffer_content)-strlen(buffer_content)-1);
		}
		sprintf(buffer_title, "[WARN] 内存占用警告 (%.1f%%)", pmem);

		printf("[TITLE] %s\n", buffer_title);
		printf("[CONTENT] %s\n", buffer_content);

		if (hold && hold <= 40 && hold++) continue;
		hold = 1;

		if (fork() == 0) {
			execvp(NOTIFICATIOR, (char*[]){
			       NOTIFICATIOR,
			       "--title", buffer_title,
			       "--content", buffer_content,
			       "--priority", "max",
			       "--vibrate", "500",
			       "--sound", NULL});
			exit(-1);
		}
		usleep(0.5*1e6);
	}
}

int main(int argc, char *argv[])
{
	int ch = 0;
	bool flg_once = false;
	while ((ch = getopt(argc, argv, "hp")) != -1) {
		switch (ch) {
		case '?':
		case 'h':
			printf("Usage: %s [-hp]\n", argv[0]);
			return 0;
			break;
		case 'p':
			flg_once = true;
			break;
		}
	}
	if (!flg_once) return monitor();

	const size_t len = 20;
	Process_t proc_summary = {0};
	Process_t proc_list[len];
	struct sysinfo info;
	sysinfo(&info);
	const double total_mem = (info.totalram+info.totalswap)/1024.;
	update(&proc_summary, proc_list, countof(proc_list));
	qsort(proc_list, countof(proc_list), sizeof(proc_list[0]), proc_cmp);

	printf("Top %ld:\n", len);
	for (size_t i = 0; i < countof(proc_list); i++) {
		printf("[%4.1f%%] %.1lfMB (rss:%.1f ,swap:%.1f) (pid:%d) %s\n",
		       100.*proc_list[i].total/total_mem, proc_list[i].total/1024.,
		       proc_list[i].rss/1024., proc_list[i].swap/1024.,
		       proc_list[i].pid, proc_list[i].comm);
	}
	printf("SUMMARY:\n- RSS(%.1f/%.1f [%.1f%%])\n- SWAP(%.1f/%.1f [%.1f%%])\n- Total(%.1f/%.1f [%.1f%%])\n",
	       proc_summary.rss/1024., info.totalram/1024./1024., proc_summary.rss*1024./info.totalram,
	       proc_summary.swap/1024., info.totalswap/1024./1024., proc_summary.swap*1024./info.totalswap,
	       proc_summary.total/1024., total_mem/1024., proc_summary.total*1024./(info.totalram+info.totalswap)
	       );
	return 0;
}

