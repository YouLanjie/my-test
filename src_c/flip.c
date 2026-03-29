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

int main(int argc, char *argv[])
{
	if (argc < 2) {
		fprintf(stderr, "Usage: flip <FILE> [NUM]\n");
		return 1;
	}
	FILE *fp = fopen(argv[1], "rb");
	if (!fp) {
		fprintf(stderr, "打开文件失败：%s\n", argv[1]);
		return 1;
	}
	int operator = 0;
	if (argc >= 3) sscanf(argv[2], "%d", &operator);
	int c = 0;
	while (((c = fgetc(fp)) && 0) || c != EOF) {
		c ^= operator;
		putc(c, stdout);
	}
	fclose(fp);
	return 0;
}


