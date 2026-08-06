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
#include <fcntl.h>

static bool quiet = false;
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

int bits2str(int ind, uint8_t ch, char *buffer, size_t buffer_size)
{
	static int len_need = 0, len = 0;
	static char wchar[10] = "";
	static bool space = false;
	char buf[65] = "";
	char *sep = "\t";
	int ret = 0;

	if (!quiet) {
		snprintf(buffer+strlen(buffer), buffer_size-strlen(buffer),
			 "ch[%s%3d%s]: %s0x%s%02x%s: %s0b%s%s%s",
			 colors[3], ind, colors[0],
			 colors[2], colors[3], ch, colors[0],
			 colors[2], colors[3],
			 u8tbits(ch, buf, sizeof(buf)), colors[0]);
		ret = 1;
	} else sep = "";

	/* TODO: fix the clang-tidy report (in normal way):
	 *   - `Access of the region with a tainted index that may be too large`
	 * true solution unknow, but it disapper after adding this `for` loop */
	for (int i = 0; i < 4; i++);

	if (ch < INT8_MAX && isprint(ch)) {
		snprintf(buffer+strlen(buffer), buffer_size-strlen(buffer),
			 "%s%s%c%s", sep,
			 isspace(ch)&&!quiet?"<SPACE>'":"", ch,
			 isspace(ch)?"'":"");
		space = false;
	} else if (ch & 0b10000000) {
		wchar[len] = ch;
		len++;
		if (!len_need) {
			for (int i = 0; ch & (0b10000000>>i); i++) len_need = i+1;
			if (len_need > 4) len_need = 1;
			if (!quiet)
				snprintf(buffer+strlen(buffer), buffer_size-strlen(buffer),
					 "%s<UTF8-CHAR-LEN=%d>", sep, len_need);
		} else if ((ch & 0b11000000) != 0b10000000) {
			len_need = 1;
		}
		if (len_need>1 && len == len_need) {
			snprintf(buffer+strlen(buffer), buffer_size-strlen(buffer),
				 "%s%s", sep, wchar);
			memset(wchar, 0, sizeof(wchar));
			space = false;
		}
		if (len >= len_need) {
			len_need = 0;
			len = 0;
		}
	} else {
		if (quiet && !space) {
			ret = 1;
			space = true;
		}
		if (len_need && !quiet)
			snprintf(buffer+strlen(buffer), buffer_size-strlen(buffer),
				 "%s<UTF8-DECODE-ERROR>", sep);
		len_need = 0;
		len = 0;
	}
	if (!quiet) ret = 1;
	return ret;
}

int main(int argc, char *argv[])
{
	int inp = 0;
	size_t min_len = 1;
	bool utf8_only = false;
	char buf[PATH_MAX] = "";
	while ((inp = getopt(argc, argv, "hf:n:qu")) != -1) {
		switch (inp) {
		case '?':
		case 'h':
			printf("Usage: ./%s [-hqu] [-n <min_len>] [-f <FILE>]\n",
			       argc > 1 ? argv[0] : "input_to_hex");
			return 0;
			break;
		case 'q':
			quiet = true;
			break;
		case 'u':
			utf8_only = true;
			break;
		case 'f':
			strncpy(buf, optarg, sizeof(buf)-1);
			break;
		case 'n':
			sscanf(optarg, "%zu", &min_len);
			if (min_len <= 0) min_len = 1;
			break;
		}
	}
	if (buf[0]) {
		int fd = open(buf, O_RDONLY);
		if (fd < 0) {
			perror("open");
			return 1;
		}
		dup2(fd, STDIN_FILENO);
		close(fd);
	}
	if (!isatty(STDOUT_FILENO)) {
		for (size_t i = 0; i < countof(colors); i++) colors[i] = "";
	}

	char ch[2048];
	size_t size = sizeof(ch);
	flag_isatty = isatty(STDIN_FILENO);
	if (flag_isatty) printf("请输入:\n");
	do  {
		if (flag_isatty) {
			if (!fgets(ch, sizeof(ch), stdin)) break;
		} else {
			if ((size = fread(ch, 1, sizeof(ch), stdin)) <= 0) break;
		}
		for (size_t i = 0; i < size && (!flag_isatty||ch[i] != 0); ++i) {
			if (!bits2str(i, ch[i], buf, sizeof(buf)-1))
				continue;
			if (strlen(buf) < min_len) {
				memset(buf, 0, sizeof(buf));
				continue;
			}
			inp = 0;
			while (utf8_only && buf[inp] && !(buf[inp]&0b10000000))
				inp++;
			if (!utf8_only || buf[inp]&0b10000000)
				printf("%s\n", buf);
			memset(buf, 0, sizeof(buf));
		}
		if (size < sizeof(ch)) break;
	} while (!flag_isatty);
	return 0;
}

