/*
 *   Copyright (C) 2023 YouLanjie
 *
 *   文件名称：input_to_hex.c
 *   创 建 者：youlanjie
 *   创建日期：2023年03月31日
 *   描    述：将输入使用十六进制逐行打印
 *
 */


#include <ctype.h>
#include <limits.h>
#include <stdbool.h>
#include <stdcountof.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

static bool flag_isatty = false;
static const char *colors[] = {
	"\e[0m",
	"\e[1;31m",
	"\e[1;32m",
	"\e[1;33m",
};

static char *u8tbits(uint8_t data, char *buf, size_t buf_size)
{
	if (!buf || buf_size == 0) return NULL;

	size_t bits = sizeof(data)*8;
	if (bits >= buf_size) bits = buf_size - 1;  // 留1字节给'\0'

	for (size_t i = 0; i < bits; ++i) {
		unsigned long mask = 1 << (bits - i - 1);
		buf[i] = (data & mask) ? '1' : '0';
	}
	buf[bits] = '\0';
	return buf;
}

int print_bits(int ind, uint8_t ch)
{
	static int len_need = 0, len = 0;
	static char wchar[10] = "";
	char buf[65] = "";
	printf("ch[%s%3d%s]: "
	       "%s0x%s%02x%s: "
	       "%s0b%s%s%s",
	       colors[3], ind, colors[0],
	       colors[2], colors[3], ch, colors[0],
	       colors[2], colors[3], u8tbits(ch, buf, sizeof(buf)), colors[0]);

	if (ch < INT8_MAX && isprint(ch)) {
		printf("\t%s%c%s", isspace(ch)?"<SPACE>'":"", ch,
		       isspace(ch)?"'":"");
	} else if (ch & 0b10000000) {
		wchar[len] = ch;
		len++;
		if (!len_need) {
			for (int i = 0; ch & (0b10000000>>i); i++) len_need = i+1;
			if (len_need > 4) len_need = 1;
			printf("\t<UTF8-CHAR-LEN=%d>", len_need);
		} else if ((ch & 0b11000000) != 0b10000000) {
			len_need = 1;
		}
		if (len_need>1 && len == len_need) {
			printf("\t%s", wchar);
			memset(wchar, 0, sizeof(wchar));
		}
		if (len >= len_need) {
			len_need = 0;
			len = 0;
		}
	} else {
		if (len_need) printf("\t<UTF8-DECODE-ERROR>");
		len_need = 0;
		len = 0;
	}
	printf("\n");
	return 0;
}

int main(void)
{
	char ch[2048];
	printf("请输入:\n");
	if (!isatty(STDOUT_FILENO)) {
		for (size_t i = 0; i < countof(colors); i++) colors[i] = "";
	}
	flag_isatty = isatty(STDIN_FILENO);
	do  {
		if (!fgets(ch, sizeof(ch), stdin)) break;
		for (size_t i = 0; i < sizeof(ch) && ch[i] != 0; ++i) {
			print_bits(i, ch[i]);
		}
	} while (!flag_isatty);
	return 0;
}

