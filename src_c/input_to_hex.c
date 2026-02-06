/*
 *   Copyright (C) 2023 YouLanjie
 *
 *   文件名称：input_to_hex.c
 *   创 建 者：youlanjie
 *   创建日期：2023年03月31日
 *   描    述：将输入使用十六进制逐行打印
 *
 */


#include "tools.h"

int main(void)
{
	int len = 256;
	char ch[len];
	printf("请输入:\n");
	fgets(ch, len, stdin);
	for (int i = 0; i < len && ch[i] != 0; ++i) {
		printf("ch[\033[1;33m%3d\033[0m]: "
		       "\033[1;32m0x\033[1;33m%02x\033[0m: "
		       "\033[1;32m0b\033[1;33m%08b\033[0m",
		       i, ch[i] & 0xff, ch[i] & 0xff);
		if (ch[i] < '!') {
			printf("\n");
		} else {
			printf("\t%c\n", ch[i]);
		}
	}
	return 0;
}


