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

char *i2b(unsigned long data, int bytes, char *buf, size_t buf_size)
{
	if (!buf || buf_size == 0) return NULL;
	if (bytes <= 0 || bytes > sizeof(data)) bytes = sizeof(data);

	int bits = bytes * 8;
	if (bits >= buf_size) bits = buf_size - 1;  // 留1字节给'\0'

	for (int i = 0; i < bits; ++i) {
		unsigned long mask = 1UL << (bits - 1 - i);  // 1UL 保证至少 32/64 位
		buf[i] = (data & mask) ? '1' : '0';
	}
	buf[bits] = '\0';
	return buf;
}

int print_bits(int ind, long data, int len)
{
	char buf[65] = "";
	printf("ch[\033[1;33m%3d\033[0m]: "
	       "\033[1;32m0x\033[1;33m%02lx\033[0m: "
	       "\033[1;32m0b\033[1;33m%s\033[0m",
	       ind, data, i2b(data, len, buf, 65));
	if (data >= '!' && data < CHAR_MAX)
		printf("\t%c\n", (char)data);
	else printf("\n");
	return 0;
}

int main(void)
{
	int len = 256;
	char ch[len];
	printf("请输入:\n");
	fgets(ch, len, stdin);
	for (int i = 0; i < len && ch[i] != 0; ++i) {
		print_bits(i, ch[i], 1);
	}
	return 0;
}


