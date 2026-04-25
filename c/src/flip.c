/*
 *   Copyright (C) 2026 u0_a221
 *
 *   文件名称：flip.c
 *   创 建 者：u0_a221
 *   创建日期：2026年03月29日
 *   描    述：翻转指定文件的每一位并输出到标准输出
 *   应用：在以下路径运行命令解密网易云音乐缓存
 *   /storage/emulated/0/Android/data/com.netease.cloudmusic/files/Cache/Music3/[0-9]/
 *   $ ls *.nmsf|while read i;do echo $i;flip "$i" 163 >~/link/Download/$i.m4a;done
 */


#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <unistd.h>

#define BUFSIZE (1024*256)

int main(int argc, char *argv[])
{
	bool tty = isatty(STDIN_FILENO);
	if (argc < 2 && tty) {
		fprintf(stderr,
			"Usage: flip <FILE> [NUM]\n"
			"       flip [NUM] < ./FILE\n");
		return 1;
	}
	FILE *fp = tty ? fopen(argv[1], "rb") : stdin;
	if (!fp) {
		fprintf(stderr, "打开文件失败：%s\n", argv[1]);
		return 1;
	}
	int operator = 0;
	if (argc >= 3 - !tty) operator = atoi(argv[2-!tty]);;
	uint8_t buffer[BUFSIZE] = {0};
	size_t size = 0, i = 0;
	while ((size = fread(buffer, 1, BUFSIZE, fp)) > 0) {
		for (i = 0; i<size; buffer[i]^=operator,i++);
		/*fwrite(buffer, sizeof(*buffer), size, stdout);*/
		write(STDOUT_FILENO, buffer, size);
	}
	if (tty) fclose(fp);
	return 0;
}

