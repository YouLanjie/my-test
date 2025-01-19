#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <sys/time.h>

void run();
void quit();

int main() {
	int stat = 0;
	struct itimerval tick;

	signal(SIGINT, quit);
	signal(SIGALRM, run);
	printf("这是一个测试时钟的程序\n");
	tick.it_interval.tv_sec = 1;
	tick.it_interval.tv_usec = 0;

	tick.it_value.tv_sec = 0;
	tick.it_value.tv_usec = 1;

	// alarm(1);
	stat = setitimer(ITIMER_REAL, &tick, NULL);
	if (stat) {
		perror("setitimer");
		return -1;
	}
	while (1);
	// for (int i = 0; i < 10; i++) {
		// printf("%d\n", i);
		// sleep(1);
	// }
	return 0;
}

void run() {
	printf("Time is up!\n");
	return;
}

void quit() {
	printf("\nExiting...\n");
	exit(0);
}

