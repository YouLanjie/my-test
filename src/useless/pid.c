#include <stdio.h>
#include <stdlib.h>    //exit();
#include <string.h>

#include <sys/types.h> //pid_t
#include <signal.h>    //signal();
#include <unistd.h>    //pause();
#include <wait.h>      //wait();

int wait_flag;
void stop();

int main() {
	pid_t pid1,pid2;

	signal(2,stop);
	pause();
	while((pid1 = fork()) == -1)
	if(pid1 > 0) {
		while((pid2 = fork()) == -1)
		if(pid2 > 0) {
			wait_flag = 1;
			sleep(5);
			kill(pid1,16);
			kill(pid2,17);
			wait(0);
			wait(0);
			printf("程序结束！\n");
			exit(0);
		}
		else {
			wait_flag = 1;
			signal(17,stop);
			printf("进程2结束！\n");
			exit(0);
		}
	}
	else {
		wait_flag = 1;
		signal(16,stop);
		printf("进程1结束！\n");
		exit(0);
	}
	return 0;
}

void stop() {
	wait_flag = 0;
	printf("Now in stop\n");
}

