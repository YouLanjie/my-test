/*
 *   Copyright (C) 2023 YouLanjie
 *
 *   文件名称：bit.c
 *   创 建 者：youlanjie
 *   创建日期：2023年02月18日
 *   描    述：为了测试dwm使用位运算切换工作区的原理，
 *             搞清楚怎么配置而做的测试程序
 *
 */


#include "include/tools.h"

int main(void)
{
	int i = 1;
	printf("Plese input a number:\n");
	scanf("%d", &i);
	int out = 1 << i;
	printf("out:%%d:%d\t%%x:%x\n", out, out);
	for (i = 0; out != 0; out = out << 1) {
		i++;
		printf("num:%d\tout:%%d:%d\t%%d:%d\n", i, out, 1 << i);
		usleep(20000);
	}
	printf("your input is %d\n", 32 - i);
	return 0;
}

