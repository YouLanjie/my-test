/*
 *   Copyright (C) 2023 YouLanjie
 *
 *   文件名称：a.c
 *   创 建 者：youlanjie
 *   创建日期：2023年03月19日
 *   描    述：测试fork程序
 *
 */


#include "include/tools.h"

int main(int argc, char *argv[])
{
	int PID;
	if (argc < 2) {
		printf("Useg:\n./argv[0] \"command\"\n");
		return -1;
	} else {
		PID = fork();
		if (PID != 0) {
			printf("This is the pid of the child:%d\n", PID);
			return 0;
		} else {
			usleep(500000);
			printf("This is a tips form the child\n");
			printf("The command is:\n%s\n", argv[1]);
			system(argv[1]);
			return 0;
		}
	}
	printf("Maybe someting is worng?\nThat is not good.Check your self.\n");
	return 1;
}


