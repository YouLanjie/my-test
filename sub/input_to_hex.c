/*
 *   Copyright (C) 2023 YouLanjie
 *
 *   文件名称：input_to_hex.c
 *   创 建 者：youlanjie
 *   创建日期：2023年03月31日
 *   描    述：将输入使用十六进制逐行打印
 *
 */


#include "../include/head.h"

union ctools_cmd_arg input_to_hex(void)
{
	char ch[10];
	printf("请输入:\n");
	getchar();
	fgets(ch, 10, stdin);
	for (int i = 0; i < 10; ++i) {
		if (ch[i] < '!') {
			printf("ch[%d]:\033[1;32m0x\033[1;33m%x\033[0m\n", i, ch[i]);
		} else {
			printf("ch[%d]:\033[1;32m0x\033[1;33m%x\033[0m\t%c\n", i, ch[i], ch[i]);
		}
	}
	union ctools_cmd_arg arg = {.num = 0};
	return arg;
}


