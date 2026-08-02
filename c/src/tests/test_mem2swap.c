/**
 * @file        a.c
 * @author      Chglish
 * @date        2026-08-01
 * @brief       测试swap,申请70%剩余swap空间
 *              并使用子进程全力申请内存将其挤到交换空间
 */

#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/sysinfo.h>
#include <sys/wait.h>
#include <unistd.h>
#include <wait.h>

static const int block_size = 1*1024*1024;
struct chain_t {
	char *p;
	struct chain_t *next;
};
struct chain_t *c_alloc()
{
	struct chain_t *p = malloc(sizeof(*p));
	if (!p) return NULL;
	*p = (struct chain_t){
		.p = malloc(block_size),
		.next = NULL,
	};
	if (!p->p) {
		free(p);
		return NULL;
	}
	memset(p->p, 114, block_size);
	return p;
}
void c_free(struct chain_t *p)
{
	if (!p) return;
	struct chain_t *lp;
	while (p) {
		free(p->p);
		lp = p;
		p = p->next;
		free(lp);
	}
	return;
}
enum pmode_t {PMODE_minused, PMODE_minfree};
struct chain_t *c_pressure(enum pmode_t mode, size_t limit, size_t *size, size_t *count)
{
	struct chain_t *head = NULL, *p = c_alloc();
	if (!p) return NULL;
	size_t tmp_size = 0, tmp_count = 0;
	head = p;
	struct sysinfo info = {};
	while (p) {
 		sysinfo(&info);
		if (mode == PMODE_minfree) {
			if (info.totalswap > block_size) {
				if (info.freeswap <= limit)break;
			} else if (info.freeram <= limit) break;
		} else if (info.freeram < 50*1024*1024 || tmp_size > limit) break;
		p->next = c_alloc();
		p = p->next;
		tmp_size += block_size+sizeof(*p);
		tmp_count++;
	}
	if (size) *size = tmp_size;
	if (count) *count = tmp_count;
	return head;
}
void print_ps()
{
	printf("[INFO] 打印free输出：\e[2m\n");
	system("free -h");
	printf("\e[0m[INFO] 打印ps输出：\e[2m\n");
	system("ps auxww --sort=-%mem|head -n 10");
	printf("\e[0m[INFO] 命令输出结束\n");
}


void child()
{
	struct chain_t *head = NULL;
	head = c_pressure(PMODE_minfree, 1*1024*1024*1024ULL, NULL, NULL);
	printf("[CHILD] 子进程挤出任务完成\n");
	print_ps();
	c_free(head);
	return;
}

int main(void)
{
	struct chain_t *head = NULL;
	size_t size = 0, count = 0;

	struct sysinfo info = {};
 	sysinfo(&info);
	if (info.totalswap < 100) {
		printf("[WARN] 没有可用交换空间\n");
		info.freeswap = info.freeram;
	}
	head = c_pressure(PMODE_minused, info.freeswap*0.7, &size, &count);
	if (!head) return 1;

	printf("[INFO] 总占用内存应该为：%.2fMB\n", size/(1024*1024.));
	printf("[INFO] 链长：%zu\n", count);
	print_ps();
	pid_t pid = fork();
	if (!pid) {
		c_free(head);
		child();
		exit(0);
	}
	printf("[INFO] 等待子进程(PID%d)挤出。。。。\n", pid);
	while (wait(NULL) != -1);
	printf("[INFO] 子进程(PID%d)已退出\n", pid);
	print_ps();

	/* 假设是恶意代码 */
	pid = fork();
	if (!pid) {
		sleep(365*24*60*60);
		c_free(head);
		printf("[INFO] 子进程退出\n");
		exit(0);
	}
	printf("[INFO] 注意新子进程(PID%d)\n", pid);
	/* 父进程特意让不wait让子进程变成孤儿进程被systemd接管 */

	c_free(head);
	printf("[INFO] 已释放内存\n");
	print_ps();
	printf("[INFO] 主进程退出\n");
	return 0;
}

