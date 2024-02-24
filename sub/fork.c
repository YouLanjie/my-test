/*
 *   Copyright (C) 2023 YouLanjie
 *
 *   文件名称：a.c
 *   创 建 者：youlanjie
 *   创建日期：2023年03月19日
 *   描    述：测试fork程序
 *
 */


#include "../include/head.h"

void *shell_f(void)
{
	int PID;

	char cmd[1024] = "exit";
	printf("shell command > ");
	getchar();
	fgets(cmd, 1024, stdin);

	PID = fork();
	if (PID != 0) {
		printf("Pid of the child: %d\n", PID);
		static int ret = 0;
		return &ret;
	} else {
		usleep(500000);
		printf("\033[1;33mExec: %s\033[0m\n", cmd);
		system(cmd);
		exit(0);
	}
	printf("Maybe someting is worng?\nThat is not good.Check your self.\n");
	exit(1);
}


