/*
 *   Copyright (C) 2023 YouLanjie
 *   
 *   文件名称：CPU.c
 *   创 建 者：youlanjie
 *   创建日期：2023年03月11日
 *   描    述：挤占CPU性能，建议和nice或renice使用，效果更佳
 *
 */


#include "../include/head.h"

static void * running();
static int  run_status = 1;
static int  pthread_num = 1;
static int  hide        = 0;

void *CPU() {
	static int ret = 0;
	int num = 0;
	pthread_t pid;
	printf("请输入线程数：\n");
	scanf("%d", &num);
	getchar();

	if (num <= 0 || num > 225) {
		perror("Error!线程数为负数或过大!\n");
		ret = -2;
		return &ret;
	}

	printf("隐藏?：\n");
	scanf("%d", &hide);
	getchar();

	printf("线程数为：%d\n"
	       "现在，请按下回车开始程序:\n",
	       num);
	getchar();
	printf("正在创建线程中...\n");
	for (int i = 0; i < num; i++) {
		pthread_create(&pid, NULL, running, NULL);
		pthread_num++;
	}
	printf("线程创建完成!\n"
	       "若想退出程序，请按下回车\n");
	getchar();
	run_status = 0;
	ret = 0;
	return &ret;
}

static void * running()
{
	static int num, i = 0;
	num = pthread_num;
	while (run_status) {
		if (hide == 0) {
			printf("This is pthread No.%d\r", num);
		}
		i = i + i * 2 - i / 2;
	}
	return NULL;
}



