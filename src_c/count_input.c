/*
 *   Copyright (C) 2026 u0_a221
 *
 *   文件名称：count_input.c
 *   创 建 者：u0_a221
 *   创建日期：2026年02月09日
 *   描    述：测试执行速度（针对python同名程序）
 *
 */


#include <signal.h>
#include <stdio.h>
#include <time.h>
#include <unistd.h>

int LOCK = 1;

void quit(int i) {
	LOCK = 0;
}

int main()
{
	if (isatty(STDIN_FILENO)) {
		printf("Not allow stdin is a tty\n");
		return 0;
	}
	struct timespec begging, t;
	clock_gettime(CLOCK_MONOTONIC, &begging);

	register int c = 0;
	unsigned register int count = 0;
	long sec = 0, nsec = 0;
	double diff;

	signal(SIGINT, quit);
	while (LOCK && sec < 5 && c != -1) {
		c = fgetc(stdin);
		if (c == -1) continue;
		count++;

		clock_gettime(CLOCK_MONOTONIC, &t);
		nsec = t.tv_nsec - begging.tv_nsec;
		sec = t.tv_sec - begging.tv_sec;
		if (nsec < 0) {
			sec--;
			nsec += 1000000000;
		}
		diff = sec + nsec/1000000000.0;

		if (count % 10000)  continue;
		printf("COUNT: %d [%ld.%09lds] (%lftps)     \r",
		       count, sec, nsec, count/diff);
		fflush(stdout);
		/*printf("%ld.%06ld\n", t.tv_sec, t.tv_nsec);*/
	}
	printf("\nRESULT: %d [%ld.%09lds] (%lftps)     \n",
	       count, sec, nsec, count/diff);
	printf("QUIT\n");
	return 0;
}
