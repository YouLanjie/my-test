/*
 *   Copyright (C) 2023 YouLanjie
 *
 *   文件名称：b.c
 *   创 建 者：youlanjie
 *   创建日期：2023年01月25日
 *   描    述：
 *
 */


#include "ctools.h"

int main(void)
{
	char ch[10];
	printf("请输入:\n");
	scanf("%s", ch);
	for (int i = 0; i < 10; ++i) {
		if (ch[i] < '!') {
			printf("ch[%d]:0x%x\n", i, ch[i]);
		} else {
			printf("ch[%d]:0x%x\t%c\n", i, ch[i], ch[i]);
		}
	}
	return 0;
}


