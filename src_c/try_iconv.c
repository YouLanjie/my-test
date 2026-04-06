/*
 *   Copyright (C) 2026 u0_a221
 *
 *   文件名称：iconv.c
 *   创 建 者：u0_a221
 *   创建日期：2026年04月05日
 *   描    述：编码测试
 *
 */


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <iconv.h>
/*#include <wchar.h>*/

int main(int argc, char *argv[])
{
	char fromcode[1024] = {0}, tocode[1024] = {0};
	printf(">>>> GET INPUT\n");
	fread(fromcode, 1, sizeof(fromcode)-1, stdin);
	printf("<<<< END INPUT\n");
	printf("Fromcoode Str Size: %ld\n", strlen(fromcode));
	printf(">>>> PRINT SOURCE\n%s\n", fromcode);
	printf("<<<< END SOURCE\n");
	iconv_t cd = iconv_open("GBK", "UTF-8");
	if (cd == (iconv_t)-1) {
		perror("iconv_open");
		return 1;
	}
	size_t limit1 = strlen(fromcode);
	size_t limit2 = sizeof(tocode)-1;
	char *inp = fromcode, *out = tocode;
	if (iconv(cd, &inp, &limit1, &out, &limit2) == (size_t)-1) {
		iconv_close(cd);
		printf(">>>> DEBUG PRINT INPUT\n%.*s\n", 100, inp);
		printf("<<<<\n");
		*out=0;
		printf(">>>> DEBUG PRINT OUTPUT\n%s\n", tocode);
		printf("<<<<\n");
		perror("iconv");
		if (inp > fromcode)
			fprintf(stderr, "[INFO] 解码指针已经移动%ld字节\n", inp-fromcode);
		if (out > tocode)
			fprintf(stderr, "[INFO] 编码指针已经移动%ld字节\n", out-tocode);
		fprintf(stderr, "[INFO] in:%ld, ou:%ld\n", limit1, limit2);
		return 1;
	}
	*out=0;
	printf("Tocoode Str Size: %ld\n", strlen(tocode));
	printf(">>>> OUTPUT\n%s\n", tocode);
	iconv_close(cd);
	return 0;
}
