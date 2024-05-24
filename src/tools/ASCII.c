/*
 *   Copyright (C) 2021 u0_a221
 *
 *   文件名称：ASCII.c
 *   创 建 者：u0_a221
 *   创建日期：2021年05月29日
 *   描    述：
 *
 */

#include <stdio.h>

int main() {
	printf("Num |  Hex  |    Bit    | Char\n");
	for (unsigned short a = 0; a < 260; a++) {
		unsigned int out = 0;
		for (int i = 0; i < 32; i++) out = out * 10 + ((a << i) >> 31);
		out = 0 - out;
		printf("%03d | 0x%03x | %09d | ", a, a, out);  //循环打印输出
		switch (a) {
		case '\r':
			printf("\\r\n");
			break;
		case '\n':
			printf("\\n\n");
			break;
		case '\t':
			printf("\\t\n");
			break;
		default:
			printf("%c\n", a < '!' ? ' ' : a);
			break;
		}
	}
	return 0;
}

