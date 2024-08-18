/*
 *   Copyright (C) 2023 YouLanjie
 *
 *   文件名称：a.c
 *   创 建 者：youlanjie
 *   创建日期：2023年03月19日
 *   描    述：测试fork程序
 *
 */


#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

int main(int argc, char *argv[])
{
	if (argc < 2) {
		printf("Usage: %s <Command>\n", argv[0]);
		return -1;
	}
	int PID;
	PID = fork();
	if (PID != 0)    /* Father */
		return 0;
	/* Child */
	usleep(500000);
	printf("The command is:\n%s\n", argv[1]);
	system(argv[1]);
	return 0;
}


